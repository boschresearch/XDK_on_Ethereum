/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

/* system header files */
#include <string.h>

#include "FreeRTOS.h"
#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "PAL_initialize_ih.h"
#include "PAL_socketMonitor_ih.h"
#include "Serval_Coap.h"
#include "Serval_CoapServer.h"
#include "Serval_CoapClient.h"
#include "Serval_Network.h"
#include "queue.h"
#include "BCDS_BSP_LED.h"
#include "BSP_BoardType.h"

/* user includes */
#include "UserConfig.h"
#include "SystemConfig.h"
#include "Encryption.h"
#include "CoAPServer.h"
#include "Http.h"
#include "SensorData.h"
#include "SecureEdgeDevice.h"

/* ethereum account information */
#define ENCRYPTED_BUFF_SIZE			256

/* callback buff sizes */
#define CLIENT_OPTION_BUFF_SIZE		56
#define CALLBACK_RESPONSE_BUFF_SIZE	256

/* state machine enum */
typedef enum producerDataProcessingState {
	DATA_PROCESSING_START = 0,
	DATA_PROCESSING_INIT = 1,
	DATA_PROCESSING_READ_PUB_KEY_DLT = 2,
	DATA_PROCESSING_READ_SENS_DATA = 3,
	DATA_PROCESSING_ENCRYPT = 4,
	DATA_PROCESSING_CALC_HASH = 5,
	DATA_PROCESSING_WRITE_HASH_DLT = 6,
	DATA_PROCESSING_PUSH_QUEUE_DATA = 7,
	DATA_PROCESSING_SUCCESSFUL = 8,
	DATA_PROCESSING_FAILED = 0xFF
} producerDataProcessingState_T;

/* local data processing variable declarations */
static producerDataProcessingState_T DataProcessingState = DATA_PROCESSING_FAILED;
/* flag to check if consumer is already authenticated */
static bool ConsumerAuthenticated = false;

/* authentication table definition - stores consumer account+pubKey information */
AuthConsumer_T AuthenticatedConsumerTable[CONSUMER_NUMBER_MAX] = { 0 };

/**
 * This function is called to convert capital letters of
 * an incoming ethereum address to lower case letters
 * because the ethereum network is case insensitive.
 *
 * @param[in] stringToConvert_ptr
 * Pointer to the string which should be converted
 * to lower case
 *
 * @param[in] iStringSize
 * This variable holds the size of the incoming
 * string.
 *
 * @param[out] oBuff
 * This buffer will hold the converted input string
 * with only lower case letters
 *
 * @return
 * void
 */
void convertUppercaseToLowercase(uint8_t *stringToConvert_ptr, size_t iStringSize, uint8_t *oBuff)
{
	uint8_t tempBuff[CONTRACT_ADDRESS_LENGTH] = {0};

	/* check for null pointer */
	if(NULL != stringToConvert_ptr) {
		/* convert numbers from ascii format to int value (atoi
		 * does not support hex numbers larger then 9) */
		for(size_t i = 0; i < iStringSize; ++i) {
			if( ('A' <= stringToConvert_ptr[i]) && ('F' >= stringToConvert_ptr[i]) ) {
				tempBuff[i] = stringToConvert_ptr[i] + 32;
			} else {
				/* keep numbers and lowercase letters */
				tempBuff[i] = stringToConvert_ptr[i];
			}
		}
		strncpy(oBuff, tempBuff, CONTRACT_ADDRESS_LENGTH);
	}
}

