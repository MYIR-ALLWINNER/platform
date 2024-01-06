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
TARGET_PATH :=$(call my-dir)
$(call copy-files-under, \
	$(TARGET_PATH)/include/command_listener.h, \
	$(LICHEE_HOST_INCLUDE)/wifi \
)

$(call copy-files-under, \
	$(TARGET_PATH)/include/response_code.h, \
	$(LICHEE_HOST_INCLUDE)/wifi \
)

$(call copy-files-under, \
	$(TARGET_PATH)/include/aw_softap_intf.h, \
	$(LICHEE_HOST_INCLUDE)/wifi \
)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_interface.c \
	wifi.c \
	netd_softap_controller.c \
	command_listener.c

TARGET_INC := $(TARGET_PATH)/include
TARGET_CFLAGS += \
	-DHAVE_SYS_UIO_H \
	-DHAVE_IOCTL \
	-DANDROID_SMP=1
TARGET_CFLAGS := -O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_LDFLAGS += -lpthread -static
TARGET_MODULE := libsoftap
include $(BUILD_STATIC_LIB)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_interface.c \
	wifi.c \
	netd_softap_controller.c \
	command_listener.c

TARGET_INC := $(TARGET_PATH)/include
TARGET_CFLAGS += \
	-DHAVE_SYS_UIO_H \
	-DHAVE_IOCTL \
	-DANDROID_SMP=1
TARGET_CFLAGS := -O2 -fPIC -Wall -ansi -pedantic

TARGET_LDFLAGS += \
	-lpthread \
	-ldl \
	-shared -Wl,-Bsymbolic -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib
TARGET_MODULE := libsoftap
include $(BUILD_SHARED_LIB)

