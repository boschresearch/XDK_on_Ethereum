/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_WIFI_H_
#define SOURCE_WIFI_H_

/* global interface task declarations */
xTaskHandle WifiCheckTask;

/* global interface function declarations */
Retcode_T WifiNetworkInit(void);
void wifiCyclic(void* pvParameters);

#endif /* SOURCE_WIFI_H_ */