/**
 * This function is called to parse an incoming
 * CoAP request
 *
 * @param[out] msg_ptr
 * This reference will hold the current CoAP message context
 *
 * @param[out] code_ptr
 * This reference will hold the parsed request code type
 *
 * @param[out] optionBuff
 * This buffer will hold the parsed request option
 *
 * @param[in] optionBuffLen
 * This variable holds the size of optionBuff
 *
 * @param[out] payload_pptr
 * This buffer will hold the parsed payload data
 * of the request
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPServerParseCoAPRequest(Msg_T *msg_ptr, uint8_t *code_ptr, uint8_t* optionBuff, size_t optionBuffLen, uint8_t const **payload_pptr)
{
	CoapParser_T parser;
	CoapPayloadLength_t payloadLength = 0;
    CoapOption_T option;
	retcode_t ret = RC_SERVAL_ERROR;

	/* setup CoAP parser */
	CoapParser_setup(&parser, msg_ptr);

	/* read code from CoAP message */
	*code_ptr = CoapParser_getCode(msg_ptr);
	/* read option from CoAP message */
    CoapParser_getOption(&parser, msg_ptr, &option, Coap_Options[COAP_URI_PATH]);
    /* check option length */
    if(optionBuffLen >= option.length) {
    	 strncpy(optionBuff, option.value, option.length);
    } else {
    	ret = RC_COAP_CLIENT_REQ_ERROR;
    	return ret;
    }

    /* read payload */
	ret = CoapParser_getPayload(&parser, payload_pptr, &payloadLength);

#ifdef ENABLE_DEBUG
	if(RC_OK == ret) {
		printf("CoAPServer: Incoming CoAP request:Option %s, Payload: %s \n\r", option.value,*payload_pptr);
	}
	else {
		printf("CoAPServer: Error in CoAPServerParseCoAPRequest\n\r");
	}
#endif

    return ret;
}

/**
 * This function is called to create a CoAP response
 * for the incoming CoAP request. It is used to setup
 * and serialize the response message.
 *
 * @param[in] msg_ptr
 * This holds the current CoAP message context.
 *
 * @param[in] payload_ptr
 * This buffer holds the actual reponse payload
 *
 * @param[in] payloadLength
 * This variable holds the payload length
 *
 * @param[in] responseCode
 * This variable holds the code which represents
 * the type of CoAP message e.g. content-type
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
retcode_t CoAPServerCreateCoAPResponse(Msg_T *msg_ptr, const uint8_t *payload_ptr, size_t payloadLength, uint8_t responseCode)
{
	retcode_t ret = RC_MAX_APP_ERROR;
	CoapSerializer_T serializer;
	uint8_t resource[QUEUE_BUFF_SIZE] = {0};
	uint16_t resourceLength = 0;

	ret = CoapSerializer_setup(&serializer, msg_ptr, RESPONSE);
	ret = CoapSerializer_setCode(&serializer, msg_ptr, responseCode);
	/* we do not require a confirmation so we set input as false
	 * --> Non-confirmables are faster but they are not guaranteed to arrive
	 * 	   at the destination
	*/
	CoapSerializer_setConfirmable(msg_ptr, false);

	/* re-use the same token in the response which was used for the request	*/
    ret = CoapSerializer_reuseToken(&serializer, msg_ptr);

    /* call this after all options have been serialized (in this case: none) */
    ret = CoapSerializer_setEndOfOptions(&serializer, msg_ptr);

    /* strlen terminates on \0 so i have to give length as parameter payloadLength */
	resourceLength = payloadLength;

    /* copy payload into resource buffer */
    memcpy(resource, payload_ptr, resourceLength + 1);

    /* start payload serialization */
    ret = CoapSerializer_serializePayload(&serializer, msg_ptr, resource, resourceLength);

#ifdef ENABLE_DEBUG
    if(RC_OK != ret) {
    	printf("CoAPServer: Error in CoAPServerCreateCoAPResponse\n\r");
    }
#endif

    return ret;
}

/**
 * This callback function is invoked after the
 * message is sent to the client
 *
 * @param[in] callable_ptr
 * This reference holds the Callable_t object
 *
 * @param[out] status
 * This variable holds the status of the callback
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
static retcode_t CoAPServerSendingCallback(Callable_T *callable_ptr, retcode_t status)
{
	/* surpress warning message concerning unused variable */
    (void) callable_ptr;
    (void) status;

    return RC_OK;
}

