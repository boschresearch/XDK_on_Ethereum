/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

/* system includes */
#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "PAL_initialize_ih.h"
#include "PAL_socketMonitor_ih.h"
#include "Serval_HttpClient.h"
#include "Serval_Network.h"
#include "PIp.h"

/* user includes */
#include "Http.h"
#include "UserConfig.h"
#include "SystemConfig.h"
#include "Encryption.h"
#include "cJSON.h"
#include "CoAPServer.h"

/* post command is used to invoke blockchain functions via json-rpc */
#define DESTINATION_POST_PATH "/post"

/* Example of how a json string is put together */
/**
 * JSON string contains the following hex encoded data information
 * hex representation of data offset - here: 0x20 = 32 Bytes
 * hex representation of Payload size - here: 0x20 = 32 Bytes
 * hex representation of Payload - always padded to 32 byte blocks (left aligned)
 *
 * message format for dynamic input parameter data like bytes, string, array:
 *    4 bytes - first four bytes of sha256 function hash (web3.sha3("WritePublicKey(bytes)") to generate function hash)
 * + 32 bytes - offset to the start of payload data (e.g. 0000000000000000000000000000000000000000000000000000000000000020 --> 32 byte offset)
 * + 32 bytes - number of payload data in bytes (e.g. 0000000000000000000000000000000000000000000000000000000000000020 --> 32 bytes payload)
 * + xx bytes - payload right padded (e.g. 2121000000000000000000000000000000000000000000000000000000000000)
 * */
/* function headers hard coded until parameter values */
#define READ_DATA_HASH_HEADER   		"0x0546bedb00000000000000000000000000000000000000000000000000000000"
										/* Func hash															 Offset												Payload length*/
#define WRITE_DATA_HASH_HEADER  		"0x0ef8126900000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000020"
#define WRITE_DATA_HASH_HEADER_LEN  	strlen(WRITE_DATA_HASH_HEADER)

#define READ_PUBLIC_KEY_HEADER  		"0xfcc01d5100000000000000000000000000000000000000000000000000000000"
										/* Func hash															 Offset												Payload length*/
#define WRITE_PUBLIC_KEY_HEADER 		"0x2ea8dff500000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000110"
#define WRITE_PUBLIC_KEY_HEADER_LEN 	strlen(WRITE_PUBLIC_KEY_HEADER)

#define RATE_PRODUCER_POSITIVE_DATA		"0xf18aeab60000000000000000000000000000000000000000000000000000000000000001"
#define RATE_PRODUCER_NEGATIVE_DATA		"0xf18aeab60000000000000000000000000000000000000000000000000000000000000000"
#define RATE_PRODUCER_HEADER_LEN		strlen(RATE_PRODUCER_POSITIVE_DATA)

#define TRANSACTION_EXECUTION_GAS 		"0x47E7C0"
#define JSON_RPC_VERSION 				"2.0"
#define DATA_ETHER_EXCHANGE_RATE		"0x1BC16D674EC80000" /* 2 ether in wei */

#define JSON_STRING_BUFF_SIZE 							2048

#define READ_DATA_HASH_JSON_RESULT_OFFSET				130
#define READ_DATA_HASH_JSON_RESPONSE_LENGTH 			230

#define READ_PUBLIC_KEY_JSON_RESULT_OFFSET				194
#define READ_PUBLIC_KEY_JSON_RESPONSE_LENGTH 			742

#define READ_ETH_ACCOUNT_ADDR_JSON_RESULT_OFFSET 		90
#define READ_ETH_ACCOUNT_ADDR_JSON_RESPONSE_LENGTH 		102

#define READ_PUB_KEY_JSON_RESULT_DATA_LENGTH 			READ_PUB_KEY_RESULT_LENGTH * 2

#define READ_DATA_HASH_JSON_RESULT_DATA_LENGTH 			READ_DATA_HASH_RESULT_LENGTH * 2

/**
 * This handler struct holds all the information which
 * is required for data exchange via http
 */
typedef struct httpIpHandler_S{
	uint8_t payload[JSON_STRING_BUFF_SIZE];
	size_t payload_len;
} httpIpHandler_T;
static httpIpHandler_T httpIpHandleVar_T = {0};

/* external buffer to hold blockchain information */
uint8_t SEEDConsumerDataHashBuffer[READ_DATA_HASH_RESULT_LENGTH] = { 0 };

