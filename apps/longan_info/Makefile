OBJS = longan_info.o
OUT_BIN = longan_info

all:$(OBJS)
	$(CC) $(LDFLAGS) -g -o $(OUT_BIN) $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(OUT_BIN)
	rm -rf $(shell find -name "*.d")
