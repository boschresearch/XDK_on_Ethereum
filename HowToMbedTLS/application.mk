##################################################################
#      Makefile for generating the xdk application               #
##################################################################
MAKE_FILE_ROOT ?= .

BCDS_PLATFORM_PATH := $(MAKE_FILE_ROOT)/../Platform
BCDS_LIBRARIES_PATH := $(MAKE_FILE_ROOT)/../Libraries
BCDS_XDK_TOOLS_PATH := $(MAKE_FILE_ROOT)/../Tools
include $(dir $(BCDS_COMMON_MAKEFILE))common_settings.mk

#XDK application specific Paths 
BCDS_XDK_COMMON_PATH =$(MAKE_FILE_ROOT)/../Common
BCDS_XDK_CONFIG_PATH = $(BCDS_XDK_COMMON_PATH)/config
FLASH_TOOL_PATH = $(BCDS_DEVELOPMENT_TOOLS)/EA_commander/V2.82/eACommander
BCDS_PACKAGE_ID = 153
BCDS_XDK_DEBUG_DIR = debug
BCDS_XDK_RELEASE_DIR = release
BCDS_XDK_INCLUDE_DIR = $(BCDS_XDK_COMMON_PATH)/include
BCDS_XDK_LEGACY_INCLUDE_DIR = $(BCDS_XDK_COMMON_PATH)/legacy/include
BCDS_XDK_OBJECT_DIR = objects
BCDS_XDK_LINT_DIR = lint
BCDS_XDK_APP_PATH= $(BCDS_APP_DIR)
BCDS_APP_NAME_WITH_HEADER = $(BCDS_APP_NAME)_with_header

MBEDTLS_LIBRARY_PATH := $(BCDS_APP_DIR)/mbedtls

export XDK_FOTA_ENABLED_BOOTLOADER ?=0
export BCDS_DEVICE_ID = EFM32GG390F1024
export BCDS_SYSTEM_STARTUP_METHOD ?= DEFAULT_STARTUP
export BCDS_TOOL_CHAIN_PATH ?= $(BCDS_GCC_PATH)
export BCDS_TARGET_PLATFORM = efm32
export BCDS_DEVICE_TYPE = EFM32GG
# The below defined values is to update firmware version formed by given MAJOR MINOR and PATCH 
BCDS_FOTA_TOOL_PATH =  $(BCDS_XDK_TOOLS_PATH)/Fota_Tools
MAJOR_SW_NO ?= 0x00#Defines MAJOR number and maximum value is 255
MINOR_SW_NO ?= 0x00#Defines MINOR number and maximum value is 255
PATCH_SW_NO ?= 0x01#Defines PATCH number and maximum value is 255
HEADER_VERSION =0100#Defines the current used header version
PRODUCT_CLASS =0001#(Productcode[12bit]=001 for APLM, Minimum Hardwarerevision[4bit]
PRODUCT_VARIANT =0001
FIRMWARE_VERSION = $(subst 0x,,00$(MAJOR_SW_NO)$(MINOR_SW_NO)$(PATCH_SW_NO))
CREATE_CONTAINER_SCRIPT = $(BCDS_FOTA_TOOL_PATH)/create_fota_container.py

#Path where the LINT config files are present(compiler and environment configurations)
BCDS_LINT_CONFIG_PATH =  $(BCDS_TOOLS_PATH)/PCLint/v9.00-0.2.3/config
BCDS_LINT_CONFIG_FILE := $(BCDS_LINT_CONFIG_PATH)/bcds.lnt
LINT_EXE = $(BCDS_TOOLS_PATH)/PCLint/v9.00-0.2.3/exe/lint-nt.launch

