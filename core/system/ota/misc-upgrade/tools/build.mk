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

##################################################################################
include $(ENV_CLEAR)

TARGET_SRC := \
	write_misc.c \
	misc_message.c

TARGET_INC := \
	$(LICHEE_HOST_INCLUDE)

# and this var use to set system lib
TARGET_LDFLAGS += \
		-lpthread \
    -lrt \
    -ldl

TARGET_CPPFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions

TARGET_MODULE := write_misc
include $(BUILD_BIN)

##################################################################################
include $(ENV_CLEAR)

TARGET_SRC := \
	read_misc.c \
	misc_message.c

TARGET_INC := \
	$(LICHEE_HOST_INCLUDE)

# and this var use to set system lib
TARGET_LDFLAGS += \
		-lpthread \
    -lrt \
    -ldl

TARGET_CPPFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions

TARGET_MODULE := read_misc
include $(BUILD_BIN)

##################################################################################
