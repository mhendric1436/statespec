CLANG_FORMAT ?= clang-format
REPO_DIR := ..
DEFAULT_CLI := $(abspath $(REPO_DIR)/build/bin/statespec)
CLI ?= $(DEFAULT_CLI)

TEST_SCRIPTS := ast_cli_tests.sh formatter_cli_tests.sh validate_cli_tests.sh
TEST_DIRS := cli bindings

.PHONY: ensure-cli test-cli format format-check help

ensure-cli:
	@if [ "$(CLI)" = "$(DEFAULT_CLI)" ] && [ ! -x "$(CLI)" ]; then \
		$(MAKE) -C "$(REPO_DIR)" cli; \
	fi

test-cli: ensure-cli
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/$$script"; \
		(cd $(REPO_DIR) && sh "tests/$$script" "$(CLI)") || exit $$?; \
	done
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
	@echo "StateSpec tests targets:"
	@echo "  make test-cli     Run CLI shell tests"
	@echo "  make format       Format test fixtures"
	@echo "  make format-check Check test fixture formatting"
