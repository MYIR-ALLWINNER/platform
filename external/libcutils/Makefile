#SRCS = $(wildcard *.c)
SRCS += \
	hashmap.c \
	native_handle.c \
	config_utils.c \
	cpu_info.c \
	load_file.c \
	open_memstream.c \
	strdup16to8.c \
	strdup8to16.c \
	record_stream.c \
	process_name.c \
	threads.c \
	sched_policy.c \
	iosched_policy.c \
	str_parms.c \
	fs.c \
	multiuser.c \
	socket_inaddr_any_server.c \
	socket_local_client.c \
	socket_local_server.c \
	socket_loopback_client.c \
	socket_loopback_server.c \
	socket_network_client.c \
	sockets.c \
	android_reboot.c \
	debugger.c \
	klog.c \
	memory.c \
	misc_rw.c \
	partition_utils.c \
	properties.c \
	qtaguid.c \
	uevent.c

OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(SRCS:.c=.dep)

TARGET_MODULE = libcutils.so
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