# These serval macros should be above Libraies.mk
# This variable should fully specify the build configuration of the Serval 
# Stack library with regards the enabled and disabled features of TLS to use DTLS instead of TLS. 
SERVAL_ENABLE_TLS_CLIENT ?=0
SERVAL_ENABLE_TLS_ECC ?=0
SERVAL_ENABLE_TLS_PSK ?=1
SERVAL_ENABLE_DTLS_PSK ?=1
SERVAL_MAX_NUM_MESSAGES ?=16
SERVAL_MAX_SIZE_APP_PACKET ?=600
SERVAL_ENABLE_TLS ?=0
SERVAL_ENABLE_DTLS_ECC ?=0
EscTls_CIPHER_SUITE ?=TLS_PSK_WITH_AES_128_CCM_8

# This variable should fully specify the build configuration of the Serval 
# Stack library with regards the enabled and disabled features as well as 
# the configuration of each enabled feature.
BCDS_SERVALSTACK_MACROS += \
	-D SERVAL_LOG_LEVEL=SERVAL_LOG_LEVEL_ERROR\
	-D SERVAL_ENABLE_HTTP_CLIENT=1\
	-D SERVAL_ENABLE_HTTP_SERVER=1\
	-D SERVAL_ENABLE_WEBSERVER=1\
	-D SERVAL_ENABLE_COAP=1\
	-D SERVAL_ENABLE_COAP_OBSERVE=1\
	-D SERVAL_ENABLE_COAP_CLIENT=1\
	-D SERVAL_ENABLE_COAP_SERVER=1\
	-D SERVAL_ENABLE_REST_CLIENT=1\
	-D SERVAL_ENABLE_REST_SERVER=1\
	-D SERVAL_ENABLE_REST_HTTP_BINDING=1\
	-D SERVAL_ENABLE_REST_COAP_BINDING=1\
	-D SERVAL_ENABLE_XUDP=1\
	-D SERVAL_ENABLE_DPWS=0\
	-D SERVAL_ENABLE_TLS_CLIENT=$(SERVAL_ENABLE_TLS_CLIENT)\
	-D SERVAL_ENABLE_TLS_SERVER=0\
	-D SERVAL_ENABLE_TLS_ECC=$(SERVAL_ENABLE_TLS_ECC)\
	-D SERVAL_ENABLE_TLS_PSK=$(SERVAL_ENABLE_TLS_PSK)\
	-D SERVAL_ENABLE_DTLS=1\
	-D SERVAL_ENABLE_DTLS_CLIENT=1\
	-D SERVAL_ENABLE_DTLS_SERVER=1\
	-D SERVAL_ENABLE_DTLS_PSK=$(SERVAL_ENABLE_DTLS_PSK)\
	-D SERVAL_ENABLE_HTTP_AUTH=1\
	-D SERVAL_ENABLE_HTTP_AUTH_DIGEST=1\
	-D SERVAL_ENABLE_DUTY_CYCLING=1\
	-D SERVAL_ENABLE_APP_DATA_ACCESS=0\
	-D SERVAL_ENABLE_COAP_COMBINED_SERVER_AND_CLIENT=1\
	-D SERVAL_ENABLE_COAP_OBSERVE=1\
	-D SERVAL_ENABLE_LWM2M=1\
	-D SERVAL_ENABLE_XTCP=1\
	-D SERVAL_ENABLE_XTCP_SERVER=1\
	-D SERVAL_ENABLE_XTCP_CLIENT=1\
	-D SERVAL_HTTP_MAX_NUM_SESSIONS=3\
	-D SERVAL_HTTP_SESSION_MONITOR_TIMEOUT=4000\
	-D SERVAL_MAX_NUM_MESSAGES=$(SERVAL_MAX_NUM_MESSAGES)\
	-D SERVAL_MAX_SIZE_APP_PACKET=$(SERVAL_MAX_SIZE_APP_PACKET)\
	-D SERVAL_MAX_SECURE_SOCKETS=5\
	-D SERVAL_MAX_SECURE_CONNECTIONS=5\
	-D SERVAL_SECURE_SERVER_CONNECTION_TIMEOUT=300000\
	-D SERVAL_DOWNGRADE_TLS=1\
	-D SERVAL_TLS_CYASSL=0 \
	-D SERVAL_ENABLE_DTLS_ECC=$(SERVAL_ENABLE_DTLS_ECC) \
	-D SERVAL_ENABLE_DTLS_RSA=0 \
	-D COAP_MSG_MAX_LEN=224 \
	-D COAP_MAX_NUM_OBSERVATIONS=50 \
	-D LWM2M_MAX_NUM_OBSERVATIONS=50 \
	-D SERVAL_LWM2M_SECURITY_INFO_MAX_LENGTH=65 \
	-D LWM2M_IP_ADDRESS_MAX_LENGTH=65 \
	-D SERVAL_HTTP_MAX_LENGTH_URL=256 \
	-D SERVAL_TLS_CYCURTLS=1 \
	-D LWM2M_MAX_LENGTH_DEVICE_NAME=32 \
	-D SERVAL_ENABLE_DTLS_SESSION_ID=0 \
	-D LWM2M_DISABLE_CLIENT_QUEUEMODE=1 \
	-D PAL_MAX_NUM_ADDITIONAL_COMM_BUFFERS=6 \
	-D SERVAL_ENABLE_DTLS_HEADER_LOGGING=0 \
	-D CYCURTLS_HAVE_GET_CURRENT_TASKNAME=0 \
	-D SERVAL_DTLS_FLIGHT_MAX_RETRIES=4 \
	-D SERVAL_EXPERIMENTAL_DTLS_MONITOR_EXTERN=0 \
	-D SERVAL_POLICY_STACK_CALLS_TLS_API=1 \
	-D SERVAL_SECURITY_API_VERSION=2 \
	-D SERVAL_ENABLE_HTTP_RANGE_HANDLING=1 \
	-D SERVAL_ENABLE_MQTT=1 \
	-D SERVAL_ENABLE_TLS=$(SERVAL_ENABLE_TLS) \
	-D COAP_OVERLOAD_QUEUE_SIZE=15 \
	-D SERVAL_ENABLE_SNTP_CLIENT=1 \
	-D SERVAL_ENABLE_HTTP=1 \
	-D BCDS_SERVAL_COMMBUFF_SEND_BUFFER_MAX_LEN=1000