/**
 * This function is called to send a CoAP response
 * from CoAP server to an incoming CoAP client request.
 * It sets up the response, serializes the message,
 * defines the Callback and finally sends the respond.
 *
 * @param[in] msg_ptr
 * This holds the current CoAP message context.
 *
 * @param[in] payload_ptr
 * This buffer holds the current reponse payload
 *
 * @param[in] payloadLength
 * This variable holds the payload length
 */
void CoAPServerSendCoAPResponse(Msg_T *msg_ptr, uint8_t const *payload_ptr, size_t payloadLength)
{
	CoAPServerCreateCoAPResponse(msg_ptr, payload_ptr, payloadLength, Coap_Codes[COAP_CONTENT]);
	Callable_T *alpCallable_ptr = Msg_defineCallback(msg_ptr, (CallableFunc_T) CoAPServerSendingCallback);

	CoapServer_respond(msg_ptr, alpCallable_ptr);
}

/**
 * This function is called to create a CoAP response
 * for the incoming CoAP request. It is used to setup
 * and serialize the response message.
 *
 * @param[out] msg_ptr
 * This reference will hold the current CoAP message context.
 * It will be parsed in from the incoming CoAP request here.
 *
 * @param[out] status
 * This variable holds the current message processing status
 *
 * @return
 * RC_OK, if successful<br>
 * RC_SERVAL_ERROR, otherwise.
 */