/* local buffers to hold blockchain information */
static uint8_t SEEDCroducerPublicKeyBuffer[READ_PUB_KEY_RESULT_LENGTH] = { 0 };
static uint8_t SEEDEtherAccountAddressBuffer[READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH] = { 0 };
static uint8_t SEEDTransactionHashBuffer[TRANSACTION_HASH_RESULT_LENGTH] = { 0 };

/* flag to indicate that http callback is received */
static bool HttpResponseCallbackReceivedFlag = false;

/* flag to check if transaction is already confirmed */
static bool TransactionConfirmed = false;

/**
 * This function decodes the payload data from
 * the blockchain. It converts numbers in ascii
 * format into int numbers and combines the two
 * 4 bit values in one byte stored in oBuff
 *
 * @param[in] stringToConvert_ptr
 * This reference holds the data string which
 * should be converted
 *
 * @param[in] iStringSize
 * Size of the string which should be converted
 *
 * @param[out] oBuff
 * Output buffer which holds the converted and
 * combined input data
 *
 * @return
 * void
 */
void convertCharToHex(uint8_t *stringToConvert_ptr, size_t iStringSize, uint8_t *oBuff)
{
	uint8_t tempBuff[READ_PUB_KEY_JSON_RESULT_DATA_LENGTH] = {0};

	/* check for null pointer */
	if(NULL != stringToConvert_ptr) {
		/* convert numbers from ascii format to int value (atoi
		 * does not support hex numbers larger then 9) */
		for(size_t i = 0; i < iStringSize; ++i) {
			if( ('a' <= stringToConvert_ptr[i]) && ('f' >= stringToConvert_ptr[i]) ) {
				tempBuff[i] = stringToConvert_ptr[i] - 87;
			} else if( ('A' <= stringToConvert_ptr[i]) && ('F' >= stringToConvert_ptr[i]) ) {
				tempBuff[i] = stringToConvert_ptr[i] - 55;
			} else {
				tempBuff[i] = stringToConvert_ptr[i] - '0';
			}
		}
		/* combine two 4 bit values into one 8 bit value to
		 * get the original data */
		for(size_t i = 0, u = 0; u < iStringSize; ++i, u += 2) {
			oBuff[i] = (tempBuff[u] << 4) | tempBuff[u+1];
		}
	}
}

/**
 * This function waits for a specified time until
 * a http response is received
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T WaitForHttpReceiveCallback()
{
	Retcode_T ret = RETCODE_FAILURE;
	uint8_t counter = 0;

	/* wait until flag is set */
	while(true != HttpResponseCallbackReceivedFlag) {
#ifdef ENABLE_DEBUG
		vTaskDelay(SECONDS(1));
		printf("waiting for http response\n\r");
#else
		vTaskDelay(SECONDS(1));
#endif
		counter++;
		/* break out of loop until maximum waiting time is reached */
		if(HTTPRESPONSE_SECONDSTOWAIT == counter) {
			return ret;
		}
	}

	/* reset variable and return */
	HttpResponseCallbackReceivedFlag = false;
	ret = RETCODE_SUCCESS;

	return ret;
}

/**
 * This function polls the blockchain until
 * the transaction is confirmed or until
 * the specified time runs out
 *
 * @return
 * true, if successful<br>
 * false, otherwise.
 */
bool WaitForTransactionConfirmation()
{
	bool retTransConfirmed = false;
	Retcode_T retHttpRequest = RETCODE_FAILURE;
	uint8_t counter = 0;

	/* while transaction is unconfirmed */
	while(false == TransactionConfirmed)
	{
		counter++;
		/* call getTransactionReceipt function to check wether transaction is confirmed */
		retHttpRequest = sendHttpDLTClientRequest(GET_TRANSACTION_RECEIPT, "na", "na", SEEDTransactionHashBuffer, sizeof(SEEDTransactionHashBuffer));

		/* if anything goes wrong then return false */
		if(RETCODE_SUCCESS == retHttpRequest) {
			retHttpRequest = WaitForHttpReceiveCallback();
			if(RETCODE_SUCCESS != retHttpRequest) {
				return retTransConfirmed;
			}
		}

		/* wait until transaction is confirmed */
		if( (RETCODE_SUCCESS == retHttpRequest) && (true == TransactionConfirmed)) {
			TransactionConfirmed = false;
			retTransConfirmed = true;

			return retTransConfirmed;
		} else {
			/* wait for a specific time before the next request */
			vTaskDelay(SECONDS(CONFIRMATION_TIME_TO_WAIT));
		}

		/* if maximum defined counter of request is reached return */
		if(CONFIRMATION_TRANSACTION_COUNTER == counter) {
			return retTransConfirmed;
		}
	}

	return retTransConfirmed;
}