BCDS_CYCURTLS_MACROS = \
	-DEscTls_PSK_IDENTITY_LENGTH_MAX=65 \
	-DEscTls_MAXIMUM_FRAGMENT_LENGTH=256 \
	-D EscTls_CIPHER_SUITE=$(EscTls_CIPHER_SUITE) \
	-D EscHashDrbg_SHA_TYPE=256 \
	-D EscDtls_MAX_COOKIE_SIZE=64 \
	-D EscX509_HASH_SHA1_ENABLED=1 \
	-D EscX509_HASH_SHA512_ENABLED=1 \
	-D EscX509_ALGORITHM_RSA_PKCS1V15_ENABLED=1 \
	-D EscX509_PARSE_INFO=1 \
	-D EscTls_CHECK_CERT_EXPIRATION=0 \
	-D EscRsa_KEY_BITS_MAX=2048U

include Libraries.mk

# Build chain settings
ifneq ("$(wildcard $(BCDS_TOOL_CHAIN_PATH))","")
CC = $(BCDS_TOOL_CHAIN_PATH)/arm-none-eabi-gcc
AR = $(BCDS_TOOL_CHAIN_PATH)/arm-none-eabi-ar
OBJCOPY = $(BCDS_TOOL_CHAIN_PATH)/arm-none-eabi-objcopy
else
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
OBJCOPY = arm-none-eabi-objcopy
endif
RMDIRS := rm -rf

#This flag is used to generate dependency files 
DEPEDENCY_FLAGS = -MMD -MP -MF $(@:.o=.d)

