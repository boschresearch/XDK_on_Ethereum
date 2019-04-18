export BCDS_COMMON_MAKEFILE=$(CURDIR)/common.mk

#List the shared package required for building XDK application below

BCDS_LIBS_LIST = \
	$(BCDS_SENSORS_UTILS_PATH);$(BCDS_SENSORS_UTILS_PATH)/debug/libSensorUtils_efm32_debug.a;$(BCDS_SENSORS_UTILS_PATH)/release/libSensorUtils_efm32.a \
	$(BCDS_SENSORS_PATH);$(BCDS_SENSORS_PATH)/debug/libSensors_efm32_debug.a;$(BCDS_SENSORS_PATH)/release/libSensors_efm32.a \
	$(BCDS_BSP_PATH);$(BCDS_BSP_PATH)/debug/libBSP_efm32_debug.a;$(BCDS_BSP_PATH)/release/libBSP_efm32.a \
	
	
#List the 3rd party library path required for building XDK application	

BCDS_LIBS_LIST += \
    $(BCDS_FREERTOS_PATH);$(BCDS_FREERTOS_PATH)/debug/libFreeRTOS_efm32_debug.a;$(BCDS_FREERTOS_PATH)/release/libFreeRTOS_efm32.a \
	$(BCDS_EMLIB_PATH);$(BCDS_EMLIB_PATH)/debug/libEMLib_efm32_debug.a;$(BCDS_EMLIB_PATH)/release/libEMLib_efm32.a \
	$(BCDS_BSTLIB_PATH);$(BCDS_BSTLIB_PATH)/debug/libBSTLib_efm32_debug.a;$(BCDS_BSTLIB_PATH)/release/libBSTLib_efm32.a \
	$(BCDS_WIFILIB_PATH);$(BCDS_WIFILIB_PATH)/debug/libWiFi_efm32_debug.a;$(BCDS_WIFILIB_PATH)/release/libWiFi_efm32.a \
	$(BCDS_FATFSLIB_PATH);$(BCDS_FATFSLIB_PATH)/debug/libFATfs_efm32_debug.a;$(BCDS_FATFSLIB_PATH)/release/libFATfs_efm32.a \
	$(BCDS_SERVALPAL_PATH);$(BCDS_SERVALPAL_PATH)/debug/libServalPal_efm32_debug.a;$(BCDS_SERVALPAL_PATH)/release/libServalPal_efm32.a
	