/**
 * This function is called to create a JSON string
 * to make a JSON RPC call to the ethereum blockchain
 *
 * @param[in] etherMethod
 * This parameter holds the method which shall be called.
 * You can choose between four function calls which are
 * supported by the smart contract and passed as an enum
 * value.
 *	1. WRITE_DATA_HASH
 *	2. READ_DATA_HASH
 *	3. WRITE_PUBLIC_KEY
 *	4. READ_PUBLIC_KEY
 *	5. READ_PUBLIC_KEY_SENDER_ADDRESS
 *
 * @param[in] senderAddress_ptr
 * This string holds the ethereum sender address
 *
 * @param[in] receiverAddress_ptr
 * This string holds the ethereum receiver/contract address
 *
 * @param[in] payload_ptr
 * This reference holds the payload of which will be send
 * to the smart contract e.g. data hash or public key.
 * This parameter is only required for function WRITE_DATA_HASH
 * and WRITE_PUBLIC_KEY. Can be NULL for READ functions
 *
 * @param[in] iPayloadLength
 * Holds the length of the incoming payload
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T genJSONRequest(etherFuncCalls etherMethod, uint8_t const *senderAddress_ptr, uint8_t const *receiverAddress_ptr, uint8_t const *payload_ptr, size_t iPayloadLength)
{
	Retcode_T ret = RETCODE_FAILURE;
	uint8_t const *ethMethod_ptr  = NULL;
	uint8_t JSONStringBuff[JSON_STRING_BUFF_SIZE] = {0};
	uint16_t payloadOffset = 0;
	size_t JSONStringLength = 0;

	/* check for null pointers - payload_pr can be null
	 * iPayloadLength*2 because we extract two bytes(higher and lower nipple) out of one char */
	if( (NULL != senderAddress_ptr) && (NULL != receiverAddress_ptr) && \
			((iPayloadLength * 2 + WRITE_DATA_HASH_HEADER_LEN) <= sizeof(JSONStringBuff)) )
	{
		/* choose ethereum method */
		switch (etherMethod) {
			case WRITE_DATA_HASH:
				/* copy func hash and payload length information */
				strcpy(JSONStringBuff, WRITE_DATA_HASH_HEADER);
				/* set offset for payload */
				payloadOffset = WRITE_DATA_HASH_HEADER_LEN;
				/* convert input character into single bytes
				 * data encoding */
				for(uint16_t i = 0, u = 0; u < iPayloadLength; i=i+2, ++u) {
					snprintf(&JSONStringBuff[payloadOffset + i], 2, "%01x", (payload_ptr[u] >> 4 & 0x0F));
					snprintf(&JSONStringBuff[payloadOffset + i+1], 2, "%01x", (payload_ptr[u] & 0x0F));
				}
				/* set name of ether function call */
				ethMethod_ptr = "eth_sendTransaction";
			break;
			case READ_DATA_HASH:
				/* copy func hash */
				strcpy(JSONStringBuff, READ_DATA_HASH_HEADER);
				/* set name of ether function call */
				ethMethod_ptr = "eth_call";
			break;
			case WRITE_PUBLIC_KEY:
				/* copy func hash and payload length information */
				strcpy(JSONStringBuff, WRITE_PUBLIC_KEY_HEADER);
				/* set offset for payload */
				payloadOffset = WRITE_PUBLIC_KEY_HEADER_LEN;
				/* convert input character into single bytes
				 * data encoding */
				for(uint16_t i = 0, u = 0; u < iPayloadLength; i=i+2, ++u) {
					snprintf(&JSONStringBuff[payloadOffset + i], 2, "%01x", (payload_ptr[u] >> 4 & 0x0F));
					snprintf(&JSONStringBuff[payloadOffset + i+1], 2, "%01x", (payload_ptr[u] & 0x0F));
				}
				/* set name of ether function call */
				ethMethod_ptr = "eth_sendTransaction";
			break;
			case READ_PUBLIC_KEY:
				/* copy func hash */
				strcpy(JSONStringBuff, READ_PUBLIC_KEY_HEADER);
				/* set name of ether function call */
				ethMethod_ptr = "eth_call";
			break;
			case RATE_PRODUCER_POSITIVE:
				/* copy func hash */
				strcpy(JSONStringBuff, RATE_PRODUCER_POSITIVE_DATA);
				/* set name of ether function call */
				ethMethod_ptr = "eth_sendTransaction";
			break;
			case RATE_PRODUCER_NEGATIVE:
				/* copy func hash */
				strcpy(JSONStringBuff, RATE_PRODUCER_NEGATIVE_DATA);
				/* set name of ether function call */
				ethMethod_ptr = "eth_sendTransaction";
			break;
			case GET_TRANSACTION_RECEIPT:
				ethMethod_ptr = "eth_getTransactionReceipt";
			break;
			default:
				printf("Create JSON string input error\n\r");
			break;
		}

		/* two dimensional array to store JSON parameters */
		uint8_t const * ethereum_parameters[10][2] = {
				{"jsonrpc", JSON_RPC_VERSION},
				{"method", 	ethMethod_ptr},
				{"params", 	NULL},
				{"from", 	senderAddress_ptr},
				{"to", 		receiverAddress_ptr},
				{"gas", 	TRANSACTION_EXECUTION_GAS},
				{"value",	DATA_ETHER_EXCHANGE_RATE},
				{"data", 	JSONStringBuff},
				{"latest",	NULL},
				{"id", NULL}
		};

		/*create JSON message */
		cJSON *ethereumCallParameters = NULL;
		uint8_t *JSONstring = NULL;
		cJSON *ethereumCall = cJSON_CreateObject();

		/* add parameters to main JSON tree */
		cJSON_AddStringToObject(ethereumCall, ethereum_parameters[0][0], ethereum_parameters[0][1]);
		cJSON_AddStringToObject(ethereumCall, ethereum_parameters[1][0], ethereum_parameters[1][1]);

		/* add parameter-Array to main JSON tree */
		ethereumCallParameters = cJSON_AddArrayToObject(ethereumCall, ethereum_parameters[2][0]);

		if(GET_TRANSACTION_RECEIPT != etherMethod) {
			cJSON *params_object = cJSON_CreateObject();

			/* create params-Array parameters */
			cJSON_AddStringToObject(params_object, ethereum_parameters[3][0], ethereum_parameters[3][1]);
			cJSON_AddStringToObject(params_object, ethereum_parameters[4][0], ethereum_parameters[4][1]);
			cJSON_AddStringToObject(params_object, ethereum_parameters[5][0], ethereum_parameters[5][1]);

			/* provide ether in value parameter to write public key */
			if(WRITE_PUBLIC_KEY == etherMethod) {
				cJSON_AddStringToObject(params_object, ethereum_parameters[6][0], ethereum_parameters[6][1]);
			}
			cJSON_AddStringToObject(params_object, ethereum_parameters[7][0], ethereum_parameters[7][1]);
			/* add params-Array parameters to main tree params object */
			cJSON_AddItemToArray(ethereumCallParameters, params_object);
		} else {
			/* for function getTransactionReceipt only transaction is required as a parameter */
			cJSON_AddItemToArray(ethereumCallParameters, cJSON_CreateString(payload_ptr));
		}

		/* add quantity tag only for eth_call
		 * It is not supported by eth_sendTransaction
		 * */
		if( (strncmp(ethMethod_ptr, "eth_call", strlen("eth_call")) == 0) ) {
			cJSON_AddStringToObject(ethereumCallParameters, "NULL", ethereum_parameters[8][0]);
		}

		/* add id value to main JSON tree */
		if(cJSON_AddNumberToObject(ethereumCall, ethereum_parameters[9][0], etherMethod) != NULL) {
			/* copy created JSON tree into JSON string */
			JSONstring = cJSON_Print(ethereumCall);
			JSONStringLength = strlen(JSONstring);

			/* reset httpIpHandler payload buffer */
			memset(httpIpHandleVar_T.payload, 0, sizeof(httpIpHandleVar_T.payload));

			/* copy content and length information into global handler struct */
			if(sizeof(httpIpHandleVar_T.payload) >= JSONStringLength) {
				strcpy(httpIpHandleVar_T.payload, JSONstring);
				httpIpHandleVar_T.payload_len = JSONStringLength;
#ifdef ENABLE_DEBUG
				printf("JSON string: \n%s\n\r", httpIpHandleVar_T.payload);
#endif
				ret = RETCODE_SUCCESS;
			}
		}

		/* free allocated memory otherwise a stack heap collision
		 * will occur. JSONstring has to be freed because cJSON_Print
		 * assignes an allocated pointer to JSONstring */
		free(JSONstring);
		cJSON_Delete(ethereumCall);
	}

	return ret;
}