#Path of the XDK debug object files
BCDS_XDK_APP_DEBUG_OBJECT_DIR = $(BCDS_XDK_APP_PATH)/$(BCDS_XDK_DEBUG_DIR)/$(BCDS_XDK_OBJECT_DIR)
BCDS_XDK_APP_DEBUG_DIR = $(BCDS_XDK_APP_PATH)/$(BCDS_XDK_DEBUG_DIR)


#Path of the XDK Release object files
BCDS_XDK_APP_RELEASE_OBJECT_DIR =  $(BCDS_XDK_APP_PATH)/$(BCDS_XDK_RELEASE_DIR)/$(BCDS_XDK_OBJECT_DIR)
BCDS_XDK_APP_RELEASE_DIR = $(BCDS_XDK_APP_PATH)/$(BCDS_XDK_RELEASE_DIR)

export BCDS_CFLAGS_COMMON += -std=c99 -Wall -Wextra -Wstrict-prototypes -D$(BCDS_DEVICE_ID) -D BCDS_TARGET_EFM32 \
-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections \
$(BCDS_SERVALSTACK_MACROS) $(BCDS_CYCURTLS_MACROS) -D$(BCDS_SYSTEM_STARTUP_METHOD) \
-DENABLE_DMA -DARM_MATH_CM3 -DXDK_FOTA_ENABLED_BOOTLOADER=$(XDK_FOTA_ENABLED_BOOTLOADER) \
-DBCDS_SERVALPAL_WIFI=1


LDFLAGS_DEBUG = -Xlinker -Map=$(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).map \
-mcpu=cortex-m3 -mthumb -T $(BCDS_XDK_LD_FILE) -Wl,--gc-sections

ASMFLAGS = -x assembler-with-cpp -Wall -Wextra -mcpu=cortex-m3 -mthumb

export BCDS_CFLAGS_DEBUG_COMMON ?= $(BCDS_CFLAGS_COMMON) -O0 -g
export BCDS_CFLAGS_DEBUG = $(BCDS_CFLAGS_DEBUG_COMMON)

export BCDS_CFLAGS_RELEASE_COMMON ?= $(BCDS_CFLAGS_COMMON) -O0 -DNDEBUG
export BCDS_CFLAGS_RELEASE = $(BCDS_CFLAGS_RELEASE_COMMON)

LDFLAGS_RELEASE = -Xlinker -Map=$(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).map \
-mcpu=cortex-m3 -mthumb -T $(BCDS_XDK_LD_FILE) -Wl,--gc-sections

LIBS = -Wl,--start-group -lgcc -lc -lm  -Wl,--end-group

#The static libraries of the platform and third party sources are grouped here. Inorder to scan the libraries
#for undefined reference again and again, the libraries are listed between the --start-group and --end-group.
BCDS_DEBUG_LIBS_GROUP = -Wl,--start-group $(BCDS_LIBS_DEBUG) $(BCDS_THIRD_PARTY_LIBS) -Wl,--end-group

BCDS_RELEASE_LIBS_GROUP = -Wl,--start-group $(BCDS_LIBS_RELEASE) $(BCDS_THIRD_PARTY_LIBS) -Wl,--end-group

ifneq ($(XDK_FOTA_ENABLED_BOOTLOADER),1)
XDK_APP_ADDRESS = 0x00010000 # @see efm32gg.ld , This is the flash start address if EA commander tool used for flashing 
BCDS_XDK_LD_FILE = efm32gg.ld
$(info old_bootloader)
else
XDK_APP_ADDRESS = 0x00020000 # @see efm32gg_new.ld, This is the flash start address if EA commander tool used for flashing 
BCDS_XDK_LD_FILE = efm32gg_new.ld
$(info new_bootloader)
endif


