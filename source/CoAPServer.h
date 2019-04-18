/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_COAPSERVER_H_
#define SOURCE_COAPSERVER_H_
#include "SystemConfig.h"

/* global interface task declarations */
xTaskHandle CoAPServerTask;
xTaskHandle PrepSensPayloadDataTask;

/* control declaration for extern variable */
extern AuthConsumer_T AuthenticatedConsumerTable[CONSUMER_NUMBER_MAX];

/* global interface function declarations */
Retcode_T CoAPServerInit(void);
void prepareSensorPayloadData(void* pvParameters);

#endif /* SOURCE_COAPSERVER_H_ */
