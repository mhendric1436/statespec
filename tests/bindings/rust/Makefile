TEST_SCRIPTS := generate_tests.sh
FORMAT_FILES := $(wildcard *.rs)
REPO_DIR := ../../..
DEFAULT_CLI := $(abspath $(REPO_DIR)/build/bin/statespec)
CLI ?= $(DEFAULT_CLI)

.PHONY: ensure-cli test-cli format format-check help

ensure-cli:
	@if [ "$(CLI)" = "$(DEFAULT_CLI)" ] && [ ! -x "$(CLI)" ]; then \
		$(MAKE) -C "$(REPO_DIR)" cli; \
	fi

test-cli: ensure-cli
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/bindings/rust/$$script"; \
		(cd $(REPO_DIR) && sh "tests/bindings/rust/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@if [ -n "$(FORMAT_FILES)" ]; then rustfmt $(FORMAT_FILES); fi

format-check:
	@if [ -n "$(FORMAT_FILES)" ]; then rustfmt --check $(FORMAT_FILES); fi

help:
	@echo "StateSpec Rust generated binding test targets:"
	@echo "  make test-cli     Run Rust generated binding shell tests"
	@echo "  make format       Format Rust generated binding test fixtures"
	@echo "  make format-check Check Rust generated binding test fixture formatting"