/**
 * This function is called to parse an incoming json string
 *
 * @param[in] monitor_ptr
 * This reference holds the incoming JSON string
 *
 * @param[out] oJSONData_buff
 * This buffer holds the result of the parsed
 * JSON object. The result contains the payload data
 * e.g. the DataHash or the PublicKey in a get function
 *
 * @param[in] JSONBuffLength
 * This buffer holds length of the provided buffer.
 * Must be at least the size of the incoming JSON message.
 *
 * @param[out] oEthMessageID_ptr
 * This reference will hold the message id of the incoming
 * response from the blockchain
 *
 * @return
 * RC_OK, if successful<br>
 * RC_MAX_APP_ERROR, otherwise.
 */
retcode_t parseIncomingJSONMessage(const uint8_t *monitor_ptr, uint8_t *oJSONData_buff, size_t JSONBuffLength, etherFuncCalls *oEthMessageID_ptr)
{
	retcode_t status = RC_MAX_APP_ERROR;
	const cJSON *result_ptr = NULL;
	const cJSON *id_ptr = NULL;
	const cJSON *status_ptr = NULL;

	/* parse incoming payload */
	cJSON *monitor_json = cJSON_Parse(monitor_ptr);

	if(NULL != monitor_json) {
		/* extract result parameter of incoming json string */
		result_ptr = cJSON_GetObjectItemCaseSensitive(monitor_json, "result");
		id_ptr = cJSON_GetObjectItemCaseSensitive(monitor_json, "id");

		/* assign ethereum function message id */
		if(cJSON_IsNumber(id_ptr) ) {
#ifdef ENABLE_DEBUG
			printf("JSON MessageID is: %i\n\r", id_ptr->valueint);
#endif
			*oEthMessageID_ptr = id_ptr->valueint;
		}

		/* for results without JSON array - all functions but GET_TRANSACTION_RECEIPT */
		if( (cJSON_IsString(result_ptr)) && (result_ptr->valuestring != NULL) ) {
#ifdef ENABLE_DEBUG
			printf("Result is: %s\n\r", result_ptr->valuestring);
#endif
			/* check if output buffer is big enough to hold
			 * result data */
			if(JSONBuffLength >= strlen(result_ptr->valuestring)) {
				strcpy(oJSONData_buff, result_ptr->valuestring);
				status = RC_OK;
			}
		}

		/* if transaction is not confirmed, result contains no string */
		if( (GET_TRANSACTION_RECEIPT == *oEthMessageID_ptr) && (!(cJSON_IsString(result_ptr->child))) ) {
#ifdef ENABLE_DEBUG
			printf("Transaction not confirmed\n\r");
#endif
			/* copy NotConfirmed string into result buff */
			if(JSONBuffLength >= strlen("NotConfirmed")) {
				strncpy(oJSONData_buff, "NotConfirmed", strlen("NotConfirmed"));
				status = RC_OK;
			}
		}

		/* if transaction is confirmed, result contains a string - status indicates if transaction was successful */
		if( (GET_TRANSACTION_RECEIPT == *oEthMessageID_ptr) && (cJSON_IsString(result_ptr->child)) ) {
			status_ptr = cJSON_GetObjectItemCaseSensitive(result_ptr, "status");
#ifdef ENABLE_DEBUG
			printf("Transaction confirmed with status %s\n\r",status_ptr->valuestring);
#endif
			/* if transaction was successfull, status is equal to 0x1 */
			if(strncmp(status_ptr->valuestring, "0x1", strlen("0x1")) == 0) {
				/* copy confirm string and status into result buff */
				if( JSONBuffLength >= strlen("Confirmed0x1") ) {
					strncpy(oJSONData_buff, "Confirmed0x1", strlen("Confirmed0x1"));
					status = RC_OK;
				}
			} else {
				/* copy confirm string and status into result buff */
				if( JSONBuffLength >= strlen("Confirmed0x0") ) {
					strncpy(oJSONData_buff, "Confirmed0x0", strlen("Confirmed0x0"));
					status = RC_OK;
				}
			}
		}
	}

	/* free cJSON objects */
	cJSON_Delete(monitor_json);

	return status;
}

