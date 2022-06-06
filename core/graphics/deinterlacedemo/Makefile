############################################################################
# note: before make you should modify you compile path                      #
############################################################################

#命令显示
ifneq ($V,1)
Q ?= @
endif

ifeq ($D,1)
DEBUG   = -g -O0
else
DEBUG   = -O3 -fvisibility=hidden -s
endif

COMPILEPATH := /home/yezixing/workspace/t5-linux4.9/kunos/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin

INCLUDEPATH := /home/yezixing/workspace/t5-linux4.9/kunos/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/aarch64-linux-gnu/libc/usr/include

INSTALLPATH := ./out

CC  := $(COMPILEPATH)/aarch64-linux-gnu-gcc
CPP := $(COMPILEPATH)/aarch64-linux-gnu-g++
AS := $(COMPILEPATH)/aarch64-linux-gnu-as

INCLUDE = -I$(INCLUDEPATH) -I$(PWD) -I/home/yezixing/workspace/t5-linux4.9/kunos/platform/framework/libcedarc/include \
            -I/home/yezixing/workspace/t5-linux4.9/kunos/platform/framework/cedarx/libcore/base/include


CFLAGS = $(DEBUG) -Wall -fexceptions  -s -ffunction-sections -fdata-sections
CPPFLAGS = $(DEBUG) -Wall -fexceptions -std=c++11 -c -s -ffunction-sections \
           -fdata-sections -fpermissive -Wno-packed-bitfield-compat -DFPGA -DDECODER_ONLY

LDFLAGS = -L/home/yezixing/workspace/t5-linux4.9/kunos/out/t507/demo2.0/longan/buildroot/target/usr/lib -lVE -lMemAdapter \
          -lcdc_base

LDLIBS  =

###############################################################################

SRC1 =  ditInterfac.c \
        deinterlace_new.c \
        cdx_log.c  \
        deinterlacedemo.c

TAGET1 = deinterlacedemo

OBJ1 = $(SRC1:.c=.o) #$(SRC1:.S=.o)
NOTDIR_OBJ1 = $(notdir $(OBJ1))
SUFFIX_OBJ1 = $(addprefix $(INSTALLPATH)/, $(NOTDIR_OBJ1))

all: out build

out:
	mkdir -p $(INSTALLPATH)
	echo $(NOTDIR_OBJ1)
	echo $(SUFFIX_OBJ1)

build:$(OBJ1)
	@echo build [$(TAGET1)]
	$(CPP)  -o $(INSTALLPATH)/$(TAGET1) $(SUFFIX_OBJ1) $(LDLIBS)  $(INCLUDE) $(LDFLAGS)

.c.o:
	@echo build [$(notdir $@)]
	$Q $(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $(INSTALLPATH)/$(notdir $@)

.cc.o:
	@echo build [$(notdir $@)]
	$Q $(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $(INSTALLPATH)/$(notdir $@)

#.S.o:
#   @echo build [$(notdir $@)]
#   $Q $(AS) $< -o $(INSTALLPATH)/$(notdir $@)

.PHONY: clean
clean:
	$Q rm -rf $(INSTALLPATH)
