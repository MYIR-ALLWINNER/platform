#This confidential and proprietary software should be used
#under the licensing agreement from Allwinner Technology.

#Copyright (C) 2020-2030 Allwinner Technology Limited
#All rights reserved.

#Author: Albert zhengwanyu <zhengwanyu@allwinnertech.com>

#The entire notice above must be reproduced on all authorised
#copies and copies may only be made to the extent permitted
#by a licensing agreement from Allwinner Technology Limited.

CROSS_COMPILE := ../../../../out/t507/demo2.0/longan/buildroot/host/bin/aarch64-linux-gnu-
LIBS_DIR := -L../gpu_um_pub/mali-bifrost/fbdev/mali-g31/aarch64-linux-gnu/lib
HEADERS_DIR := -I./include \
	       -I../gpu_um_pub/mali-bifrost/include \
	       -I../gpu_um_pub/mali-bifrost/fbdev/include


CC := $(CROSS_COMPILE)gcc
LIBS := $(LIBS_DIR)
INCLUDE := $(HEADERS_DIR)

LDFLAGS := -lmali
DEFINES := -DDEBU -D__ARM__ -DUSE_OZONE -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES
#DEFINES := -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES  -DUSE_OZONE
#-DMESA_EGL_NO_X11_HEADERS

CFLAGS := -g -Wall -O3 $(DEFINES) $(INCLUDE) $(CFLAGS)

SRCS := $(wildcard *.c)
BINS := $(patsubst %.c, %, $(SRCS))

.c :

	$(CC) $(CFLAGS) -o $@ $<  $(LDFLAGS) $(LIBS)

all : $(BINS)

clean :
	rm -f $(BINS)