/**
 * This function is called when a response to
 * an outgoing request is received.
 *
 * @param[in] httpSession_ptr
 * This reference holds all the information of
 * the HttpSession. Unused here.
 *
 * @param[in] msg_ptr
 * This reference holds all the context information
 * of the current Http message
 *
 * @param[in] status
 * This variable holds the status of the incoming
 * message.
 *
 * @return
 * RC_OK, if successful<br>
 * RC_MAX_APP_ERROR, otherwise.
 */
static retcode_t httpResponseReceivedCallback(HttpSession_T *httpSession_ptr, Msg_T *msg_ptr, retcode_t status)
{
    uint8_t JSONStringBuff[JSON_STRING_BUFF_SIZE] = { 0 };
    uint8_t JSONPubKeyResultBuff[READ_PUB_KEY_JSON_RESULT_DATA_LENGTH] = { 0 };
    uint8_t JSONConsumerAccountAddressResultBuff[READ_ETH_ACCOUNT_ADDRESS_RESULT_LENGTH] = { 0 };
    uint8_t JSONDataHashResultBuff[READ_DATA_HASH_JSON_RESULT_DATA_LENGTH] = { 0 };

    retcode_t ret = RC_MAX_APP_ERROR;
    etherFuncCalls ethMessageID = UNDEFINED;

    if(RC_OK == status && msg_ptr != NULL) {
    	/* get http status codes e.g. Http_StatusCode_OK (200) */
    	Http_StatusCode_T statusCode = HttpMsg_getStatusCode(msg_ptr);

    	/* get http content type e.g. Http_ContentType_Text_Html */
    	uint8_t const *contentType = HttpMsg_getContentType(msg_ptr);

    	uint8_t const *content_ptr;
    	unsigned int contentLength = 0;

    	HttpMsg_getContent(msg_ptr, &content_ptr, &contentLength);
    	/* create array of size content length + 1 */
    	uint8_t content[contentLength + 1];
    	memcpy(content, content_ptr, contentLength);
    	/* add null termination at the end of the content string */
    	content[contentLength] = 0;

#ifdef ENABLE_DEBUG
    	printf("HTTP response: %d [%s]\n\r", statusCode, contentType);
    	printf("%s\n\r", content);
    	printf("Content length: %i\n\r", contentLength);
#endif
    	/* parse incoming JSON request */
    	ret = parseIncomingJSONMessage(content, JSONStringBuff, sizeof(JSONStringBuff), &ethMessageID);

    	/* call function dependent on message id */
    	switch (ethMessageID) {
			case READ_DATA_HASH:
				/* read and convert data hash */
				memset(SEEDConsumerDataHashBuffer, 0, sizeof(SEEDConsumerDataHashBuffer));
				strncpy(JSONDataHashResultBuff, &JSONStringBuff[READ_DATA_HASH_JSON_RESULT_OFFSET], READ_DATA_HASH_JSON_RESULT_DATA_LENGTH);
				convertCharToHex(JSONDataHashResultBuff, READ_DATA_HASH_JSON_RESULT_DATA_LENGTH, SEEDConsumerDataHashBuffer);
#ifdef ENABLE_DEBUG
				printf("SEEDConsumerDataHashBuffer result: \n%s\n\r", SEEDConsumerDataHashBuffer);
#endif
			break;
			case READ_PUBLIC_KEY:
				/* read and convert public key */
				/* reset the public key buffer */
				memset(SEEDCroducerPublicKeyBuffer, 0, sizeof(SEEDCroducerPublicKeyBuffer));
				/* copy JSON result string into JSON result buff*/
				strncpy(JSONPubKeyResultBuff, &JSONStringBuff[READ_PUBLIC_KEY_JSON_RESULT_OFFSET], READ_PUB_KEY_JSON_RESULT_DATA_LENGTH);
				/* decode JSON and write public key into pk-buffer*/
				convertCharToHex(JSONPubKeyResultBuff, READ_PUB_KEY_JSON_RESULT_DATA_LENGTH, SEEDCroducerPublicKeyBuffer);
#ifdef ENABLE_DEBUG
				printf("SEEDCroducerPublicKeyBuffer result: \n%s\n\r", SEEDCroducerPublicKeyBuffer);
#endif
				/* read consumer account address */
				/* reset the account address buffer */
				memset(SEEDEtherAccountAddressBuffer, 0, sizeof(SEEDEtherAccountAddressBuffer));
				/* copy JSON result string into JSON result buff*/
				strncpy(JSONConsumerAccountAddressResultBuff, &JSONStringBuff[READ_ETH_ACCOUNT_ADDR_JSON_RESULT_OFFSET], READ_ETH_ACCOUNT_ADDRESS_RESULT_LENGTH);
				/* add 0x in front of address */
				strcpy(SEEDEtherAccountAddressBuffer, "0x");
				/* copy consumer account address to global buffer */
				strncat(SEEDEtherAccountAddressBuffer, JSONConsumerAccountAddressResultBuff, READ_ETH_ACCOUNT_ADDRESS_RESULT_LENGTH);
#ifdef ENABLE_DEBUG
				printf("SEEDEtherAccountAddressBuffer: %s\n\r", SEEDEtherAccountAddressBuffer);
#endif
				/* push consumer information into authentication array - shift old information upwards */
				strncpy(AuthenticatedConsumerTable[2].consumerPublicKey, AuthenticatedConsumerTable[1].consumerPublicKey, READ_PUB_KEY_RESULT_LENGTH);
				strncpy(AuthenticatedConsumerTable[2].accountAddress, AuthenticatedConsumerTable[1].accountAddress, READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH);
				AuthenticatedConsumerTable[2].activeConsumer = AuthenticatedConsumerTable[1].activeConsumer;

				strncpy(AuthenticatedConsumerTable[1].consumerPublicKey, AuthenticatedConsumerTable[0].consumerPublicKey, READ_PUB_KEY_RESULT_LENGTH);
				strncpy(AuthenticatedConsumerTable[1].accountAddress, AuthenticatedConsumerTable[0].accountAddress, READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH);
				AuthenticatedConsumerTable[1].activeConsumer = AuthenticatedConsumerTable[0].activeConsumer;

				strncpy(AuthenticatedConsumerTable[0].consumerPublicKey, SEEDCroducerPublicKeyBuffer, READ_PUB_KEY_RESULT_LENGTH);
				strncpy(AuthenticatedConsumerTable[0].accountAddress, SEEDEtherAccountAddressBuffer, READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH);
			break;
			case GET_TRANSACTION_RECEIPT:
				/* check if transaction is mined/confirmed - if status is bad then transaction
				 * is considered as unconfirmed because we have to send it again
				 * */
				if(strncmp(JSONStringBuff, "Confirmed0x1", strlen("Confirmed0x1")) == 0) {
					TransactionConfirmed = true;
				} else {
					TransactionConfirmed = false;
				}
			break;
			case WRITE_DATA_HASH:
			case WRITE_PUBLIC_KEY:
				/* for state changing functions store transaction hash so confirmation function can be called */
				memset(SEEDTransactionHashBuffer, 0, sizeof(SEEDTransactionHashBuffer));
				strncpy(SEEDTransactionHashBuffer, JSONStringBuff, TRANSACTION_HASH_RESULT_LENGTH);
#ifdef ENABLE_DEBUG
				printf("Transaction hash: %s\n\r", SEEDTransactionHashBuffer);
#endif
			break;
			default:
				/* do nothing */
			break;
    	}

    	/* set flag to true if answer is available in buffer */
    	HttpResponseCallbackReceivedFlag = true;

    	/* print received data */
    	if( (RC_OK == ret) && (Http_StatusCode_OK == statusCode) ) {
    		//good
    	}
#ifdef ENABLE_DEBUG
    	else {
    		printf("JSON parser result buffer to small\n\r");
    	}
#endif
    }
#ifdef ENABLE_DEBUG
    else {
    	printf("httpResponseReceivedCallback failed!\n\r");
    }
#endif

    /* explicitly close the current http session */
    HttpPool_close(httpSession_ptr);
    HttpPool_delete(httpSession_ptr);

    return ret;
}

