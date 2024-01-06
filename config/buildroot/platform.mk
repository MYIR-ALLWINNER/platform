PLATFORM_PATH := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
include ${PLATFORM_PATH}/demo/demo.mk
include ${PLATFORM_PATH}/mtop/mtop.mk
include ${PLATFORM_PATH}/longan_info/longan_info.mk
include ${PLATFORM_PATH}/libcedarc/libcedarc.mk

include ${PLATFORM_PATH}/cedarx/cedarx.mk
include ${PLATFORM_PATH}/boot-play/boot-play.mk
include ${PLATFORM_PATH}/mediaplayer/mediaplayer.mk
include ${PLATFORM_PATH}/nativepower/nativepower.mk
include ${PLATFORM_PATH}/ota/ota.mk
include ${PLATFORM_PATH}/libawresample/libawresample.mk
include ${PLATFORM_PATH}/libspeex-lite/libspeex_lite.mk
include ${PLATFORM_PATH}/resample/resample.mk
include ${PLATFORM_PATH}/libcutils/libcutils.mk
include ${PLATFORM_PATH}/libsys_info/libsys_info.mk

include ${PLATFORM_PATH}/mtp/mtp.mk
include ${PLATFORM_PATH}/logwrapper/logwrapper.mk
include ${PLATFORM_PATH}/wifi-manager/wifi_manager.mk
include ${PLATFORM_PATH}/smartlinkd/smartlinkd.mk
include ${PLATFORM_PATH}/bluetooth/bluetooth.mk
