
LIBPATH := arm/glibc

ifeq ($(STATIC_AUDIOMIX),1)
LIB=$(LIBPATH)/libresamplerate.a
CFLAGS+=-fPIC
else
    ifeq ($(BR2_aarch64), "y")
        LIB=$(LIBPATH)/libresamplerate_64.so
    else
        LIB=$(LIBPATH)/libresamplerate.so
    endif
CFLAGS+=-fPIC
endif

## define commands for make, sush as all, clean
.PHONY: all clean cleanall
all:
	mkdir -p ./.lib
	cp -f $(LIB) ./.lib/

clean:
	rm -rf ./.lib

cleanall: clean
	-rm -rf ./.lib
