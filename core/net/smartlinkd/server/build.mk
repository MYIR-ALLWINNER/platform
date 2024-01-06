# Set the path and clear environment
TARGET_PATH := $(call my-dir)

include $(ENV_CLEAR)

# Set the source files and headers files
TARGET_SRC := \
	core/main.cpp \
	core/Thread.cpp \
	core/Server.cpp \
	proto/Proto.cpp \
	proto/airkiss/Airkiss.cpp \
	proto/cooee/Cooee.cpp \
	proto/adt/Adt.cpp \
	proto/xrsc/xrsc.cpp \
	platform/tina/TinaWifiNetwork.cpp

TARGET_INC := \
	./ \
	./platform \
	./platform/tina \
	./core \
	./proto \
	./proto/airkiss \
	./proto/cooee \
	./proto/adt \
	./proto/xrsc \
	../include \
	../include/platform/Tina

TARGET_LDFLAGS += \
	-lpthread

TARGET_CPPFLAGS += \
	-Wno-write-strings

# Set the output target  
TARGET_MODULE := smartlinkd

# Set the main makefile 
include $(BUILD_BIN)

