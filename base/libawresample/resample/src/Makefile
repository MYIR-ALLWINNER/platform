TARGET := libawresample.so

SRC :=  ./do_audioresample.c

all: $(TARGET) test

$(TARGET) : $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) libresamplerate.a -shared -fPIC

#test : ./test.c
#	$(CC) -o $@ $^ -lawresample -L.

clean:
	rm -rf *.o $(TARGET)