/**
 * This function is called when the request is
 * finished/sent.
 *
 * @param[in] callfunc_ptr
 * This reference holds the Callable context
 *
 * @param[in] status
 * This variable holds the sent message status of the
 * request.
 *
 * @return
 * RC_OK, if successful<br>
 * RC_MAX_APP_ERROR, otherwise.
 */
static retcode_t HttpRequestSentCallback(Callable_T *callfunc_ptr, retcode_t status)
{
	/* surpress warning message concerning unused variable */
    (void) callfunc_ptr;

#ifdef ENABLE_DEBUG
    if(RC_OK != status) {
    	printf("Failed to send HTTP request!\n\r");
    } else {
    	printf("HTTP request sent!\n\r");
    }
#else
    (void) status;
#endif

    return status;
}

/**
 * This function is called for serializing
 * a part of an outgoing message.
 *
 * @param[in] handover_ptr
 * This reference holds the data for the serializer.
 * If the data is too large this function will serialize
 * and append the remaining data to the message.
 *
 * @return
 * RC_OK, if successful<br>
 * RC_MAX_APP_ERROR, otherwise.
 */
retcode_t writeNextPartToBuffer(OutMsgSerializationHandover_T* handover_ptr)
{
	retcode_t ret = RC_MAX_APP_ERROR;
	size_t payloadLength = httpIpHandleVar_T.payload_len;
	uint16_t alreadySerialized = handover_ptr->offset;
	uint16_t remainingLength = payloadLength - alreadySerialized;
	uint16_t bytesToCopy = 0;

	/* if whole message is serialized */
	if(remainingLength <= handover_ptr->bufLen) {
		bytesToCopy = remainingLength;
		ret = RC_OK;
	/* if the message is not fully serialized return incomplete
	 * and serialize the data segment which fits into the buffer
	 * */
	} else {
		bytesToCopy = handover_ptr->bufLen;
		ret = RC_MSG_FACTORY_INCOMPLETE;
	}

	memcpy(handover_ptr->buf_ptr, httpIpHandleVar_T.payload + alreadySerialized, bytesToCopy);
	/* set offset for next data chunk */
	handover_ptr->offset = alreadySerialized + bytesToCopy;
	/* set number of written bytes */
	handover_ptr->len = bytesToCopy;

    return ret;
}

