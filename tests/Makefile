CLANG_FORMAT ?= clang-format

TEST_SCRIPTS := ast_cli_tests.sh formatter_cli_tests.sh validate_cli_tests.sh
TEST_DIRS := cli bindings

.PHONY: test-cli format format-check help

test-cli:
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/$$script"; \
		(cd .. && sh "tests/$$script" "$(CLI)") || exit $$?; \
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