static retcode_t CoAPServerReceiveCallback(Msg_T *msg_ptr, retcode_t status)
{
    uint8_t coAPRetCode = 0;
    uint8_t const *ClientPayload = NULL;
    uint8_t const ClientOptionBuffer[CLIENT_OPTION_BUFF_SIZE] = {0};
    uint8_t responseBuffer[CALLBACK_RESPONSE_BUFF_SIZE]  = {0};
    BaseType_t queueResult = pdFAIL;
    queueHandler_T queueHandlerCoAP = {0};
    uint8_t consumerEthAccountBuffer[CONTRACT_ADDRESS_LENGTH] = {0};
    uint8_t consumerEthAccountBufferTemp[CONTRACT_ADDRESS_LENGTH] = {0};
    uint8_t contractAddressBuffer[CONTRACT_ADDRESS_LENGTH * 2] = {0};

    /* parse the incoming consumer request */
    status = CoAPServerParseCoAPRequest(msg_ptr, &coAPRetCode, ClientOptionBuffer, sizeof(ClientOptionBuffer), &ClientPayload);

    /* if status OK and post request received then continue */
    if( (RC_OK == status) && (Coap_Codes[COAP_POST] == coAPRetCode) ) {

    	/* 1. Store consumer information in authentication table */
    	if(strncmp(ClientOptionBuffer, "ContractAddress", strlen("ContractAddress")) == 0) {
#ifdef ENABLE_DEBUG
			printf("ContractAddress request received\n\r");
#endif
			/* reset buffers */
			memset(consumerEthAccountBuffer, 0, sizeof(consumerEthAccountBuffer));
			memset(consumerEthAccountBufferTemp, 0, sizeof(consumerEthAccountBufferTemp));
			strncpy(consumerEthAccountBufferTemp, ClientPayload, sizeof(consumerEthAccountBufferTemp));

			/* convert upper case letters to lower case letters
			 * because address is stored in lower case letters
			 * on the ethereum blockchain */
			convertUppercaseToLowercase(consumerEthAccountBufferTemp, sizeof(consumerEthAccountBufferTemp), consumerEthAccountBuffer);

			/* check if incoming account address is already authenticated */
			for(uint8_t counter = 0; counter < CONSUMER_NUMBER_MAX; ++counter) {
				if(strncmp(AuthenticatedConsumerTable[counter].accountAddress, consumerEthAccountBuffer, CONTRACT_ADDRESS_LENGTH) == 0) {
					ConsumerAuthenticated = true;
					/* set active consumer if more then one is stored in authentication table */
					AuthenticatedConsumerTable[counter].activeConsumer = true;
					/* send CoAP answer */
					strncpy(contractAddressBuffer, "ConsumerAlreadyAuthenticated_", strlen("ConsumerAlreadyAuthenticated_"));
					strncat(contractAddressBuffer, CONTRACT_ADDRESS, strlen(CONTRACT_ADDRESS));
					CoAPServerSendCoAPResponse(msg_ptr, contractAddressBuffer, strlen(contractAddressBuffer));
					DataProcessingState = DATA_PROCESSING_START;
				} else {
					AuthenticatedConsumerTable[counter].activeConsumer = false;
				}
			}
			if(true != ConsumerAuthenticated) {
				/* send contract address */
				strncpy(contractAddressBuffer, "ContractAddress_", strlen("ContractAddress_"));
				strncat(contractAddressBuffer, CONTRACT_ADDRESS, strlen(CONTRACT_ADDRESS));
				CoAPServerSendCoAPResponse(msg_ptr, contractAddressBuffer, strlen(contractAddressBuffer));
			}

			status = RC_OK;

		/* 2. Prepare sensor raw data - encrypt + write data hash to blockchain */
    	} else if(strncmp(ClientOptionBuffer, "PublicKeyAvailable", strlen("PublicKeyAvailable")) == 0) {
#ifdef ENABLE_DEBUG
    		printf("PublicKeyAvailable request received\n\r");
#endif
    		/* set authenticated flag to false, so a consumer can reset his public key */
    		ConsumerAuthenticated = false;

    		/* start state machine */
    		DataProcessingState = DATA_PROCESSING_START;
    		CoAPServerSendCoAPResponse(msg_ptr, "Prepare payload data", strlen("Prepare payload data"));
    		status = RC_OK;

    	/* 3. Send encrypted data to consumer */
    	} else if(strncmp(ClientOptionBuffer, "Data", strlen("Data")) == 0) {
#ifdef ENABLE_DEBUG
    		printf("Data request received\n\r");
#endif
    		/* if a data request is received from consumer then send response based on state machine state */
    		switch(DataProcessingState) {
    			case DATA_PROCESSING_START:
    				CoAPServerSendCoAPResponse(msg_ptr, "Data processing started", strlen("Data processing started"));
    			break;
    			case DATA_PROCESSING_SUCCESSFUL:
    				/* reset state variable */
    				DataProcessingState = DATA_PROCESSING_FAILED;
					/* reset queue type */
					memset(&queueHandlerCoAP, 0, sizeof(queueHandlerCoAP));
					/* pull encrypted data out of dataQueue */
					queueResult = xQueueReceive(dataQueue, &queueHandlerCoAP, SECONDS(20));

					if(pdPASS == queueResult) {
#ifdef ENABLE_DEBUG
						printf("QUEUE coap server received data: %s; Length: %i\n\r", queueHandlerCoAP.queuePayload, queueHandlerCoAP.queuePayloadLength);
#endif
						/* copy encrypted data into response buffer */
						strcpy(responseBuffer, "Data_");
						memcpy(&responseBuffer[strlen("Data_")], queueHandlerCoAP.queuePayload, queueHandlerCoAP.queuePayloadLength);
						printf("Response buffer: %s; Length: %i\n\r", responseBuffer, strlen(responseBuffer));
						/* send encrypted data from producer to consumer */
						CoAPServerSendCoAPResponse(msg_ptr, responseBuffer, strlen("Data_") + queueHandlerCoAP.queuePayloadLength);
						status = RC_OK;
					}
				break;
    			case DATA_PROCESSING_FAILED:
    				CoAPServerSendCoAPResponse(msg_ptr, "Data processing failed, start new request", strlen("Data processing failed, start new request"));
    			break;
    			default:
    				CoAPServerSendCoAPResponse(msg_ptr, "Data processing in progress", strlen("Data processing in progress"));
    			break;
			}
    	} else {
    		CoAPServerSendCoAPResponse(msg_ptr, "Option not supported", strlen("Option not supported"));
    	}
    } else {
    	CoAPServerSendCoAPResponse(msg_ptr, "Error: Wrong request code, only POST supported", strlen("Error: Wrong request code, only POST supported"));
    	status = RC_SERVAL_ERROR;
    }

    return status;
}

