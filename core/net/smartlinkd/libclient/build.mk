# Set the path and clear environment
TARGET_PATH := $(call my-dir)

include $(ENV_CLEAR)

# Set the source files and headers files
TARGET_SRC := \
	aw_smartlinkd_connect.c

TARGET_INC := \
	./ \
	../include \
	../include/platform/Tina

# Set the output target  
TARGET_MODULE := libsmartlinkd_client

# Set the main makefile 
include $(BUILD_SHARED_LIB)

