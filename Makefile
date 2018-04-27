SDK_DIR ?= sdk
VERSION ?= vdev

CFLAGS += -D'VERSION="${VERSION}"'

-include sdk/Makefile.mk

.PHONY: all
all: sdk
	@$(MAKE) -s debug

.PHONY: sdk
sdk:
	@if [ ! -f $(SDK_DIR)/Makefile.mk ]; then echo "Initializing Git submodule 'sdk'..."; git submodule update --init sdk; fi

.PHONY: update
update: sdk
	@echo "Updating Git submodule 'sdk'..."; git submodule update --remote --merge sdk
