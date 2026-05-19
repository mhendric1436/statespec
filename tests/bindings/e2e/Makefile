TEST_SCRIPTS := external_system_metadata_tests.sh
REPO_DIR := ../../..

.PHONY: test-cli format format-check help

test-cli:
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
