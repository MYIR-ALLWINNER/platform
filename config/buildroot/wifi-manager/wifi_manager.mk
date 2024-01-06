wifi_manager_path := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))
include ${wifi_manager_path}/*/*.mk
