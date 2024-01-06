SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
DEPS = $(SRCS:.c=.dep)
OUT_BIN = initplay

all: $(OUT_BIN)

#ifndef ($(MAKECMDGOALS), clean)
-include $(DEPS)
#endif

$(OUT_BIN): $(OBJS)
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
	rm -rf $(OUT_PUT)
	rm -rf *.dep
