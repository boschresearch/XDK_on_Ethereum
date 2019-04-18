/*
 Copyright (c) 2019 Robert Bosch GmbH
 All rights reserved.

 This source code is licensed under the MIT license found in the
 LICENSE file in the root directory of this source tree.
*/
 
/* system includes */

#include "FreeRTOS.h"
#include "queue.h"
#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "PAL_initialize_ih.h"
#include "PAL_socketMonitor_ih.h"
#include "Serval_Coap.h"
#include "Serval_CoapServer.h"
#include "Serval_CoapClient.h"
#include "Serval_Network.h"
#include "BCDS_BSP_LED.h"
#include "BSP_BoardType.h"

/* user includes */
#include "UserConfig.h"
#include "SystemConfig.h"
#include "CoAPClient.h"
#include "Encryption.h"
#include "Http.h"
#include "SensorData.h"
#include "SecureEdgeDevice.h"

/* locally used defines */
#define ENCRYPTED_DATA_PAYLOAD_SIZE 	128

/* Coap handler struct to hold ip address and server port of current connection */
typedef struct CoAPIpHandler_S{
	Ip_Address_T ip;
	Ip_Port_T serverPort;
} CoAPIpHandler_T;
CoAPIpHandler_T CoAPIpHandleVar = {0};

/* buffer to hold contract address */
static uint8_t ContractAddressBuffer[CONTRACT_ADDRESS_LENGTH] = {0};
/* authentication flag - is set if consumer pubKey and address is already stored at producer side.
 * There is no need to write public key twice. */
static bool ConsumerAlreadyAuthenticatedFlag = false;
/* counter to order user requests - only used for presentation to order consumer requests
 * in cyclic function after button pressed on XDK */
static uint8_t counterForUserInteraction = 0;

/**
 * This function is called after CoAP client received
 * a CoAP server reponse to his request
 *
 * @param[in] coapSession_ptr
 * This is the reference to the current CoAP session.
 *
 * @param[out] msg_ptr
 * This holds message to be filled by application for request.
 *
 * @param[out] status
 * This is the response status of the callback
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPClientResponseCallback(CoapSession_T *coapSession_ptr, Msg_T *msg_ptr, retcode_t status)
{
	/* local variable declarations */
    (void) coapSession_ptr;
    CoapParser_T parser;
    uint8_t const *payload_ptr;
	BaseType_t queueResult = pdFAIL;
	CoapPayloadLength_t iEncryptedLength = 0;
	queueHandler_T queueHandlerCoAPClient = {0};

	/* setup CoAP parser */
    CoapParser_setup(&parser, msg_ptr);
    /* read producer response payload + length */
    status = CoapParser_getPayload(&parser, &payload_ptr, &iEncryptedLength);

    /* if status is RC_OK then continue */
    if(RC_OK != status) {
#ifdef ENABLE_DEBUG
    	printf("CoAPClient: Error in CoAPClientResponseCallback\n\r");
#endif
    } else {
    	/* check incoming payload for correct syntax and start action depending on producer response */
    	if(strncmp(payload_ptr, "ContractAddress_", strlen("ContractAddress_")) == 0) {
#ifdef ENABLE_DEBUG
    		printf("CoAPClient server response: %s; Length: %i\n\r", payload_ptr, iEncryptedLength);
#endif
    		/* reset buffer before writing */
    		memset(ContractAddressBuffer, 0, sizeof(ContractAddressBuffer));
    		/* extract information out of producer response */
    		strncpy(ContractAddressBuffer, &payload_ptr[strlen("ContractAddress_")], sizeof(ContractAddressBuffer));
    		/* write public key into blockchain */
    		status = sendHttpDLTClientRequest(WRITE_PUBLIC_KEY, CONSUMER_ACCOUNT_ADDRESS, ContractAddressBuffer, PublicRSAKeyConsumer1024, strlen(PublicRSAKeyConsumer1024));
#ifdef ENABLE_DEBUG
    		if(RC_OK != status) {
    			printf("Error while writing public key into blockchain\n\r");
    		}
#endif
    	/* if consumer is already authenticated on consumer side then continue here */
    	} else if (strncmp(payload_ptr, "ConsumerAlreadyAuthenticated_", strlen("ConsumerAlreadyAuthenticated_")) == 0) {
    		memset(ContractAddressBuffer, 0, sizeof(ContractAddressBuffer));
    		strncpy(ContractAddressBuffer, &payload_ptr[strlen("ConsumerAlreadyAuthenticated_")], sizeof(ContractAddressBuffer));
    		/* no need to rewrite public key into blockchain
    		 * public key already stored on server side */
    		ConsumerAlreadyAuthenticatedFlag = true;

    	/* get encrypted data from producer */
    	} else if(strncmp(payload_ptr, "Data_", strlen("Data_")) == 0) {
#ifdef ENABLE_DEBUG
			printf("CoAPClient server response: %s; Length: %i\n\r", payload_ptr, iEncryptedLength);
#endif
			/* prepare encrypted data for queue - encrypted data always padded to ENCRYPTED_DATA_PAYLOAD_SIZE */
			queueHandlerCoAPClient.queuePayloadLength = ENCRYPTED_DATA_PAYLOAD_SIZE;
	    	memcpy(queueHandlerCoAPClient.queuePayload, &payload_ptr[strlen("Data_")], ENCRYPTED_DATA_PAYLOAD_SIZE);
	    	/* push prepared data into queue */
	    	queueResult = xQueueSend(dataQueue, &queueHandlerCoAPClient, 0);

	    	if(pdPASS == queueResult) {
#ifdef ENABLE_DEBUG
	    		printf("CoAPClient data pushed into dataQueue\n\r");
#endif
	    		status = RC_OK;
	    	}
    	} else {
#ifdef ENABLE_DEBUG
    		printf("CoAPClient receive data: %s, Length: %i\n\r", payload_ptr, iEncryptedLength);
#endif
    	}
    }

    return status;
}

