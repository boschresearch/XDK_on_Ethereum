/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_USERCONFIG_H_
#define SOURCE_USERCONFIG_H_

/* debug definition - enable printf's */
#define ENABLE_DEBUG

/* enable/disable single modules of the program */
#define ENABLE_WIFI
#define ENABLE_STATIC_IP
//#define ENABLE_WIFI_ENTERPRISE
#define ENABLE_SENSOR
#define ENABLE_ENCRYPTION
#define ENABLE_HTTP
/* configure Producer or Consumer build */
#define ENABLE_CONSUMER
//#define ENABLE_PRODUCER


/* WIFI credentials */
#ifdef ENABLE_WIFI_ENTERPRISE
	#define SSID 		" "
	#define USERNAME   	" "
	#define PASSPHRASE 	" "
	/* EAP method security type */
	#define SEC_TYPE 	0x05
#else
	#define SSID " "
	#define PASSPHRASE " "
#endif

/* Ethereum node network informations */
#ifdef ENABLE_WIFI_ENTERPRISE
	#ifdef ENABLE_PRODUCER
		/* network defines */
		#define HTTP_IP_ADDRESS " "
		#define HOST_ADDRESS 	" "
		/* HTTP json-rpc port */
		#define HTTP_PORT 0000
	#else /* Consumer */
		/* network defines */
		#define HTTP_IP_ADDRESS " "
		#define HOST_ADDRESS 	" "
		/* HTTP json-rpc port */
		#define HTTP_PORT 0000
	#endif
#else
	/* in my private blockchain, Producer and Consumer
	 * run on the same RPC server
	 * */
	/* network defines */
	#define HTTP_IP_ADDRESS " "
	#define HOST_ADDRESS 	" "
	/* HTTP json-rpc port */
	#define HTTP_PORT 00000
#endif

/* default CoAP port for producer <--> consumer off-chain data exchange */
#define COAP_PORT 0000
/* use of static ip so it always remains the same.
 * In some cases could cause an error if it is
 * already in use.
 * */
#ifdef ENABLE_WIFI_ENTERPRISE
	#define COAP_SERVER_IP ""
#else
	#define COAP_SERVER_IP ""
#endif

/* seconds to wait until Http response is received */
#define HTTPRESPONSE_SECONDSTOWAIT		10

/* seconds to wait until transaction is confirmed */
#define CONFIRMATION_TRANSACTION_COUNTER 	5
#define CONFIRMATION_TIME_TO_WAIT			5

/* accel value threshhold */
#define ACCELEROMETER_VALUE_THRESHHOLD	5
/* define count of ticks for measuring x accel values after button1 pressed on XDK */
#define ACCELERATION_VALUES 1000

/* ethereum account information */
#define CONTRACT_ADDRESS 			"" 			//e.g. "0xc47e575b2cacdc22545da4c0fe7aead9ce90a9f2"
#define CONSUMER_ACCOUNT_ADDRESS 	"" 	//e.g. "0x275b4EFC07BB4A8eb56fAF050Cf6436C2c06250E"
#define PRODUCER_ACCOUNT_ADDRESS 	"" 	//e.g. "0x63c3465D4a300d767F0BDDD8d5ce256BbBD6fb41"

/* define XDK IPV4 address - Producer */
#ifdef ENABLE_WIFI_ENTERPRISE
	#ifdef ENABLE_PRODUCER
		#define IPV4_ADDRESS            NetworkConfig_Ipv4Value(00, 00, 0, 000)
	#else /* CONSUMER */
		#define IPV4_ADDRESS            NetworkConfig_Ipv4Value(00, 00, 0, 000)
	#endif
	#define IPV4_DNS_SERVER         NetworkConfig_Ipv4Value(00, 0, 0, 000)
	#define IPV4_GATEWAY            NetworkConfig_Ipv4Value(00, 00, 0, 0)
	#define IPV4_MASK               NetworkConfig_Ipv4Value(255, 255, 255, 0)
#else
	#ifdef ENABLE_PRODUCER
		#define IPV4_ADDRESS            NetworkConfig_Ipv4Value(000, 000, 000, 00)
	#else /* CONSUMER */
		#define IPV4_ADDRESS            NetworkConfig_Ipv4Value(000, 000, 000, 00)
	#endif
	#define IPV4_DNS_SERVER         NetworkConfig_Ipv4Value(000, 000, 000, 0)
	#define IPV4_GATEWAY            NetworkConfig_Ipv4Value(000, 000, 000, 0)
	#define IPV4_MASK               NetworkConfig_Ipv4Value(255, 255, 255, 0)
#endif

