/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_COAPCLIENT_H_
#define SOURCE_COAPCLIENT_H_

/* global interface task declarations */
xTaskHandle CoAPClientTask;
xTaskHandle processSensorPayloadDataTask;

/* global interface function declarations */
Retcode_T CoAPClientInit(void);
void CoAPClientCyclic(void* pvParameters);
void processSensorPayloadDataCyclic(void* pvParameters);

#endif /* SOURCE_COAPCLIENT_H_ */