/**
 * This function is called to send a Http request.
 * It sets the receiver with Http port and ip address.
 * It sets the request method option e.g. post or get
 * and sends the payload.
 *
 * @param[in] ethMethod
 * This enum variable holds the method which shall be called.
 * You can choose between four function calls which are
 * supported by the smart contract and passed as a string:
 *	WRITE_DATA_HASH = 0,
 *	READ_DATA_HASH,
 *	WRITE_PUBLIC_KEY,
 *	READ_PUBLIC_KEY,
 *	RATE_PRODUCER_POSITIVE,
 *	RATE_PRODUCER_NEGATIVE,
 *	GET_TRANSACTION_RECEIPT,
 *
 * @param[in] senderAddress_ptr
 * This reference holds the sender ethereum account address
 *
 * @param[in] receiverAddress_ptr
 * This reference holds the receiver ethereum account address
 *
 * @param[in] payload_ptr
 * This reference holds the payload which will be send
 * to the smart contract e.g. data hash or public key.
 *
 * @param[in] iPayloadLength
 * This variable holds the length of the incoming payload data
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T sendHttpDLTClientRequest(etherFuncCalls ethMethod, uint8_t const *senderAddress_ptr, uint8_t const *receiverAddress_ptr, uint8_t const *payload_ptr, size_t iPayloadLength)
{
	static Callable_T sentCallableHttp;
	Msg_T *msg_ptr = 0;
	Retcode_T ret = RETCODE_FAILURE;
	Ip_Address_T destIPAddr = 0;
	Ip_Port_T destIPPort = 0;

	/* reset global callback response received flag */
	HttpResponseCallbackReceivedFlag = false;

	/* convert ip address and port */
	Ip_convertStringToAddr(HTTP_IP_ADDRESS, &destIPAddr);
	destIPPort = Ip_convertIntToPort(HTTP_PORT);

	/* call this function to create the outgoing JSON string */
	genJSONRequest(ethMethod, senderAddress_ptr, receiverAddress_ptr, payload_ptr, iPayloadLength);

	ret = HttpClient_initRequest(&destIPAddr, destIPPort, &msg_ptr);

	if(RETCODE_SUCCESS == ret) {
		/* prepare http header */
		HttpMsg_setReqMethod(msg_ptr, Http_Method_Post);
		HttpMsg_setContentType(msg_ptr, Http_ContentType_App_Json);
		HttpMsg_setReqUrl(msg_ptr, DESTINATION_POST_PATH);
		HttpMsg_setHost(msg_ptr, HOST_ADDRESS);

		if(RETCODE_SUCCESS == ret) {
			/* add a function to the message factory which serializes the buffer data
			 * Here we use the global handle struct to process the payload
			 * */
			ret = TcpMsg_prependPartFactory(msg_ptr, &writeNextPartToBuffer);

#ifdef ENABLE_DEBUG
			printf("TcpMsg_prependPartFactory\n\r");
#endif

			if(RETCODE_SUCCESS == ret) {
				/* start sending the request by assign the function to the callable
				 * HttpClient_pushRequest expects a callable object as a parameter */
				Callable_assign(&sentCallableHttp, &HttpRequestSentCallback);

				/* push the request */
				ret = HttpClient_pushRequest(msg_ptr, &sentCallableHttp, &httpResponseReceivedCallback);
#ifdef ENABLE_DEBUG
				if(RETCODE_SUCCESS == ret) {
					printf("Send http request successful\n\r");
				} else {
					printf("Error during send http request: %i\n\r", ret);
				}
#endif
			}
		}
	}
#ifdef ENABLE_DEBUG
	if(RETCODE_SUCCESS != ret) printf("send http request error\n\r");
#endif

	return ret;
}