/**
 * This function initializes the basic CoAP
 * server functionality. It sets up CoAP port and
 * prints the server Ip address which must be used
 * by the client to send requests.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T CoAPServerInit(void)
{
	NetworkConfig_IpSettings_T ServerIp; /* my IP address */
	Retcode_T ret = RETCODE_FAILURE;

    ret = CoapServer_initialize();

    Ip_Port_T serverPort = Ip_convertIntToPort((uint16_t)COAP_PORT);

    /* start CoAP server - callback is required */
	ret = CoapServer_startInstance(serverPort, (CoapAppReqCallback_T) &CoAPServerReceiveCallback);
	if(RETCODE_SUCCESS == ret) {
		NetworkConfig_GetIpSettings(&ServerIp);
#ifdef ENABLE_DEBUG
		printf("CoAPServer: The IP was retrieved: %u.%u.%u.%u \n\r",
				(unsigned int) (NetworkConfig_Ipv4Byte(ServerIp.ipV4, 3)),
		        (unsigned int) (NetworkConfig_Ipv4Byte(ServerIp.ipV4, 2)),
		        (unsigned int) (NetworkConfig_Ipv4Byte(ServerIp.ipV4, 1)),
		        (unsigned int) (NetworkConfig_Ipv4Byte(ServerIp.ipV4, 0)));
#endif
	}

#ifdef ENABLE_DEBUG
    if(RETCODE_SUCCESS != ret) {
    	printf("CoAPServer: Error in CoAPServerInit\n\r");
    }
#endif

    return ret;
}

/**
 * This cyclic function is used to prepare the
 * sensor data. It goes trough different states
 * to wait until consumer public key is available
 * in the blockchain and starts the data processing
 * on producer side.
 *
 * @param[in] pvParameters (unused)
 *
 * @return
 * void
 */
