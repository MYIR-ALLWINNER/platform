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

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_down.c

TARGET_CFLAGS += \
	-DHAVE_SYS_UIO_H \
	-DHAVE_IOCTL \
	-DANDROID_SMP=1 \
	-O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_INC := $(LICHEE_HOST_INCLUDE)
TARGET_LDFLAGS += \
	 -lsoftap -lssl
TARGET_MODULE := softap_down
include $(BUILD_BIN)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_longtime_test.c

TARGET_INC := $(LICHEE_HOST_INCLUDE)

TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl \
	-lsoftap \
	-lssl

TARGET_CFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions

TARGET_MODULE := softap_longtime_test
include $(BUILD_BIN)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_test.c

TARGET_INC := $(LICHEE_HOST_INCLUDE)

TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl \
	-lsoftap \
	-lssl

TARGET_CFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions

TARGET_MODULE := softap_test
include $(BUILD_BIN)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
	softap_up.c

TARGET_INC := $(LICHEE_HOST_INCLUDE)

TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl \
	-lsoftap \
	-lssl

TARGET_CFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions

TARGET_MODULE := softap_up
include $(BUILD_BIN)

#########################################
