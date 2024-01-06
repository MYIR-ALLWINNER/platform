LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../config.mk
CEDARC = $(TOP)/frameworks/av/media/libcedarc/
CEDARX = $(LOCAL_PATH)/../../
################################################################################
## set flags for golobal compile and link setting.
################################################################################

CONFIG_FOR_COMPILE =
CONFIG_FOR_LINK =

LOCAL_CFLAGS += $(CONFIG_FOR_COMPILE)
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wno-unused-parameter -O0

## set the include path for compile flags.
LOCAL_SRC_FILES:= deinterlacedemo.c \
                  ditInterfac.c \
                  cdx_log.c \
                  deinterlace_new.c

LOCAL_C_INCLUDES := $(SourcePath)                             \
                    $(LOCAL_PATH)/../../                      \
                    $(CEDARC)/ve/include                      \
                    $(CEDARC)/memory/include                  \
                    $(CEDARC)/include/                        \
                    $(CEDARC)/vdecoder/include/               \
                    $(CEDARX)/libcore/base/                   \
                    $(CEDARX)/libcore/parser/include          \
                    $(CEDARX)/libcore/stream/include          \
                    $(CEDARX)/libcore/playback/include        \
                    $(CEDARX)/libcore/base/include/           \
                    $(CEDARX)/libcore/common/iniparser/       \
                    $(CEDARX)/xplayer/include                 \
                    $(CEDARX)/external/include/adecoder/      \
                    $(CEDARX)/external/include/               \
                    $(CEDARX)/../../libcore/parser/include    \
                    $(CEDARX)/../../android_adapter/output/    \

LOCAL_SHARED_LIBRARIES := \
            libcutils       \
            libutils        \
            libMemAdapter   \
            libVE   \
            liblog   \

#LOCAL_32_BIT_ONLY := true
LOCAL_MODULE:= deinterlacedemo

include $(BUILD_EXECUTABLE)
