# Makefile for libclient

VPATH+= $(SRCDIR)/server/core
SRCS+=main.cpp 
SRCS+=Thread.cpp 
SRCS+=Server.cpp 

VPATH+= $(SRCDIR)/server/proto
SRCS+=Proto.cpp 

VPATH+= $(SRCDIR)/server/proto/airkiss
SRCS+=Airkiss.cpp 

VPATH+= $(SRCDIR)/server/proto/cooee
SRCS+=Cooee.cpp 

VPATH+= $(SRCDIR)/server/proto/adt
SRCS+=Adt.cpp 

VPATH+= $(SRCDIR)/server/proto/xrsc
SRCS+=xrsc.cpp 

VPATH+= $(SRCDIR)/server/platform/tina
SRCS+=TinaWifiNetwork.cpp

CPPFLAGS+= -Wno-write-strings
CPPFLAGS+= -I$(SRCDIR)/server/platform
CPPFLAGS+= -I$(SRCDIR)/server/platform/tina
CPPFLAGS+= -I$(SRCDIR)/server/core
CPPFLAGS+= -I$(SRCDIR)/server/proto
CPPFLAGS+= -I$(SRCDIR)/server/proto/airkiss
CPPFLAGS+= -I$(SRCDIR)/server/proto/cooee
CPPFLAGS+= -I$(SRCDIR)/server/proto/adt
CPPFLAGS+= -I$(SRCDIR)/server/proto/xrsc
CPPFLAGS+= -I$(SRCDIR)/include
CPPFLAGS+= -I$(SRCDIR)/include/platform/Tina

LDFLAGS+= -L$(TARGET_DIR)/usr/lib/ -lpthread 

#LIBS+= -lpthread

OBJS = $(SRCS:.cpp=.o)

TARGET_MODULE = smartlinkd
all: $(TARGET_MODULE)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

$(TARGET_MODULE): $(OBJS) $(SUB_LIB)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET_MODULE)
	rm -rf *.dep