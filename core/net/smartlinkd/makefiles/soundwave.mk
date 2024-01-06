# Makefile for libclient

VPATH+= $(SRCDIR)/soundwave/src
SRCS+= sound.c

CFLAGS+= -O2 -g -Wall -Wno-pointer-sign -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable
CFLAGS+= -DHAVE_SYS_UIO_H -DHAVE_IOCTL -DANDROID_SMP=1
CFLAGS+= -I$(SRCDIR)/include
CFLAGS+= -I$(SRCDIR)/include/platform/Tina
CFLAGS+= -I$(SRCDIR)/libclient
CFLAGS+= -I$(SRCDIR)/soundwave/inc

LDFLAGS+= -L$(TARGET_DIR)/usr/lib/ -lpthread -lasound -lm -lwpa_client -lwifimg -luci -L$(SRCDIR)/build-libclient -lsmartlinkd_client
ALIB=$(SRCDIR)/soundwave/lib/libADT.a

OBJS = $(SRCS:.c=.o)

TARGET_MODULE = smartlinkd_soundwave
all: $(TARGET_MODULE)

$(TARGET_MODULE): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(ALIB)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET_MODULE)
	rm -rf *.dep