BCDS_THIRD_PARTY_LIBS= \
    $(BCDS_ESSENTIALS_PATH)/release/libEssentials_efm32.a \
    $(BCDS_UTILS_PATH)/release/libUtils_efm32.a \
    $(BCDS_DRIVERS_PATH)/release/libDrivers_efm32.a \
    $(BCDS_FOTA_PATH)/release/libFOTA_efm32.a \
    $(BCDS_WLAN_PATH)/release/libWlan_efm32.a \
    $(BCDS_BLE_PATH)/release/libBLE_efm32.a \
    $(BCDS_SENSOR_TOOLBOX_PATH)/release/libSensorToolbox_efm32.a \
	$(filter-out $(BCDS_BLE_LIB_PATH)/3rd-party/Alpwise/ALPW-BLESDKCM3/BLESW_CoreStack/Libraries/BLESW_CoreStack_Broadcaster_CM3.a, $(wildcard $(BCDS_BLE_LIB_PATH)/3rd-party/Alpwise/ALPW-BLESDKCM3/BLESW_*/Libraries/*.a)) \
	$(BCDS_EMLIB_PATH)/3rd-party/EMLib/CMSIS/Lib/GCC/libarm_cortexM3l_math.a \
	$(BCDS_BSX_LIB_PATH)/BSX4/Source/algo/algo_bsx/Lib/libalgobsxm3/libalgobsx.a \
    $(BCDS_LORA_DRIVERS_PATH)/release/libLoRaDrivers_efm32.a \
    
# Only enable mbedTLS if SEED_BUILD is set to prevent other project builds without mbedtls from crashing
ifeq ($(SEED_BUILD), TRUE)
   BCDS_THIRD_PARTY_LIBS += \
   $(MBEDTLS_LIBRARY_PATH)/source/libmbedcrypto.a 
   #$(MBEDTLS_LIBRARY_PATH)/source/libmedtls.a \
   #$(MBEDTLS_LIBRARY_PATH)/source/libmedx509.a 
endif

ifeq ($(SERVAL_ENABLE_TLS_ECC),0)
BCDS_THIRD_PARTY_LIBS += \
	$(BCDS_ESCRYPTLIB_PATH)/release/libEscrypt_CycurTLS_efm32.a 
else
BCDS_THIRD_PARTY_LIBS += \
	$(BCDS_ESCRYPTLIB_PATH)/release/libEscrypt_CycurTLS_RSA_WITH_AES_efm32.a 
endif 
 	
define rule_template 
.PHONY: $(1)
$(1):
	$(MAKE) -C $(2) $(3) BCDS_COMMON_MAKEFILE=$(BCDS_COMMON_MAKEFILE)
endef

$(foreach lib_paths, $(BCDS_LIBS_LIST), \
	$(eval lib_paths_space_separated = $(subst ;, ,$(lib_paths))) \
	$(eval makefile_path = $(word 1,$(lib_paths_space_separated))) \
	$(eval debug_lib_path = $(word 2,$(lib_paths_space_separated))) \
	$(eval release_lib_path = $(word 3,$(lib_paths_space_separated))) \
	\
	$(eval BCDS_LIBS_DEBUG_PATH += $(dir $(debug_lib_path))) \
	$(eval BCDS_LIBS_DEBUG += $(debug_lib_path)) \
	$(eval $(call rule_template,$(debug_lib_path),$(makefile_path),debug)) \
	\
	$(eval BCDS_LIBS_RELEASE_PATH += $(dir $(release_lib_path))) \
	$(eval BCDS_LIBS_RELEASE += $(release_lib_path)) \
	$(eval $(call rule_template, $(release_lib_path), $(makefile_path), release)) \
)	


clean_libraries: clean_Drivers clean_Wifi clean_BLE clean_Sensors clean_Essentials clean_BSP \
clean_Utils clean_ServalPAL clean_Emlib clean_LibWifi clean_LibOS clean_LibBST \
clean_SensorsUtils clean_SensorToolbox clean_FATfs clean_Fota clean_CycurLib clean_LoraDrivers

clean_SensorToolbox:
	$(MAKE) -C $(BCDS_SENSOR_TOOLBOX_PATH) clean
clean_Drivers:
	$(MAKE) -C $(BCDS_DRIVERS_PATH) clean
clean_LoraDrivers:
	$(MAKE) -C $(BCDS_LORA_DRIVERS_PATH) clean
clean_Wifi:
	$(MAKE) -C $(BCDS_WLAN_PATH) clean
clean_BLE:
	$(MAKE) -C $(BCDS_BLE_PATH) clean
clean_Sensors:	   
	$(MAKE) -C $(BCDS_SENSORS_PATH) clean
clean_Utils:
	$(MAKE) -C $(BCDS_UTILS_PATH) clean
clean_ServalPAL:
	$(MAKE) -C $(BCDS_SERVALPAL_PATH) clean
	
clean_Emlib:
	$(MAKE) -C $(BCDS_EMLIB_PATH) clean
clean_LibWifi:
	$(MAKE) -C $(BCDS_WIFILIB_PATH) clean
clean_LibOS:
	$(MAKE) -C $(BCDS_FREERTOS_PATH) clean
clean_LibBST:
	$(MAKE) -C $(BCDS_BSTLIB_PATH) clean
clean_FATfs:
	$(MAKE) -C $(BCDS_FATFSLIB_PATH) clean
clean_Fota:
	$(MAKE) -C $(BCDS_FOTA_PATH) clean
clean_CycurLib:
	$(MAKE) -C $(BCDS_ESCRYPTLIB_PATH) clean
clean_Essentials:
	$(MAKE) -C $(BCDS_ESSENTIALS_PATH) clean
clean_SensorsUtils:
	$(MAKE) -C $(BCDS_SENSORS_UTILS_PATH) clean 
clean_BSP:
	$(MAKE) -C $(BCDS_BSP_PATH) clean 
	