# Makefile for realtek bluetooth

VPATH+= $(SRCDIR)/realtek/rtl8723ds/rtk_hciattach

SRCS+= \
	hciattach_rtk.c \
	hciattach.c

CFLAGS+= -O2 -g -Wall -Wno-unused-parameter -Wno-pointer-sign
CFLAGS+= -I$(SRCDIR)/realtek/rtl8723ds/rtk_hciattach

LIBS+= -lpthread

OBJS= $(patsubst %, %.o, $(basename $(SRCS)))

all: rtk_hciattach

rtk_hciattach: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) rtk_hciattach