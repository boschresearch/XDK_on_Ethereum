/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_HTTP_H_
#define SOURCE_HTTP_H_

#include "SystemConfig.h"

/* enum type which represents the blockchain function calls */
typedef enum {
	WRITE_DATA_HASH = 1,
	READ_DATA_HASH = 2,
	WRITE_PUBLIC_KEY = 3,
	READ_PUBLIC_KEY = 4,
	RATE_PRODUCER_POSITIVE = 5,
	RATE_PRODUCER_NEGATIVE = 6,
	GET_TRANSACTION_RECEIPT = 7,
	UNDEFINED = 0xFF
} etherFuncCalls;

/* control declaration for external variable */
extern uint8_t SEEDConsumerDataHashBuffer[READ_DATA_HASH_RESULT_LENGTH];

/* global interface function declarations */
Retcode_T sendHttpDLTClientRequest(etherFuncCalls ethMethod, uint8_t const *senderAddress_ptr, uint8_t const *receiverAddress_ptr, uint8_t const *payload_ptr, size_t iPayloadLength);
Retcode_T genJSONRequest(etherFuncCalls etherMethod, uint8_t const *senderAddress_ptr, uint8_t const *receiverAddress_ptr, uint8_t const *payload_ptr, size_t iPayloadLength);
Retcode_T WaitForHttpReceiveCallback(void);
bool WaitForTransactionConfirmation(void);

#endif /* SOURCE_COAP_H_ */
