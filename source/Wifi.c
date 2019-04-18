/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.

*/

#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "PAL_initialize_ih.h"
#include "PAL_socketMonitor_ih.h"
#include "wlan.h"

/* user includes */
#include "Wifi.h"
#include "UserConfig.h"
#include "SystemConfig.h"

#ifdef ENABLE_WIFI_ENTERPRISE
/**
 * This function is called to connect to the enterprise
 * WIFI network.
 *
 * @params[in] connectSSID
 * The SSID of the given enterprise WLAN
 *
 * @params[in] connectPass
 * Password of the given enterprise WLAN
 *
 * @params[in] connectCallback
 * Holds the callback function
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T WlanEnterpriseConnect(WlanConnect_SSID_T connectSSID,
		WlanConnect_PassPhrase_T connectPass,
		WlanConnect_Callback_T connectCallback)
{
    /* Local variables */
	WlanConnect_Status_T retVal;
    int8_t slStatus;
    SlSecParams_t secParams;
    SlSecParamsExt_t eapParams;

    /* Set network security parameters */
    secParams.Key = connectPass;
    secParams.KeyLen = strlen((const char*) connectPass);
    secParams.Type = SEC_TYPE;

    /* Set network EAP parameters */
    eapParams.User = (signed char*)USERNAME;
    eapParams.UserLen = strlen(USERNAME);
    eapParams.AnonUser = (signed char*)USERNAME;
    eapParams.AnonUserLen = strlen(USERNAME);
    eapParams.CertIndex = UINT8_C(0);
    eapParams.EapMethod = SL_ENT_EAP_METHOD_PEAP0_MSCHAPv2;

    /* Call the connect function */
    slStatus = sl_WlanConnect((signed char*) connectSSID,
            strlen(((char*) connectSSID)), (unsigned char *) WLANCONNECT_NOT_INITIALZED,
            &secParams, &eapParams);

    /* Determine return value*/
    if (WLANCONNECT_NOT_INITIALZED == slStatus)
    {
        if (WLANCONNECT_NOT_INITIALZED == connectCallback)
        {
            while ((WLAN_DISCONNECTED == WlanConnect_GetStatus())
                    || (NETWORKCONFIG_IP_NOT_ACQUIRED == NetworkConfig_GetIpStatus()))
            {
                /* Stay here until connected. */
                /* Timeout logic can be added here */
            }
            /* In blocking mode a successfully return of the API also means
             * that the connection is ok*/
            retVal = WLAN_CONNECTED;
        }
        else
        {
            /* In callback mode a successfully return of the API does not mean
             * that the WLAN Connection has succeeded. The SL event handler will
             * notify the user with WLI_CONNECTED when connection is done*/
            retVal = WLAN_CONNECTED;
        }
    }
    else
    {
        /* Simple Link function encountered an error.*/
        retVal = WLAN_CONNECTION_ERROR;
    }

    return (retVal);
}
#endif

/**
 * This function is called to setup the WIFI
 * connection. It disables DHCP and sets
 * the static IP's for CoAP consumer and producer.
 *
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T NetworkConfigStatic(void)
{
	Retcode_T retStatusSetIp;
	NetworkConfig_IpSettings_T myIpSet;

	myIpSet.isDHCP = (uint8_t) NETWORKCONFIG_DHCP_DISABLED;
	myIpSet.ipV4 = IPV4_ADDRESS;
	myIpSet.ipV4DnsServer = IPV4_DNS_SERVER;
	myIpSet.ipV4Gateway = IPV4_GATEWAY;
	myIpSet.ipV4Mask = IPV4_MASK;

	retStatusSetIp = NetworkConfig_SetIpStatic(myIpSet);

#ifdef ENABLE_DEBUG
	if(RETCODE_SUCCESS == retStatusSetIp) {
		printf("Wifi static: Initialization completed\n\r");
	} else {
		printf("Error in NetworkSetup\n\r");
	}
#endif

	return retStatusSetIp;
}

/**
 * This function is called to setup the WIFI
 * connection. It sets the SSID and password for
 * the corresponding network.
 * It initializes the PAL module for the serval stack.
 *
 * @return
 * RETCODE_SUCCESS, if successful<br>
 * RETCODE_FAILURE, otherwise.
 */
Retcode_T WifiNetworkInit(void)
{
	Retcode_T ret = RETCODE_FAILURE;
	WlanConnect_SSID_T connectSSID = (WlanConnect_SSID_T) SSID;
	WlanConnect_PassPhrase_T connectPassPhrase = (WlanConnect_PassPhrase_T) PASSPHRASE;

	ret = WlanConnect_Init();
	if(RETCODE_SUCCESS == ret) {
#ifdef ENABLE_STATIC_IP
		ret = NetworkConfigStatic();
#else
		ret = NetworkConfig_SetIpDhcp(NULL);
#endif
		if(RETCODE_SUCCESS == ret) {
			/* use callback for non blocking API call or NULL for blocking API call */
#ifdef ENABLE_WIFI_ENTERPRISE
			/* disable server authentication */
			unsigned char pValues;
			pValues = 0;  //0 - Disable the server authentication | 1 - Enable
			sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,19,1,&pValues);
			ret = (Retcode_T) WlanEnterpriseConnect(connectSSID, connectPassPhrase, NULL);
#else
			ret = (Retcode_T) WlanConnect_WPA(connectSSID, connectPassPhrase, NULL);
#endif
			if(RETCODE_SUCCESS == ret) {
				ret = PAL_initialize();
			}
		}
	}
#ifdef ENABLE_DEBUG
	if(RETCODE_SUCCESS == ret) {
		printf("Wifi: Initialization completed\n\r");
	} else {
		printf("Error in NetworkSetup\n\r");
	}
#endif
	PAL_socketMonitorInit();

	return ret;
}

/**
 * This cyclic function checks the WIFI
 * connection. If the connection gets lost,
 * this function will reconnect automatically
 * or otherwise print an error.
 *
 * @params[in] pvParameters (unused)
 */
void wifiCyclic(void* pvParameters)
{
	(void)pvParameters;
	Retcode_T ret = RETCODE_FAILURE;
	WlanConnect_SSID_T connectSSID = (WlanConnect_SSID_T) SSID;
	WlanConnect_PassPhrase_T connectPassPhrase = (WlanConnect_PassPhrase_T) PASSPHRASE;

	for(;;) {
		vTaskDelay(500);
		/* check network status */
		if(CONNECTED_AND_IPV4_ACQUIRED != WlanConnect_GetCurrentNwStatus()){
			/* if connection gets lost, reconnect */
			printf("Wifi connection lost, reconnect\n\r");
#ifdef ENABLE_WIFI_ENTERPRISE
			ret = WlanEnterpriseConnect(connectSSID, connectPassPhrase, NULL);
#else
			ret = WlanConnect_WPA(connectSSID, connectPassPhrase, NULL);
#endif
			if(RETCODE_SUCCESS == ret) {
				printf("Wifi reconnected\n\r");
			}
		}
	}
}
