/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

/* system header files */
#include <stdio.h>
/* additional interface header files */
#include "FreeRTOS.h"
#include "timers.h"
#include "XdkSensorHandle.h"
#include "BCDS_BSP_Button.h"
#include "BCDS_BSP_LED.h"
#include "BSP_Boardtype.h"
#include "BCDS_CmdProcessor.h"

/* user includes */
#include "SensorData.h"
#include "UserConfig.h"
#include "SystemConfig.h"
#include "SecureEdgeDevice.h"

/* external sensor handler variables */
extern Accelerometer_HandlePtr_T xdkAccelerometers_BMA280_Handle;
extern Environmental_HandlePtr_T xdkEnvironmental_BME280_Handle;

/* global flag to inform other modules if button is pressed */
static bool Button1PressedFlag = false;

/* variable to store the acceleration value after button pressed */
static uint8_t AccelSensorValue = 0;

/**
 * This function is used to read the current button1 status
 *
 * @return
 * true, if button1 pressed <br>
 * false, otherwise.
 */
bool Button1Pressed()
{
	return Button1PressedFlag;
}

/**
 * This function initializes the red, orange and yellow LED
 * of the XDK.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
static Retcode_T LedInitialize(void)
{
    Retcode_T ret = RETCODE_OK;
    ret = BSP_LED_Connect();

    if (RETCODE_OK == ret) {
    	ret = BSP_LED_Enable((uint32_t) BSP_XDK_LED_R);
    }
    if (RETCODE_OK == ret) {
    	ret = BSP_LED_Enable((uint32_t) BSP_XDK_LED_O);
    }
    if (RETCODE_OK == ret) {
    	ret = BSP_LED_Enable((uint32_t) BSP_XDK_LED_Y);
    }
#ifdef ENABLE_DEBUG
    if (RETCODE_OK == ret) {
        printf("LED Initialization succeed\n\r");
    } else {
        printf(" Error occurred in LED initialization\n\r");
    }
#endif
    return ret;
}

/**
 * This function is called if the button triggers
 * an interrupt.
 * It toggles the LED if button is pressed and released.
 *
 * @param[in] param_ptr
 * unused
 *
 * @param[in] buttonstatus
 * this variable holds the button status (Pressed/released)
 *
 * @return
 * void
 */
static void processButtonData(void * param_ptr, uint32_t buttonstatus)
{
    BCDS_UNUSED(param_ptr);
    Retcode_T ret = RETCODE_OK;

    if (BSP_XDK_BUTTON_PRESS == buttonstatus)
    {
#ifdef ENABLE_DEBUG
        printf("Button 1 pressed\n\r");
#endif
        Button1PressedFlag = true;

    	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        if (RETCODE_OK == ret)
        {
        	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_O, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        }
        if (RETCODE_OK == ret)
        {
        	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        }
    }

    if (BSP_XDK_BUTTON_RELEASE == buttonstatus)
    {
#ifdef ENABLE_DEBUG
        printf("Button 1 released\n\r");
#endif
        Button1PressedFlag = false;

    	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        if (RETCODE_OK == ret)
        {
        	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_O, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        }
        if (RETCODE_OK == ret)
        {
        	ret = BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_TOGGLE);
        }
    }
}

/**
 * This callback is called by an interrupt
 * if button 1 is pressed/released.
 *
 * It forwards the call to processButtonData func.
 *
 * @param[in] data
 * This variable holds the button information
 *
 * @return
 * void
 */
void Button1Callback(uint32_t data)
{
    Retcode_T returnValue = CmdProcessor_EnqueueFromIsr(AppCmdProcessor, processButtonData, NULL, data);
    if (RETCODE_OK != returnValue) {
#ifdef ENABLE_DEBUG
        printf("Enqueuing for Button 1 callback failed\n\r");
#endif
    }
}

/**
 * This routine is used to initialize the button 1
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
/* Routine to Initialize the Button module  */
static Retcode_T ButtonInitialize(void)
{
    Retcode_T ret = RETCODE_OK;
    ret = BSP_Button_Connect();

    if (RETCODE_OK == ret) {
    	ret = BSP_Button_Enable((uint32_t) BSP_XDK_BUTTON_1, Button1Callback);
    }
#ifdef ENABLE_DEBUG
    if (RETCODE_OK == ret) {
        printf("Button Initialization success\n\r");
    } else {
        printf("Error occurred in button initialization routine\n\r");
    }
#endif
    return ret;
}

