#SRCS = $(wildcard *.c)
SRCS += \
	main.c \
	MtpServer.c \
	MtpPacket.c \
	MtpDataBase.c \
	MtpProperty.c \
	MtpStorage.c \
	MtpObjectInfo.c \
	MtpCommon.c \
	Disk/Disk.c \
	Disk/Dir.c \
	Disk/File.c \
	Disk/DiskDebug.c

OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(SRCS:.c=.dep)

TARGET_MODULE = mtp
all: $(TARGET_MODULE)

-include $(DEPS)

$(TARGET_MODULE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@  $(filter %.o, $^)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $(filter %.c, $^)

%.dep: %.c
	@echo "Creating $@ ..."
	@set -e; \
	rm -rf $@.tmp; \
	$(CC) -E -MM $(filter %.c, $^) > $@.tmp; \
	sed 's,\(.*\)\.o[ :]*,objs/\l.o $@: ,g' < $@.tmp > $@; \
	rm -rf $@.tmp

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET_MODULE)
	rm -rf *.dep
