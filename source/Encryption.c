/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

/* system includes */
#include <stdio.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "XdkSensorHandle.h"
#include "queue.h"

/* mbedTLS system includes */
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"

/* user includes */
#include "Encryption.h"
#include "UserConfig.h"
#include "SystemConfig.h"
#include "SensorData.h"
#include "Http.h"
#include "CoAPServer.h"

/**
 * struct to hold all the crypto contexts which are
 * used for encryption/decryption and hash calculation
 */
typedef struct mbedEncryptionHandler_S{
	mbedtls_sha256_context sha256ctx;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_pk_context pk;
	mbedtls_entropy_context entropy;
} mbedEncryptionHandler_T;
static mbedEncryptionHandler_T mbedEncryptionHandleVar;

/**
 * This entropy function is called to setup the entropy
 * source for the crypto functions. Must be called
 * AFTER used sensor is initialized finished.
 *
 * @param[out] accelSensData_ptr
 * This reference will hold the acceleromter sensor values
 *
 * @param[out] output_ptr
 * This reference will hold the output data. Must be filled with
 * payload of the provided length.
 *
 * @param[out] len
 * This variable holds the maximum length of the data
 *
 * @param[out] oLen_ptr
 * The actual amount of bytes put into the output buffer
 *
 * @return
 * 0, if successful<br>
 */
static int setupEntropySource(void* accelSensData_ptr, uint8_t *output_ptr, size_t len, size_t *oLen_ptr)
{
	GetAcceleromterSensorEntropyData(accelSensData_ptr);
	/* use random number generator based on gyroscope sensor */
	srand(*(unsigned int*)accelSensData_ptr);
	memset( output_ptr, (int)accelSensData_ptr, len);
	*oLen_ptr = sizeof(int32_t);

	return 0;
}

/**
 * This function is used to setup the seed for
 * encryption. This has to be done once per application.
 * The EntropyStartUpPoint is a random string which should
 * be unique for each application.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T setupCryptoSeed(void)
{
	Retcode_T ret = RETCODE_FAILURE;
	/* just a random string which differs for each application.
	 * Is used as a small protection against the lack of
	 * startup entropy
	 * */
#ifdef ENABLE_CONSUMER
	const char *EntropyStartUpPoint = "ksjadsweenfjsknfskASASADNalsk";
#else
	const char *EntropyStartUpPoint = "asdlsaldoiqweopqwrpepppsxcmdd";
#endif

	/* start seed generation */
	ret = mbedtls_ctr_drbg_seed(&mbedEncryptionHandleVar.ctr_drbg, mbedtls_entropy_func, &mbedEncryptionHandleVar.entropy, (unsigned const char*)EntropyStartUpPoint, strlen(EntropyStartUpPoint));

	return ret;
}

/**
 * This function initializes the crypto library and sets up
 * the global context handler struct with initial values.
 * Initialize entropy, sha256, ctr_drbg and pk modules.
 *
 * Sets up the entropy source and the seed value for encryption
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T InitMbedCrypto()
{
	int32_t data = 0;
	Retcode_T ret = RETCODE_FAILURE;

	/* write keys into flash and read keys from flash into local buffer during startup
	#include "BCDS_MCU_Flash.h"
	uint32_t address = 0x000B6000;
	char readbuffer[3243] = {0};
	MCU_Flash_Write((uint8_t *)address, RSAPrivateKey, strlen(RSAPrivateKey));
	MCU_Flash_Read((uint8_t *)address, readbuffer, 3243);
	vTaskDelay(SECONDS(3));
	printf("BUFFER after: %s \n\r", readbuffer);
	vTaskDelay(SECONDS(3));
	*/

	/* initialize used crypto modules */
	mbedtls_sha256_init(&mbedEncryptionHandleVar.sha256ctx);
	mbedtls_entropy_init(&mbedEncryptionHandleVar.entropy);
	mbedtls_ctr_drbg_init(&mbedEncryptionHandleVar.ctr_drbg);
	mbedtls_pk_init(&mbedEncryptionHandleVar.pk);

	/* Add entropy source */
	ret = mbedtls_entropy_add_source(&mbedEncryptionHandleVar.entropy, setupEntropySource, &data, 256, MBEDTLS_ENTROPY_SOURCE_STRONG);
	if(RETCODE_SUCCESS != ret) return ret;

	/* setup seed for encryption */
	ret = setupCryptoSeed();

	return ret;
}