/**
 * This function is called after a CoAP client request
 * is sent successful
 *
 * @param[in] callable_ptr
 * This is the reference to the callable element.
 *
 * @param[in] status
 * This is the response status of the callback
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPClientSendingCallback(Callable_T *callable_ptr, retcode_t status)
{
	/* surpress warning message concerning unused variable */
    (void) callable_ptr;

    return status;
}

/**
 * This function is called to serialize CoAP requests
 *
 * @param[in] msg_ptr
 * This holds the current message request
 *
 * @param[in] requestCode
 * This holds the current client request code
 *
 * @param[in] uriOptionValue_ptr
 * This reference holds the uriOption which should
 * be serialized
 *
 * @param[in] uriOptionLen
 * This variable holds the length of the uriOption
 * which should be serialized
 *
 * @param[in] payload_ptr
 * This buffer holds the current payload to serialize
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPClientSerializeRequest(Msg_T *msg_ptr, uint8_t requestCode, uint8_t* const uriOptionValue_ptr, size_t uriOptionLen, uint8_t const *payload_ptr)
{
	retcode_t ret = RC_SERVAL_ERROR;
	CoapSerializer_T serializer;
	uint8_t resource[QUEUE_BUFF_SIZE] = {0};
	uint16_t resourceLength = 0;
	CoapOption_T uriOption = {0};
	CoapOption_T formatOption = {0};
	uint16_t formatValue = 0;

	if(NULL != uriOptionValue_ptr) {
		if(payload_ptr == NULL) {
			/* set default payload that lower layers
			 * can handle the request */
			payload_ptr = "CAFFE";
		}
		/* setup CoAP serializer */
		ret = CoapSerializer_setup(&serializer, msg_ptr, REQUEST);
		ret = CoapSerializer_setCode(&serializer, msg_ptr, requestCode);

		/* serialize the URI path of the requested ressource */
		uriOption.OptionNumber = Coap_Options[COAP_URI_PATH];
		uriOption.value = uriOptionValue_ptr;
		uriOption.length = uriOptionLen;
		ret = CoapSerializer_serializeOption(&serializer, msg_ptr, &uriOption);

		/* serialize the option content format.
		 * Here it is equal to TEXT_PLAINCHARSET_UTF8 */
		formatOption.OptionNumber = Coap_Options[COAP_CONTENT_FORMAT];
		CoapSerializer_setUint16(&formatOption, Coap_ContentFormat[TEXT_PLAINCHARSET_UTF8], &formatValue);
		CoapSerializer_serializeOption(&serializer, msg_ptr, &formatOption);

		/* we do not require a confirmation so we set input as false
		 * --> Non-confirmables are faster but they are not guaranteed to arrive
		 * 	   at the destination
		 */
		CoapSerializer_setConfirmable(msg_ptr, false);

		/* tokens are used to indicate to which request a response relates to
		 * --> NULL means that no token will be created
		 * --> If a token is inserted into the request it will also appear in the response
		 */
		ret = CoapSerializer_serializeToken(&serializer, msg_ptr, NULL, 0);

		/* CoapSerializer_serializeOption(&serializer, msg_ptr, &CoapOption);
		 * call this after all options have been serialized (in this case: none)
		 */
		ret = CoapSerializer_setEndOfOptions(&serializer, msg_ptr);

		/* serialize the payload */
		resourceLength = strlen(payload_ptr);
		/* copy payload into resource buffer */
		memcpy(resource, payload_ptr, resourceLength + 1);
		/* start CoAP serialization */
		ret = CoapSerializer_serializePayload(&serializer, msg_ptr, resource, resourceLength);
	}

    return ret;
}

