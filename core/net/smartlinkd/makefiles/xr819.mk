# Makefile for libclient

VPATH+= $(SRCDIR)/xr819
SRCS+= \
	control.c \
	main.c \
	scan.c \
	time_out.c \
	wifi_cntrl.c

CFLAGS+= -O2 -g -Wall -Winline -O2 -Wno-unused-value -Wno-pointer-sign
CFLAGS+=  -DUSE_AES
CFLAGS+= \
	-I$(SRCDIR)/include \
	-I$(SRCDIR)/libclient

LDFLAGS+= -L$(TARGET_DIR)/usr/lib/ -lpthread -lcrypto -ldl -L$(SRCDIR)/build-libclient -lsmartlinkd_client 
ALIB=$(SRCDIR)/xr819/lib/arm/libdecode.a

OBJS = $(SRCS:.c=.o)

TARGET_MODULE = smartlinkd_xrsc
all: $(TARGET_MODULE)

$(TARGET_MODULE): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(ALIB)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET_MODULE)
	rm -rf *.dep