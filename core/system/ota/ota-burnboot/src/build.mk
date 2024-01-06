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
	$(TARGET_PATH)/include/*, \
	$(LICHEE_HOST_INCLUDE) \
)

##################################################################################
include $(ENV_CLEAR)

TARGET_SRC := \
	BurnNandBoot.c \
	BurnSdBoot.c \
	OTA_BurnBoot.c \
	Utils.c

#TARGET_SRC += arch-arm/memset32.S
TARGET_INC := $(LICHEE_HOST_INCLUDE)
TARGET_CFLAGS += \
	-O2 -fPIC -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_LDFLAGS += \
	-lpthread \
	-shared -Wl,-Bsymbolic -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib
TARGET_MODULE := libota-burnboot
include $(BUILD_SHARED_LIB)

##################################################################################
include $(ENV_CLEAR)

TARGET_SRC := \
	ota-burnboot0.c

TARGET_INC := \
	$(LICHEE_HOST_INCLUDE)

TARGET_SHARED_LIB := \
    libota-burnboot

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

TARGET_MODULE := ota-burnboot0
include $(BUILD_BIN)

##################################################################################
include $(ENV_CLEAR)

TARGET_SRC := \
	ota-burnuboot.c

TARGET_INC := \
	$(LICHEE_HOST_INCLUDE)

TARGET_SHARED_LIB := \
    libota-burnboot

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

TARGET_MODULE := ota-burnuboot
include $(BUILD_BIN)

##################################################################################