/**
 * This function is called to send the CoAP request
 *
 * @param[in] addr_ptr
 * This holds the current CoAP server ip address
 *
 * @param[in] port
 * This holds the CoAP port
 *
 * @param[in] uriOptionValue_ptr
 * This reference holds the uriOption which should
 * be serialized
 *
 * @param[in] uriOptionLen
 * This variable holds the length of the uriOption
 * which should be serialized
 *
 * @param[in] payload_ptr
 * This buffer holds the current payload to send
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPClientSendCoAPClientRequest(Ip_Address_T *addr_ptr, Ip_Port_T port, uint8_t* const uriOptionValue_ptr, size_t uriOptionLen, uint8_t const *payload_ptr)
{
	retcode_t ret = RC_SERVAL_ERROR;
	Msg_T *msg_ptr = NULL;

	/* check incoming parameters */
	if( (NULL != uriOptionValue_ptr) && (0 != port) ) {
		/* init request */
		ret = CoapClient_initReqMsg(addr_ptr, port, &msg_ptr);
		if(RC_OK == ret) {
			/* serialize request */
			CoAPClientSerializeRequest(msg_ptr, Coap_Codes[COAP_POST], uriOptionValue_ptr, uriOptionLen, payload_ptr);
			/* set callback */
			Callable_T *alpCallable_ptr = Msg_defineCallback(msg_ptr, (CallableFunc_T) CoAPClientSendingCallback);
			/* push request */
			ret = CoapClient_request(msg_ptr, alpCallable_ptr, &CoAPClientResponseCallback);
#ifdef ENABLE_DEBUG
			if(RC_OK != ret) {
				printf("CoAPClient: Error in sendClientRequest\n\r");
			}
#endif
		}
	}

	return ret;
}

/**
 * This function is called to initialize the CoAP client.
 * It will set the CoAP server ip address and CoAP port.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T CoAPClientInit(void)
{
	Retcode_T ret = RETCODE_FAILURE;

	/* basic init functions are similar for CoAP client and Server */
    ret = CoapServer_initialize();

    /* convert defined CoAP port */
    CoAPIpHandleVar.serverPort = Ip_convertIntToPort((uint16_t)COAP_PORT);

    /* replace the IP-string with your target’s IP */
    ret = Ip_convertStringToAddr(COAP_SERVER_IP, &CoAPIpHandleVar.ip);

   	/* NULL is used here because we are not using the instance as a server */
   	ret = CoapServer_startInstance(CoAPIpHandleVar.serverPort, NULL);
#ifdef ENABLE_DEBUG
   	if(RETCODE_SUCCESS == ret) {
    	printf("CoAPClient: Coap initialization: OK\n\r");
	} else {
		printf("CoAPClient: Coap initialization: FAILED\n\r");
	}
#endif

    return ret;
}

/**
 * This function is a cyclic task which will send
 * a CoAP data request every x seconds to a specified
 * ip and port
 */
void CoAPClientCyclic(void* pvParameters)
{
	bool retTransConfirmed = false;
	(void) pvParameters;
	/* define CoAP request options */
	char const *caOption_ptr = "ContractAddress";
	char const *pkOption_ptr = "PublicKeyAvailable";
	char const *dataOption_ptr = "Data";

    for(;;)
    {
    	/* check if button is pressed and use correct counter value to order the requests */

    	/* first step */
    	if( (true == Button1Pressed()) && (counterForUserInteraction == 0) ) {
    		/* send contract account address request */
			CoAPClientSendCoAPClientRequest(&CoAPIpHandleVar.ip, CoAPIpHandleVar.serverPort, caOption_ptr, strlen(caOption_ptr), CONSUMER_ACCOUNT_ADDRESS);
			counterForUserInteraction +=1;
			/* prevent accidental butto push */
			vTaskDelay(SECONDS(2));

		/* second step */
    	} else if( (true == Button1Pressed()) && (counterForUserInteraction == 1)) {
    		/* if consumer is already authenticated by producer there is no need to write pubkey again */
			if(false == ConsumerAlreadyAuthenticatedFlag) {
				/* send getTransactionReceipt request to blockchain to get transaction confirmation */
				retTransConfirmed = WaitForTransactionConfirmation();
				/* if transaction is confirmed, send public key available request */
				if(true == retTransConfirmed) {
					CoAPClientSendCoAPClientRequest(&CoAPIpHandleVar.ip, CoAPIpHandleVar.serverPort, pkOption_ptr, strlen(pkOption_ptr), NULL);
					counterForUserInteraction +=1;
				}
			} else {
				counterForUserInteraction +=1;
			}
			vTaskDelay(SECONDS(2));

		/* step three */
    	} else if( (true == Button1Pressed()) && (counterForUserInteraction == 2)) {
    		/* send data request */
			CoAPClientSendCoAPClientRequest(&CoAPIpHandleVar.ip, CoAPIpHandleVar.serverPort, dataOption_ptr, strlen(dataOption_ptr), NULL);
    		counterForUserInteraction = 0;
    		vTaskDelay(SECONDS(2));
    	}
    }
}

