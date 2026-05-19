TEST_DIRS := cpp go java rust e2e

.PHONY: test-cli format format-check help

test-cli:
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
	@echo "StateSpec binding generated test targets:"
	@echo "  make test-cli     Run all generated binding shell tests"
	@echo "  make format       Format generated binding test fixtures"
	@echo "  make format-check Check generated binding test fixture formatting"
