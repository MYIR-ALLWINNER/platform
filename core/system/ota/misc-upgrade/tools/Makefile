TARGET1 = write_misc
TARGET2 = read_misc

all: $(TARGET1) $(TARGET2)

$(warning "misc--1--")

$(TARGET1) : write_misc.c misc_message.h misc_message.c
	$(CC) $^ -o $@ -I.

$(warning "misc--2--")

$(TARGET2) : read_misc.c misc_message.h misc_message.c
	$(CC) $^ -o $@ -I.

$(warning "misc--3--")

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET1) $(TARGET2)