# Define the path for include directories.
BCDS_XDK_INCLUDES += \
                  -I$(BCDS_XDK_INCLUDE_DIR) \
                  -I$(BCDS_XDK_LEGACY_INCLUDE_DIR)/ServalPAL_WiFi \
                  -I$(BCDS_XDK_CONFIG_PATH) \
                  -I$(BCDS_XDK_CONFIG_PATH)/Drivers \
                  -I$(BCDS_XDK_CONFIG_PATH)/FOTA \
                  -I$(BCDS_XDK_CONFIG_PATH)/Utils \
				  -I$(BCDS_XDK_CONFIG_PATH)/ServalPal \
       		      -I$(BCDS_BLE_PATH)/include \
       		      -I$(BCDS_BLE_PATH)/include/services \
                  -I$(BCDS_ESSENTIALS_PATH)/include \
                  -I$(BCDS_DRIVERS_PATH)/include \
                  -I$(BCDS_SENSORS_PATH)/include \
				  -I$(BCDS_SENSORS_UTILS_PATH)/include \
                  -I$(BCDS_SERVALPAL_PATH)/include \
                  -I$(BCDS_UTILS_PATH)/include \
                  -I$(BCDS_BSP_PATH)/include \
				  -I$(BCDS_SENSOR_TOOLBOX_PATH)/include \
				  -I$(BCDS_FOTA_PATH)/include \
				  -I$(BCDS_FOTA_PATH)/source/protected \
				  -I$(BCDS_WLAN_PATH)/include \
				  -I$(BCDS_XDK_CONFIG_PATH)/Essentials \
				  -I$(BCDS_ESSENTIALS_PATH)/include/bsp \
				  -I$(BCDS_ESSENTIALS_PATH)/include/mcu/efm32 \
				  -I$(BCDS_ESSENTIALS_PATH)/include/mcu \
				  -I$(BCDS_LORA_DRIVERS_PATH)/include
				  
				  
# By using -isystem, headers found in that direcwtory will be considered as system headers, because of that 
# all warnings, other than those generated by #warning, are suppressed.	
BCDS_XDK_EXT_INCLUDES += \
				  -isystem $(BCDS_SERVALSTACK_LIB_PATH)/include \
				  -isystem $(BCDS_SERVALSTACK_LIB_PATH)/3rd-party/ServalStack/api \
				  -isystem $(BCDS_SERVALSTACK_LIB_PATH)/3rd-party/ServalStack/pal \
				  -isystem $(BCDS_FREERTOS_PATH)/3rd-party/FreeRTOS/Source/portable/GCC/ARM_CM3 \
                  -isystem $(BCDS_FREERTOS_PATH)/3rd-party/FreeRTOS/Source/include \
                  -isystem $(BCDS_EMLIB_PATH)/3rd-party/EMLib/emlib/inc \
                  -isystem $(BCDS_EMLIB_PATH)/3rd-party/EMLib/usb/inc \
                  -isystem $(BCDS_EMLIB_PATH)/3rd-party/EMLib/Device/SiliconLabs/EFM32GG/Include \
                  -isystem $(BCDS_EMLIB_PATH)/3rd-party/EMLib/CMSIS/Include \
	              -isystem $(BCDS_FATFSLIB_PATH)/3rd-party/fatfs/src \
	              -isystem $(BCDS_BLE_CORE_PATH) \
                  -isystem $(BCDS_BLE_INTERFACE_PATH) \
                  -isystem $(BCDS_BLE_INTERFACE_PATH)/ATT \
                  -isystem $(BCDS_BLE_INTERFACE_PATH)/Services \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_ALPWDataExchange/Interfaces \
				  -isystem $(BCDS_WIFILIB_PATH)/3rd-party/TI/simplelink/include \
				  -isystem $(BCDS_BSX_LIB_PATH)/BSX4/Source/algo/algo_bsx/Inc \
				  -isystem $(BCDS_ESCRYPTLIB_PATH)/3rd-party/CycurTLS/src/cycurlib/lib/inc \
				  -isystem $(BCDS_ESCRYPTLIB_PATH)/3rd-party/CycurTLS/src/cycurtls/inc \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_AlertNotification/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_AppleNotificationCenter/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_BloodPressure/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_CyclingPower/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_CyclingSpeedAndCadence/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_FindMe/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_Glucose/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_HealthThermometer/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_HeartRate/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_HumanInterfaceDevice/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_iBeacon/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_LocationAndNavigation/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_PhoneAlertStatus/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_Proximity/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_RunningSpeedAndCadence/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_Time/Interfaces \
				  -isystem $(BCDS_BLE_SERVICE_PATH)/BLESW_WeightScale/Interfaces
				  