/**
 * This function is called to initialize the accelerometer
 * sensor module of the Bosch XDK. It sets different
 * parameters like bandwidth or sensor range.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T SensorInit(void)
{
	Retcode_T ret = RETCODE_FAILURE;

	/* Initialize accelerometer sensor */
	ret = Accelerometer_init(xdkAccelerometers_BMA280_Handle);
	if (RETCODE_OK == ret) {
		ret = Accelerometer_setBandwidth(xdkAccelerometers_BMA280_Handle, ACCELEROMETER_BMA280_BANDWIDTH_125HZ);
	}
	if (RETCODE_OK == ret) {
		ret = Accelerometer_setRange(xdkAccelerometers_BMA280_Handle, ACCELEROMETER_BMA280_RANGE_2G);
	}
	if (RETCODE_OK == ret) {
		/* Initialize environmental sensor */
		ret = Environmental_init(xdkEnvironmental_BME280_Handle);
	}
	if (RETCODE_OK == ret) {
		/* Initialize buttons */
		ret = ButtonInitialize();
	}
	if (RETCODE_OK == ret) {
		/* Initialize LED's */
		ret = LedInitialize();
	}
#ifdef ENABLE_DEBUG
	if (RETCODE_OK == ret) {
		printf("SensorInit successful\n\r");
	} else {
		printf("SensorInit failed\n\r");
	}
#endif

	return ret;
}

/**
 * This function is called to read the environmental sensor
 * data.
 *
 * @param[out] data_ptr
 * This reference will hold the humidity value of the
 * environmental sensor.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T GetEnvironmentalSensorData(uint32_t *data_ptr)
{
	Retcode_T ret = RETCODE_FAILURE;
	Environmental_Data_T bme280 = { INT32_C(0), UINT32_C(0), UINT32_C(0) };

	if(NULL != data_ptr) {
		ret = Environmental_readData(xdkEnvironmental_BME280_Handle, &bme280);

		/* assign humidity value to payload_ptr */
		if(RETCODE_SUCCESS == ret) {
			*data_ptr = bme280.humidity;
		}
	}

	return ret;
}

/**
 * This function is just a getter function to retreive
 * the recorded sensor value from the cyclic task
 *
 * @return uint8_t
 * Returns the recorded accel value.
 */
uint8_t GetAccelerometerSensorData(void)
{
	return AccelSensorValue;
}

/**
 * This function is called to read the accelerometer
 * sensor data for encryption.
 * The sensor values are linked by a XOR operation.
 *
 * @param[out] data_ptr
 * This buffer will hold the linked accelerometer sensor
 * data values.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T GetAcceleromterSensorEntropyData(int32_t* data_ptr)
{
	Retcode_T ret = RETCODE_FAILURE;
	/* define array to hold x, y and z sensor data */
	Accelerometer_XyzData_T bma280 = {INT32_C(0), INT32_C(0), INT32_C(0)};

	memset(&bma280, 0, sizeof(Accelerometer_XyzData_T));
	ret = Accelerometer_readXyzGValue(xdkAccelerometers_BMA280_Handle, &bma280);

	if(RETCODE_SUCCESS == ret) {
		*data_ptr = bma280.xAxisData ^ bma280.yAxisData ^ bma280.zAxisData;
	} else {
#ifdef ENABLE_DEBUG
		printf("Error in GetAcceleromterSensorData\n\r");
#endif
	}

	return ret;
}

/**
 * This cyclic function is called to read accelerometer
 * data if button one is pressed on producer xdk.
 *
 * @param[in] pvParameters
 * Unused.
 */
void SensorDataCyclic(void* pvParameters)
{
	(void)pvParameters;
	/* define array to hold x, y and z sensor data */
	Accelerometer_XyzData_T bma280 = {INT32_C(0), INT32_C(0), INT32_C(0)};
	uint32_t AccelSensorValueLocal = 0;

	for(;;)
	{
		/* check if button 1 is pressed */
		if(Button1Pressed() == true) {
			/* reset sensor values */
			AccelSensorValueLocal = 0;
			AccelSensorValue = 0;

			/* record accelermoter values within defined time value */
			for(uint16_t counter = 0; counter < ACCELERATION_VALUES; ++counter) {
				memset(&bma280, 0, sizeof(Accelerometer_XyzData_T));
				Accelerometer_readXyzGValue(xdkAccelerometers_BMA280_Handle, &bma280);
				if(bma280.xAxisData < 0) bma280.xAxisData = bma280.xAxisData * (-1);
				AccelSensorValueLocal += bma280.xAxisData;
			}

			/* get average accel value */
			AccelSensorValueLocal = AccelSensorValueLocal / (ACCELERATION_VALUES);
			/* convert to G value */
			AccelSensorValue = AccelSensorValueLocal / 100;

			/* switch led's dependent on acceleration value */
			if(AccelSensorValue >= ACCELEROMETER_VALUE_THRESHHOLD) {
				BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_ON);
				BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_OFF);
			} else {
				BSP_LED_Switch((uint32_t) BSP_XDK_LED_Y, (uint32_t) BSP_LED_COMMAND_ON);
				BSP_LED_Switch((uint32_t) BSP_XDK_LED_R, (uint32_t) BSP_LED_COMMAND_OFF);
			}
#ifdef ENABLE_DEBUG
			printf("Acceleration sensor value is: %i G\n\r", AccelSensorValue);
#endif
		}
	}
}
