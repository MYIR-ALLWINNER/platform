nativepower_path := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
include ${nativepower_path}/*/*.mk
