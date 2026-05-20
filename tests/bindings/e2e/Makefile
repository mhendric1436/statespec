TEST_SCRIPTS := external_system_metadata_tests.sh canonical_api_worker_app_tests.sh
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
		echo "tests/bindings/e2e/$$script"; \
		(cd $(REPO_DIR) && sh "tests/bindings/e2e/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@:

format-check:
	@:

help:
	@echo "StateSpec generated binding E2E test targets:"
	@echo "  make test-cli     Run generated binding E2E shell tests"
	@echo "  make format       No-op"
	@echo "  make format-check No-op"
