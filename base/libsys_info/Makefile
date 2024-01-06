SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(SRCS:.c=.dep)

TARGET_MODULE = libsys_info.so
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