# Only include mbedTLS lib if SEED_BUILD is activate to prevent other project builds from crashing
ifeq ($(SEED_BUILD), TRUE)
	BCDS_XDK_EXT_INCLUDES += \
				  -isystem $(MBEDTLS_LIBRARY_PATH)/include
endif

# list of other Source files required for XDK application
BCDS_XDK_PLATFORM_SOURCE_FILES += \
	$(wildcard $(BCDS_XDK_COMMON_PATH)/source/*.c) \
	$(wildcard $(BCDS_XDK_COMMON_PATH)/legacy/source/*.c) \
	$(BCDS_EMLIB_PATH)/3rd-party/EMLib/usb/src/em_usbdint.c \

# Filter out default cJSON of XDK workbench because SEED project uses a newer version
ifeq ($(SEED_BUILD), TRUE)
	BCDS_XDK_PLATFORM_SOURCE_FILES := $(filter-out $(BCDS_XDK_COMMON_PATH)/source/cJSON.c, $(BCDS_XDK_PLATFORM_SOURCE_FILES)) 
endif

# Startup file for XDK application	
BCDS_XDK_APP_STARTUP_FILES = \
	startup_efm32gg.S
	
# Debug Object files list for building XDK application
BCDS_XDK_PLATFORM_COMMOM_C_OBJECT_FILES = $(BCDS_XDK_PLATFORM_SOURCE_FILES:.c=.o)
BCDS_XDK_PLATFORM_C_OBJECT_FILES = $(subst $(BCDS_XDK_COMMON_PATH)/,,$(BCDS_XDK_PLATFORM_COMMOM_C_OBJECT_FILES))
BCDS_XDK_APP_S_OBJECT_FILES = $(BCDS_XDK_APP_STARTUP_FILES:.S=.o)
BCDS_XDK_APP_C_OBJECT_FILES = $(patsubst $(BCDS_APP_SOURCE_DIR)/%.c, %.o, $(BCDS_XDK_APP_SOURCE_FILES))
BCDS_XDK_APP_OBJECT_FILES =  $(BCDS_XDK_PLATFORM_C_OBJECT_FILES) $(BCDS_XDK_APP_C_OBJECT_FILES) $(BCDS_XDK_APP_S_OBJECT_FILES) 
BCDS_XDK_APP_OBJECT_FILES_DEBUG = $(addprefix $(BCDS_XDK_APP_DEBUG_OBJECT_DIR)/, $(BCDS_XDK_APP_OBJECT_FILES))

# Release Object files list for building XDK application
BCDS_XDK_APP_OBJECT_FILES_RELEASE = $(addprefix $(BCDS_XDK_APP_RELEASE_OBJECT_DIR)/, $(BCDS_XDK_APP_OBJECT_FILES))

# Dependency File List for building XDK application files 
BCDS_XDK_APP_DEPENDENCY_RELEASE_FILES = $(addprefix $(BCDS_XDK_APP_RELEASE_OBJECT_DIR)/, $(BCDS_XDK_APP_OBJECT_FILES:.o=.d))
BCDS_XDK_APP_DEPENDENCY_DEBUG_FILES = $(addprefix $(BCDS_XDK_APP_DEBUG_OBJECT_DIR)/, $(BCDS_XDK_APP_OBJECT_FILES:.o=.d))

# Lint File List for building XDK application files 

# Lint flags
LINT_CONFIG = \
	+d$(BCDS_DEVICE_TYPE) +dASSERT_FILENAME $(BCDS_SERVAL_CONFIG_LINT) $(BCDS_CYCURTLS_CONFIG_LINT) \
	+dBCDS_TARGET_PLATFORM=efm32 +d$(BCDS_DEVICE_ID) +d$(BCDS_HW_VERSION) -e506 -e464 -e438\
	$(BCDS_EXTERNAL_EXCLUDES_LINT) $(BCDS_EXTERNAL_INCLUDES_LINT) $(BCDS_LINT_CONFIG) \
	$(BCDS_XDK_INCLUDES_LINT) $(BCDS_LINT_CONFIG_FILE) +dDBG_ASSERT_FILENAME \
	+dBCDS_SERVALPAL_WIFI=1
	

BCDS_XDK_APP_LINT_PATH = $(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_XDK_LINT_DIR)
BCDS_XDK_APP_LINT_FILES = $(patsubst $(BCDS_APP_SOURCE_DIR)/%.c, %.lob, $(BCDS_XDK_APP_SOURCE_FILES))
BCDS_XDK_LINT_FILES = $(addprefix $(BCDS_XDK_APP_LINT_PATH)/, $(BCDS_XDK_APP_LINT_FILES))


#Create debug binary
.PHONY: debug 
debug: $(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).bin

#Create release binary
.PHONY: release 
release: $(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).bin


# Clean project
.PHONY: clean clean_libraries

clean: clean_libraries
	@echo "Cleaning project in app.mk"
	$(RMDIRS) $(BCDS_XDK_APP_DEBUG_DIR) $(BCDS_XDK_APP_RELEASE_DIR)	

#Compile, assemble and link for debug target
#Compile the sources from plaform or library
$(BCDS_XDK_APP_DEBUG_OBJECT_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo "Building file $<"
	$(CC) $(DEPEDENCY_FLAGS) $(BCDS_CFLAGS_DEBUG_COMMON) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES) -DBCDS_PACKAGE_ID=$(BCDS_PACKAGE_ID) -c $< -o $@
	
#Compile the sources from application
$(BCDS_XDK_APP_DEBUG_OBJECT_DIR)/%.o: $(BCDS_APP_SOURCE_DIR)/%.c 
	@mkdir -p $(@D)
	@echo $(BCDS_XDK_APP_PATH)
	@echo "Building file $<"
	@$(CC) $(DEPEDENCY_FLAGS) $(BCDS_CFLAGS_DEBUG_COMMON) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES) -DBCDS_PACKAGE_ID=$(BCDS_PACKAGE_ID) -c $< -o $@
	
$(BCDS_XDK_APP_DEBUG_OBJECT_DIR)/%.o: %.S
	@mkdir -p $(@D)
	@echo "Assembling $<"
	@$(CC) $(ASMFLAGS) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES) -c $< -o $@
	
$(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).out: $(BCDS_LIBS_DEBUG) $(BCDS_XDK_APP_OBJECT_FILES_DEBUG) 
	@echo "Creating .out $@"
	@$(CC) $(LDFLAGS_DEBUG) $(BCDS_XDK_APP_OBJECT_FILES_DEBUG) $(BCDS_DEBUG_LIBS_GROUP) $(LIBS) -o $@
	
$(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).bin: $(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).out
	@echo "Boot flag value is $(XDK_FOTA_ENABLED_BOOTLOADER)"
	@echo "Creating binary for debug $@"
	@$(OBJCOPY) -O binary $(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).out $@
	

#Compile, assemble and link for release target
#Compile the sources from plaform or library
$(BCDS_XDK_APP_RELEASE_OBJECT_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo "Building file $<"
	@$(CC) $(DEPEDENCY_FLAGS) $(BCDS_CFLAGS_RELEASE_COMMON) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES)  -DBCDS_PACKAGE_ID=$(BCDS_PACKAGE_ID) -c $< -o $@

#Compile the sources from application
$(BCDS_XDK_APP_RELEASE_OBJECT_DIR)/%.o: $(BCDS_APP_SOURCE_DIR)/%.c
	@mkdir -p $(@D)
	@echo "Building file $<"
	@$(CC) $(DEPEDENCY_FLAGS) $(BCDS_CFLAGS_RELEASE_COMMON) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES) -DBCDS_PACKAGE_ID=$(BCDS_PACKAGE_ID) -c $< -o $@

$(BCDS_XDK_APP_RELEASE_OBJECT_DIR)/%.o: %.S
	@mkdir -p $(@D)
	@echo "Assembling $<"
	@$(CC) $(ASMFLAGS) $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES) -c $< -o $@

$(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).out: $(BCDS_LIBS_RELEASE) $(BCDS_XDK_APP_OBJECT_FILES_RELEASE) 
	@echo "Creating .out $@"
	@$(CC) $(LDFLAGS_RELEASE) $(BCDS_XDK_APP_OBJECT_FILES_RELEASE) $(BCDS_RELEASE_LIBS_GROUP) $(LIBS) -o $@

$(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).bin: $(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).out
	@echo "Creating binary $@"
	@$(OBJCOPY) -R .usrpg -O binary $(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).out $@
	

#if the header file is changed, compiler considers the change in a header file and compiles. Here no need to clean the application
#to take changes of header file during compilation
-include $(BCDS_XDK_APP_DEPENDENCY_DEBUG_FILES)
-include $(BCDS_XDK_APP_DEPENDENCY_RELEASE_FILES)

#Flash the .bin file to the target
flash_debug_bin: $(BCDS_XDK_APP_DEBUG_DIR)/$(BCDS_APP_NAME).bin
	@$(FLASH_TOOL_PATH) --address $(XDK_APP_ADDRESS) -v -f $< -r
	@echo "Flashing is completed successfully"

flash_release_bin: $(BCDS_XDK_APP_RELEASE_DIR)/$(BCDS_APP_NAME).bin
	@$(FLASH_TOOL_PATH) --address $(XDK_APP_ADDRESS) -v -f $< -r
	@echo "Flashing is completed successfully"

.PHONY: lint		   
lint: $(BCDS_XDK_LINT_FILES)
	@echo "Lint End"

$(BCDS_XDK_APP_LINT_PATH)/%.lob:  $(BCDS_APP_SOURCE_DIR)/%.c
	@echo "===================================="
	@mkdir -p $(@D)
	@echo $@
	@$(LINT_EXE) $(LINT_CONFIG) $< -oo[$@]
	@echo "===================================="

.PHONY: cleanlint
cleanlint:
	@echo "Cleaning lint output files"
	@rm -rf $(BCDS_XDK_APP_LINT_PATH)
	
cdt:
	@echo "cdt"
	echo $(BCDS_CFLAGS_DEBUG_COMMON)
	$(CC) $(BCDS_CFLAGS_DEBUG_COMMON) $(patsubst %,-I%, $(abspath $(patsubst -I%,%, $(BCDS_XDK_INCLUDES) $(BCDS_XDK_EXT_INCLUDES))))  -E -P -v -dD -c ${CDT_INPUT_FILE}
		
.PHONY: cdt
	
#-------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------

BCDS_SERVAL_CONFIG_LINT := $(subst -D , +d,$(BCDS_SERVALSTACK_MACROS))
BCDS_CYCURTLS_CONFIG_LINT := $(subst -D , +d,$(BCDS_CYCURTLS_MACROS))
BCDS_EXTERNAL_EXCLUDES_LINT := $(foreach DIR, $(subst -isystem ,,$(BCDS_XDK_EXT_INCLUDES)), +libdir\($(DIR)\))
BCDS_EXTERNAL_INCLUDES_LINT := $(subst -isystem ,-i,$(BCDS_XDK_EXT_INCLUDES))
BCDS_XDK_INCLUDES_LINT := $(subst -I,-i,$(BCDS_XDK_INCLUDES))
BCDS_LINT_CONFIG = -i$(BCDS_LINT_CONFIG_PATH)
#-------------------------------------------------------------------------------------------------					 