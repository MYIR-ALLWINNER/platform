# Set the path and clear environment
TARGET_PATH := $(call my-dir)

include $(ENV_CLEAR)

# Set the source files and headers files
TARGET_SRC := $(TARGET_PATH) \
	src/sound.c \

TARGET_CFLAGS += \
	-DHAVE_SYS_UIO_H \
	-DHAVE_IOCTL \
	-DANDROID_SMP=1 \
	-O2 -Wall -Wno-pointer-sign -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_INC := $(TARGET_PATH) \
	inc \
	../include \
	../include/platform/Tina \
	../libclient

TARGET_LDFLAGS += \
	-lpthread \
	-lasound \
	-L$(TARGET_PATH)/../libclient \
	-lsmartlinkd_client \
	-L$(TARGET_PATH)/lib \
	-lm

TARGET_LDFLAGS += \
	-Wl,--whole-archive \
	-lwifimg \
	-Wl,--no-whole-archive

TARGET_LOCAL_LDFLAGS += \
	-Wl,--whole-archive \
	-L$(TARGET_PATH)/lib \
	-lADT \
	-lwpa_client

# Set the output target
TARGET_MODULE := smartlinkd_soundwave

# Set the main makefile
include $(BUILD_BIN)

