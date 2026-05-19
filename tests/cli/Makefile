TEST_SCRIPTS := generate_args_tests.sh help_tests.sh include_generation_tests.sh openapi_tests.sh
REPO_DIR := ../..

.PHONY: test-cli format format-check help

test-cli:
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