void prepareSensorPayloadData(void* pvParameters)
{
	(void) pvParameters;
	Retcode_T ret = RETCODE_FAILURE;
	/* previously used sensor data
	 * uint32_t humiditySensorRawData = 0;
	 * uint8_t humiditySensorData = 0;
	*/
	uint8_t accelerometerSensorData = 0;
	uint8_t dataHashBuff[DATA_HASH_BUFF_SIZE] = {0};
	uint8_t EncryptedBuffLocal[ENCRYPTED_BUFF_SIZE] = {0};
	size_t oLength = 0;
    BaseType_t queueResult = pdFAIL;
    queueHandler_T queueHandlerDataServer = {0};
    bool retTransConfirmed = false;

	for(;;) {
		/* check state machine for state changes */
		switch(DataProcessingState) {
			case DATA_PROCESSING_START:
#ifdef ENABLE_DEBUG
				printf("Prepare sensor payload data\n\r");
#endif
				DataProcessingState = DATA_PROCESSING_INIT;
			break;

			case DATA_PROCESSING_INIT:
				/* reset buffers */
				memset(dataHashBuff, 0, sizeof(dataHashBuff));
				memset(EncryptedBuffLocal, 0, sizeof(EncryptedBuffLocal));
				memset(&queueHandlerDataServer, 0, sizeof(queueHandlerDataServer));
				accelerometerSensorData = 0;
				retTransConfirmed = false;
				/* previously used sensor data
				 * humiditySensorRawData = 0;
				 * humiditySensorData = 0;
				*/

				DataProcessingState = DATA_PROCESSING_READ_PUB_KEY_DLT;
			break;

			case DATA_PROCESSING_READ_PUB_KEY_DLT:
				/* if consumer is not in authentication list */
				if(true != ConsumerAuthenticated) {
					/* get public key from blockchain for encryption */
					sendHttpDLTClientRequest(READ_PUBLIC_KEY, PRODUCER_ACCOUNT_ADDRESS, CONTRACT_ADDRESS, NULL, 0);
					/* wait x seconds until http response received */
					ret = WaitForHttpReceiveCallback();
					/* store latest consumer data always at authentication table position 0 */
					AuthenticatedConsumerTable[0].activeConsumer = true;
				} else {
					ret = RETCODE_SUCCESS;
				}
				/* reset authentication flag */
				ConsumerAuthenticated = false;

				if(RETCODE_SUCCESS == ret) {
					DataProcessingState = DATA_PROCESSING_READ_SENS_DATA;
				} else {
					DataProcessingState = DATA_PROCESSING_FAILED;
				}
			break;

			case DATA_PROCESSING_READ_SENS_DATA:
				/* read sensor values */
				/* ret = GetEnvironmentalSensorData(&humiditySensorRawData); */
				/* calculate data hash */
				/* humiditySensorData = (humiditySensorRawData & 0x000000ff); */
				accelerometerSensorData = GetAccelerometerSensorData();
#ifdef ENABLE_DEBUG
				printf("Accelerometer sensor data: %i\n\r", accelerometerSensorData);
#endif
				DataProcessingState = DATA_PROCESSING_ENCRYPT;
			break;

			case DATA_PROCESSING_ENCRYPT:
				/* start data encryption */
				ret = encryptData(&accelerometerSensorData, sizeof(accelerometerSensorData), EncryptedBuffLocal, sizeof(EncryptedBuffLocal), &oLength);

				if(RETCODE_SUCCESS == ret) {
					/* prepare queue data */
					memcpy(queueHandlerDataServer.queuePayload, EncryptedBuffLocal, oLength);
					queueHandlerDataServer.queuePayloadLength = oLength;
#ifdef ENABLE_DEBUG
					printf("dataQueue after encryption: %s; Length: %i\n\r", queueHandlerDataServer.queuePayload, queueHandlerDataServer.queuePayloadLength);
#endif
					DataProcessingState = DATA_PROCESSING_CALC_HASH;
				} else {
					DataProcessingState = DATA_PROCESSING_FAILED;
				}
			break;

			case DATA_PROCESSING_CALC_HASH:
				/* calaculate data hash */
				ret = CalculateHash(EncryptedBuffLocal, oLength, dataHashBuff, sizeof(dataHashBuff));

				if(RETCODE_SUCCESS == ret) {
					DataProcessingState = DATA_PROCESSING_WRITE_HASH_DLT;
				} else {
					DataProcessingState = DATA_PROCESSING_FAILED;
				}
			break;

			case DATA_PROCESSING_WRITE_HASH_DLT:
				/* write data hash into blockchain */
				ret = sendHttpDLTClientRequest(WRITE_DATA_HASH, PRODUCER_ACCOUNT_ADDRESS, CONTRACT_ADDRESS, dataHashBuff, sizeof(dataHashBuff));

				if(RETCODE_SUCCESS == ret) {
					/* wait until http callback received */
					ret = WaitForHttpReceiveCallback();
					if(RETCODE_SUCCESS == ret) {
						/* wait until transaction is confirmed */
						retTransConfirmed = WaitForTransactionConfirmation();
					}
				}

				if(true == retTransConfirmed) {
					DataProcessingState = DATA_PROCESSING_PUSH_QUEUE_DATA;
				} else {
					DataProcessingState = DATA_PROCESSING_FAILED;
				}
			break;

			case DATA_PROCESSING_PUSH_QUEUE_DATA:
				/* reset queue in case something went wrong and old data is still available in queue */
				xQueueReset(dataQueue);
				/* push encrypted data into data queue */
				queueResult = xQueueSend(dataQueue, (void*)&queueHandlerDataServer, SECONDS(2));
				if(pdPASS == queueResult) {
#ifdef ENABLE_DEBUG
					printf("Encrypted sensor data pushed into dataQueue and data hash written into blockchain\n\r");
#endif
					DataProcessingState = DATA_PROCESSING_SUCCESSFUL;
				} else {
					/* set state variable */
					DataProcessingState = DATA_PROCESSING_FAILED;
				}
			break;

			case DATA_PROCESSING_FAILED:
				/* if state is failed then sleep so CPU is free */
				vTaskDelay(SECONDS(1));
			break;
		}
	}
}