/**
 * This cyclic function is used to process the
 * received sensor data. It waits until
 * data is available in the queue and
 * starts the data processing on
 * consumer side.
 *
 * @param[in] pvParameters (unused)
 *
 * @return
 * void
 */
void processSensorPayloadDataCyclic(void* pvParameters)
{
	(void) pvParameters;
	Retcode_T ret = RETCODE_FAILURE;
	BaseType_t queueResult = pdFAIL;
	uint8_t decryptedData = 0;
	size_t outputLength = 0;
	uint8_t dataHashBuffer[DATA_HASH_BUFF_SIZE] = {0};
	queueHandler_T queueHandlerCoAPClient = {0};

	for(;;) {
		/* wait until data is available in the queue */
		queueResult = xQueueReceive(dataQueue, &queueHandlerCoAPClient, SECONDS(5));

		if(pdPASS == queueResult) {
#ifdef ENABLE_DEBUG
			printf("CoAPClient: Queue data received: %s, Length: %i\n\r", queueHandlerCoAPClient.queuePayload, queueHandlerCoAPClient.queuePayloadLength);
#endif
			/* reset local variables */
			memset(dataHashBuffer, 0, sizeof(dataHashBuffer));
			decryptedData = 0;
			outputLength = 0;

			/* get data hash from blockchain */
			ret = sendHttpDLTClientRequest(READ_DATA_HASH, CONSUMER_ACCOUNT_ADDRESS, ContractAddressBuffer, NULL, 0);

			if(RETCODE_SUCCESS == ret) {
				/* wait until http request is finished */
				ret = WaitForHttpReceiveCallback();
			}
			/* decrypt data */
			if(RETCODE_SUCCESS == ret) {
				ret = decryptData(queueHandlerCoAPClient.queuePayload, queueHandlerCoAPClient.queuePayloadLength, &decryptedData, sizeof(decryptedData), &outputLength );
				/* switch LEDs dependent on accel value */
				if(decryptedData >= ACCELEROMETER_VALUE_THRESHHOLD) {
					BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_ON);
					BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_OFF);
				} else {
					BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_ON);
					BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_OFF);
				}
			}
			/* calculate data hash of decrypted data and compare with blockchain value */
			if(RETCODE_SUCCESS == ret) {
					ret = CalculateHash(queueHandlerCoAPClient.queuePayload, queueHandlerCoAPClient.queuePayloadLength, dataHashBuffer, sizeof(dataHashBuffer));
					/* compare read hash value from blockchain with calculated value */
					ret = memcmp(SEEDConsumerDataHashBuffer, dataHashBuffer, DATA_HASH_BUFF_SIZE);

				if(RETCODE_SUCCESS == ret) {
					printf("Data successfully verified\n\r");
					vTaskDelay(SECONDS(2));
#ifdef ENABLE_DEBUG
					printf("Send positive vote\n\r");
#endif
					/* vote positive */
					ret = sendHttpDLTClientRequest(RATE_PRODUCER_POSITIVE, CONSUMER_ACCOUNT_ADDRESS, ContractAddressBuffer, NULL, 0);
					/* here we could also wait for transaction confirmation */
				} else {
					printf("Data hashes not equal: %s\n%s\n\r", SEEDConsumerDataHashBuffer, dataHashBuffer);
					vTaskDelay(SECONDS(2));
#ifdef ENABLE_DEBUG
					printf("Send negative vote\n\r");
#endif
					/* vote negative */
					ret = sendHttpDLTClientRequest(RATE_PRODUCER_NEGATIVE, CONSUMER_ACCOUNT_ADDRESS, ContractAddressBuffer, NULL, 0);
					/* here we could also wait for transaction confirmation */
				}
			}
			if(RETCODE_SUCCESS != ret) {
				/* if something went wrong during data processing then
				 * reset counter so the next request will be again the
				 * first one which asks for contract address */
				counterForUserInteraction = 0;
#ifdef ENABLE_DEBUG
					printf("Something went wrong in processSensorPayloadDataCyclic\n\r");
#endif
			}
			/* reset queueHandlerCoAPClient struct */
			memset(&queueHandlerCoAPClient, 0, sizeof(queueHandlerCoAPClient));
		} else {
			//do nothing
		}
	}
}
