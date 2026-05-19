TEST_SCRIPTS := generate_args_tests.sh help_tests.sh include_generation_tests.sh openapi_tests.sh
REPO_DIR := ../..
DEFAULT_CLI := $(abspath $(REPO_DIR)/build/bin/statespec)
CLI ?= $(DEFAULT_CLI)

.PHONY: ensure-cli test-cli format format-check help

ensure-cli:
	@if [ "$(CLI)" = "$(DEFAULT_CLI)" ] && [ ! -x "$(CLI)" ]; then \
		$(MAKE) -C "$(REPO_DIR)" cli; \
	fi

test-cli: ensure-cli
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/cli/$$script"; \
		(cd $(REPO_DIR) && sh "tests/cli/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@:

format-check:
	@:

help:
	@echo "StateSpec CLI test targets:"
	@echo "  make test-cli     Run nested CLI shell tests"
	@echo "  make format       No-op"
	@echo "  make format-check No-op"