/**
 * This function is called to calculate the data
 * hash of a specified data buffer
 *
 * @param[in] payload_ptr
 * This reference holds the "unhashed" payload data
 *
 * @param[in] iLength
 * Length of the incoming payload
 *
 * @param[out] calculatedHash_ptr
 * This buffer will hold the calculated hash value of
 * the payload data
 *
 * @param[in] iLengthOBuffer
 * Length of the hash buffer -> Must be equal to DATA_HASH_BUFF_SIZE
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T CalculateHash(uint8_t const *payload_ptr, size_t iLength, uint8_t *calculatedHash_ptr, size_t iLengthOBuffer)
{
	Retcode_T ret = RETCODE_FAILURE;

	if( (NULL!= payload_ptr) && (NULL != calculatedHash_ptr) && (0 < iLength) ) {
		if(iLengthOBuffer == DATA_HASH_BUFF_SIZE) {
			ret = mbedtls_sha256_starts_ret(&mbedEncryptionHandleVar.sha256ctx, 0);
			ret = mbedtls_sha256_update_ret(&mbedEncryptionHandleVar.sha256ctx, payload_ptr, iLength);
			ret = mbedtls_sha256_finish_ret(&mbedEncryptionHandleVar.sha256ctx, calculatedHash_ptr);
#ifdef ENABLE_DEBUG
			printf("Calculate hash\n\r");
#endif
		}
	}

    return ret;
}

/**
 * This function is called to encrypt data with the
 * RSA algorithm
 *
 * @param[in] payload_ptr
 * This reference holds the raw payload data to encrypt
 *
 * @param[in] iLength
 * Length of the incoming payload
 *
 * @param[out] oBuff
 * This buffer will hold the encrypted value of
 * the payload data (always padded to 128 bytes)
 *
 * @param[out] ioBuffLength
 * Size of the output buffer --> must be at least 128 bytes
 *
 * @param[out] oLength_ptr
 * Length of the encrypted data (with this algorithm padded to 128 bytes)
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T encryptData(uint8_t const *payload_ptr, size_t iLength, uint8_t *oBuff, size_t ioBuffLength, size_t *oLength_ptr)
{
	Retcode_T cryptoRet = RETCODE_FAILURE;

	/* check for NULL pointers */
	if( (NULL != payload_ptr) && (NULL != oBuff)) {
		/* free pk context */
		mbedtls_pk_free(&mbedEncryptionHandleVar.pk);

		/* parse public key */
		for(uint8_t counter = 0; counter < CONSUMER_NUMBER_MAX; ++counter) {
			if(AuthenticatedConsumerTable[counter].activeConsumer == true) {
#ifdef ENABLE_DEBUG
				printf("Data encryption pubkey:%s\n\r", AuthenticatedConsumerTable[counter].consumerPublicKey);
#endif
				cryptoRet = mbedtls_pk_parse_public_key(&mbedEncryptionHandleVar.pk, AuthenticatedConsumerTable[counter].consumerPublicKey, strlen(AuthenticatedConsumerTable[counter].consumerPublicKey) + 1);
				AuthenticatedConsumerTable[counter].activeConsumer = false;
			}
		}
		if(RETCODE_SUCCESS == cryptoRet) {
			/* start data encryption */
			/* default padding type is PKCS#1 v1.5
			 * so the length of the encrypted message is always the same
			 * independent of the payload length.
			 * For 1024 Bit RSA key, padded message is always 128 byte */
			cryptoRet = mbedtls_pk_encrypt(&mbedEncryptionHandleVar.pk, payload_ptr, iLength, oBuff, oLength_ptr, ioBuffLength, mbedtls_ctr_drbg_random, &mbedEncryptionHandleVar.ctr_drbg);
			if(RETCODE_SUCCESS == cryptoRet) {
				printf("Data encryption successful:%s; Length: %i\n\r", oBuff, *oLength_ptr);
			} else {
				printf("Failed to encrypt data\n\r");
			}
		} else {
			printf("Failed to read public key\n\r");
		}
	}

	return cryptoRet;
}

/**
 * This function is called to decrypt data which
 * was encrypted with the RSA algorithm
 *
 * @param[in] payload_ptr
 * This reference holds the encrypted payload data
 *
 * @param[in] iLength
 * Length of the incoming encrypted payload
 *
 * @param[out] decryptedData_ptr
 * This reference will hold the decrypted value of
 * the payload data
 *
 * @param[out] ioBuffLength
 * Size of the output buffer
 *
 * @param[out] oLength_ptr
 * Length of the decrypted data
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T decryptData(uint8_t const *payload_ptr, size_t iLength, uint8_t *decryptedData_ptr, size_t ioBuffLength, size_t *oLength_ptr)
{
	Retcode_T cryptoRet = RETCODE_FAILURE;

	/* check for NULL pointers */
	if( (NULL != payload_ptr) && (NULL != decryptedData_ptr)) {
		/* free pk context */
		mbedtls_pk_free(&mbedEncryptionHandleVar.pk);

		/* parse private key */
		cryptoRet = mbedtls_pk_parse_key( &mbedEncryptionHandleVar.pk, PrivateRSAKeyConsumer1024, strlen((const char*)PrivateRSAKeyConsumer1024) + 1, NULL, 0);

		if(RETCODE_SUCCESS == cryptoRet) {
			/* start data decryption */
			cryptoRet = mbedtls_pk_decrypt( &mbedEncryptionHandleVar.pk, payload_ptr, iLength, decryptedData_ptr, oLength_ptr, ioBuffLength, mbedtls_ctr_drbg_random, &mbedEncryptionHandleVar.ctr_drbg);

#ifdef ENABLE_DEBUG
			if(RETCODE_SUCCESS == cryptoRet) {
				printf("Data decryption successful:%i ; Length: %i\n\r", *decryptedData_ptr, *oLength_ptr);
			} else {
				printf("Failed to decrypt data\n\r");
			}
		} else {
			printf("Failed to read private key\n\r");
		}
#endif
	} else {
#ifdef ENABLE_DEBUG
		printf("Data decryption error\n\r");
#endif
	}
	return cryptoRet;
}
