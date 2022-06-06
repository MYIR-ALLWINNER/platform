ota_path := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
include ${ota_path}/*/*.mk