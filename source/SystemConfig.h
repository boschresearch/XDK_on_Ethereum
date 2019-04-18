/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_SYSTEMCONFIG_H_
#define SOURCE_SYSTEMCONFIG_H_

/* wait macro which waits for x seconds */
#define SECONDS(x) ((portTickType) (x * 1000) / portTICK_RATE_MS)

/* queue parameters */
#define QUEUE_ELEMENT_COUNTER 	1
#define QUEUE_BUFF_SIZE			256

/* number of consumers stored in authentication array */
#define CONSUMER_NUMBER_MAX	3

/* define blockchain json-rpc sizes */
#define CONTRACT_ADDRESS_LENGTH  						42
#define DATA_HASH_BUFF_SIZE 							32
#define TRANSACTION_HASH_RESULT_LENGTH					66
#define READ_DATA_HASH_RESULT_LENGTH 					32
#define READ_PUB_KEY_RESULT_LENGTH 						274
#define READ_ETH_ACCOUNT_ADDRESS_RESULT_LENGTH 			40
#define READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH 	READ_ETH_ACCOUNT_ADDRESS_RESULT_LENGTH + 2

/* queue handler for global queue access from all modules */
typedef struct queueHandler_S {
	size_t queuePayloadLength;
	uint8_t queuePayload[QUEUE_BUFF_SIZE];
} queueHandler_T;

/* authentication data type for local storage of consumer information */
typedef struct AuthConsumer_S {
	uint8_t accountAddress[READ_ETH_ACCOUNT_ADDRESS_RESULT_DATA_LENGTH];
	uint8_t consumerPublicKey[READ_PUB_KEY_RESULT_LENGTH];
	bool activeConsumer;
} AuthConsumer_T;

#endif /* SOURCE_SYSTEMCONFIG_H_ */
