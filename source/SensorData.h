/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_SENSORDATA_H_
#define SOURCE_SENSORDATA_H_

/* global interface task declarations */
xTaskHandle SensorDataTask;

/* global interface function declarations */
void SensorDataCyclic(void* pvParameters);
Retcode_T SensorInit(void);
bool Button1Pressed(void);
Retcode_T GetAcceleromterSensorEntropyData(int32_t* data_ptr);
Retcode_T GetEnvironmentalSensorData(uint32_t *data_ptr);
uint8_t GetAccelerometerSensorData(void);

#endif /* SOURCE_SENSORDATA_H_ */
