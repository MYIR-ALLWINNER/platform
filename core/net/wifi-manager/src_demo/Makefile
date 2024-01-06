SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(SRCS:.c=.dep)

TARGET_MODULE = $(patsubst %.c, %, $(SRCS))
all:$(TARGET_MODULE)

-include $(DEPS)

$(TARGET_MODULE):%:%.c
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

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