/* RSA key pair buffers */
/* producer keys - not used until now */
#if 0
/* size of private key 792 bytes including text */
static uint8_t const *PrivateRSAKeyProducer1024 = "";
/*------------------- Example -------------------------------------*/
/* "-----BEGIN RSA PRIVATE KEY-----\n"
* "MIICXQIBAAKBgQCpvMeRSLiBZuX/PP0/Uewt6kebBCsDETBTKlCnfcd4zvn7Lxre\n"
* "tPRoSAi7AkihRUoYvdEzdwjdteuMUgKXFfDQmDi/pO2qeYResxHcxhcxlPywtR99\n"
* "vPQjMeQqjiukMQ5Gaw1+VAXC78AM/VHz6Hrg6QyKgeCSGCPe7sG9Fxj4xQIDAQAB\n"
* "AoGABE1JjZAXRQhTmfV0wa8U2lEOwYoIgQplfCYdZzFT5ebxBQG7n5tcemwg9IRp\n"
* "TNURvLDK5ZAFxIDA2IyXjja7JLOtsgHZZXYkstDiZWJYYzX6y4zhscjOAjOjJRN8\n"
* "WJPesvhxudzY4h7XlA1JB1PBzdL84731SLSFRJtsLrHuLsECQQDhYnVYgYiQi5Ms\n"
* "PTI6Y4nB5fL+vgUHtQnImZZU3mpCywJGGq4YL+c7c1c+vMlFEGpcVoQQyDnGImH2\n"
* "dLpVDPuxAkEAwMs94nocH9C6oygo95+wFzXyUYvAwtdfcCxqNHVxYdAo/xSsY3wC\n"
* "RxJ3p0wr54S6jOjGCpk+nJshUFaMX5SXVQJBANxAB0SScQ4wF4Zn1ynQE9L0D955\n"
* "exjpBcKOtKYDI/xZvsMbV34zcdhbAqtAeb+QJyBNO4na4PqKpwjdUSnEIkECQQCH\n"
* "uIxAOyZBX3eEFGmCqPAV5uxHa9KvV17gYOQDOgoviZLSv4L8JfiUf/Or2nut6EpL\n"
* "mDKSk374UF0LaWI4hyphAkBRmZBQ/EMGfdR3jrk9zKC5ABuhhXdE7ItKT93Qd2g8\n"
* "g8A5ZBCdx84F2cJ2/HDnn5YcknnN0BGpVOHw+VI2YX/x\n"
* "-----END RSA PRIVATE KEY-----\n";
*/

/* 278 bytes with text */
static uint8_t const *PublicRSAKeyProducer1024 = "";
/*------------------- Example -------------------------------------*/
/* "-----BEGIN PUBLIC KEY-----\n"
* "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCpvMeRSLiBZuX/PP0/Uewt6keb\n"
* "BCsDETBTKlCnfcd4zvn7LxretPRoSAi7AkihRUoYvdEzdwjdteuMUgKXFfDQmDi/\n"
* "pO2qeYResxHcxhcxlPywtR99vPQjMeQqjiukMQ5Gaw1+VAXC78AM/VHz6Hrg6QyK\n"
* "geCSGCPe7sG9Fxj4xQIDAQAB\n"
* "-----END PUBLIC KEY-----\n";
*/
#endif

/* consumer keys */
static uint8_t const *PrivateRSAKeyConsumer1024 = "";
/*------------------- Example -------------------------------------*/
/* "-----BEGIN RSA PRIVATE KEY-----\n"
* "MIICXAIBAAKBgQCUyoD2UEz6ExolzTga0rziAuNfXxeS/5NyYehwnUMyLUB4j8i5\n"
* "XBAfFldrzDmk8D973rAwyKJ9v4sqFNyzjjJpIon0MCD5d2WS+74vypvcV6VTWQPp\n"
* "k8/pVL9M410RvHRCdhR7sVjYp91YrCXAODqy0qXQj59cLktZh0DV0BixpwIDAQAB\n"
* "AoGAIj9+cbPIFCPDeAIFsP7i5S7/ARvVREu5t7FbnFhGeE08MsP90tSjDVTKKJDo\n"
* "j9OQ+UUnzwLPjBxDvxrwNhA7/+SICpMIWJznQv3p6v1j2idw6FqssIUNPV6c44sl\n"
* "EO2flaRHHSan5GY5o5WNd9T26U3/V235hprGZF5oXH5ob/kCQQD6HyEPYAs/WOU9\n"
* "m8rnruq83EVt6cUf0uz1eaw6pz9yOQlmT+Ln+rYKBrwJT57bObB4NxuVOgWqeCRV\n"
* "K4/dIBUjAkEAmEm2pnLWm9aUGOk9NESR+IvculyyiNUDXdM6DgffYSZYPR4GKlYf\n"
* "zCZxv5Vl97p1UB7Rzj1HGcJr6XQ1OFkDrQJAVSRll5tFGOJE3sz6rBVB+NoulDTA\n"
* "ko21dfZeJ3UpRtOdnINTJU6VyyHxvmWpGM0xgiqYLBsdNKNDEu8KQOab6wJAePzf\n"
* "WsAT1n2U7XGoSXVMzz024OyyftlVMl6VWf5RLxrKscu/tDT2UDge2Mm12CnP+BZ0\n"
* "Mzkl2sZG+5NykNDPhQJBALwTzhhE+i7/6POfNB8EKiQ4ZXJwJ31RJXYE4Tv3zFPa\n"
* "ANWJBGHLKw0+ugRYfZs9qNk15hgtRGSxPjad4QAe1yI=\n"
* "-----END RSA PRIVATE KEY-----\n";
*/

static uint8_t const *PublicRSAKeyConsumer1024 = "";
/*------------------- Example -------------------------------------*/
/* "-----BEGIN PUBLIC KEY-----\n"
* "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCUyoD2UEz6ExolzTga0rziAuNf\n"
* "XxeS/5NyYehwnUMyLUB4j8i5XBAfFldrzDmk8D973rAwyKJ9v4sqFNyzjjJpIon0\n"
* "MCD5d2WS+74vypvcV6VTWQPpk8/pVL9M410RvHRCdhR7sVjYp91YrCXAODqy0qXQ\n"
* "j59cLktZh0DV0BixpwIDAQAB\n"
* "-----END PUBLIC KEY-----\n";
*/

#endif /* SOURCE_USERCONFIG_H_ */
