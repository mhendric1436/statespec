CLANG_FORMAT ?= clang-format

TEST_SCRIPTS := generate_tests.sh
FORMAT_FILES := $(wildcard *.java)
REPO_DIR := ../../../..
DEFAULT_CLI := $(abspath $(REPO_DIR)/build/bin/statespec)
CLI ?= $(DEFAULT_CLI)

.PHONY: ensure-cli test-cli format format-check help

ensure-cli:
	@if [ "$(CLI)" = "$(DEFAULT_CLI)" ] && [ ! -x "$(CLI)" ]; then \
		$(MAKE) -C "$(REPO_DIR)" cli; \
	fi

test-cli: ensure-cli
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/bindings/e2e/java/$$script"; \
		(cd $(REPO_DIR) && sh "tests/bindings/e2e/java/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@if [ -n "$(FORMAT_FILES)" ]; then $(CLANG_FORMAT) -i $(FORMAT_FILES); fi

format-check:
	@if [ -n "$(FORMAT_FILES)" ]; then $(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES); fi

help:
	@echo "StateSpec Java generated app E2E targets:"
	@echo "  make test-cli     Run Java generated app E2E tests"
	@echo "  make format       Format Java generated app E2E fixtures"
	@echo "  make format-check Check Java generated app E2E fixture formatting"
