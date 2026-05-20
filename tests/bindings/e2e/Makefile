REPO_DIR := ../../..
DEFAULT_CLI := $(abspath $(REPO_DIR)/build/bin/statespec)
CLI ?= $(DEFAULT_CLI)
TEST_DIRS := cpp go java rust

.PHONY: ensure-cli test-cli format format-check help

ensure-cli:
	@if [ "$(CLI)" = "$(DEFAULT_CLI)" ] && [ ! -x "$(CLI)" ]; then \
		$(MAKE) -C "$(REPO_DIR)" cli; \
	fi

test-cli: ensure-cli
	@for dir in $(TEST_DIRS); do \
		$(MAKE) -C "$$dir" test-cli CLI="$(CLI)" || exit $$?; \
	done

format:
	@for dir in $(TEST_DIRS); do \
		$(MAKE) -C "$$dir" format || exit $$?; \
	done

format-check:
	@for dir in $(TEST_DIRS); do \
		$(MAKE) -C "$$dir" format-check || exit $$?; \
	done

help:
	@echo "StateSpec generated binding E2E test targets:"
	@echo "  make test-cli     Run generated binding E2E shell tests"
	@echo "  make format       Format generated binding E2E fixtures"
	@echo "  make format-check Check generated binding E2E fixture formatting"
