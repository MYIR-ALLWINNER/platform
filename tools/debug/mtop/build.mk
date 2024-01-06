#
# 1. Set the path and clear environment
# 	TARGET_PATH := $(call my-dir)
# 	include $(ENV_CLEAR)
#
# 2. Set the source files and headers files
#	TARGET_SRC := xxx_1.c xxx_2.c
#	TARGET_INc := xxx_1.h xxx_2.h
#
# 3. Set the output target
#	TARGET_MODULE := xxx
#
# 4. Include the main makefile
#	include $(BUILD_BIN)
#
# Before include the build makefile, you can set the compilaion
# flags, e.g. TARGET_ASFLAGS TARGET_CFLAGS TARGET_CPPFLAGS
#

# 1. Set the path and clear environment
LOCAL_PATH:= $(call my-dir)

# 2. Set the source files and headers files
include $(CLEAR_VARS)
TARGET_SRC += \
	mtop.c \
	check_hardware.c

TARGET_INC := \
	$(LICHEE_HOST_INCLUDE)

# and this var use to set system lib
TARGET_LDFLAGS += \
	-lpthread \
	-lcutils

TARGET_CPPFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions \
    -DHAS_BDROID_BUILDCFG -DLINUX_NATIVE -DANDROID_USE_LOGCAT=FALSE \
    -DDATABASE_IN_HOTPLUG_STORAGE -DSBC_FOR_EMBEDDED_LINUX -DONE_CAM

TARGET_MODULE := mtop
include $(BUILD_BIN)



