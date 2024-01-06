# Set the path and clear environment
TARGET_PATH := $(call my-dir)

include $(ENV_CLEAR)

# Set the source files and headers files
TARGET_SRC := \
	control.c \
	main.c \
	scan.c \
	time_out.c \
	wifi_cntrl.c

TARGET_INC := \
	./ \
	../include \
	../libclient

TARGET_LDFLAGS += \
	-lpthread \
	-lcrypto \
	-ldl \
	-lsmartlinkd_client \
	-L$(TARGET_PATH)/lib/arm \
	-ldecode

TARGET_CFLAGS += \
	-Wall -Winline -O2 -DUSE_AES \
	-Wno-unused-value \
	-Wno-pointer-sign
# Set the output target  
TARGET_MODULE := smartlinkd_xrsc

# Set the main makefile 
include $(BUILD_BIN)

