# Makefile for libclient

VPATH+= $(SRCDIR)/libclient

SRCS+= aw_smartlinkd_connect.c

CFLAGS+= -O2 -g -fPIC -Wall -Wno-unused-parameter -Wno-pointer-sign
CFLAGS+= -I$(SRCDIR)/include -I$(SRCDIR)/include/platform/Tina

LDFLAGS+= -L$(TARGET_DIR)/usr/lib/ -lpthread -shared
	
#LIBS+= -lpthread

OBJS= $(patsubst %, %.o, $(basename $(SRCS)))

TARGET_MODULE = libsmartlinkd_client.so
all: $(TARGET_MODULE)

$(TARGET_MODULE): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET_MODULE)
	rm -rf *.dep