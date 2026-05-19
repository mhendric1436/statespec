TEST_SCRIPTS := generate_tests.sh
FORMAT_FILES := $(wildcard *.go)
REPO_DIR := ../../..

.PHONY: test-cli format format-check help

test-cli:
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/bindings/go/$$script"; \
		(cd $(REPO_DIR) && sh "tests/bindings/go/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@if [ -n "$(FORMAT_FILES)" ]; then gofmt -w $(FORMAT_FILES); fi

format-check:
	@if [ -n "$(FORMAT_FILES)" ]; then test -z "$$(gofmt -l $(FORMAT_FILES))"; fi

help:
	@echo "StateSpec Go generated binding test targets:"
	@echo "  make test-cli     Run Go generated binding shell tests"
	@echo "  make format       Format Go generated binding test fixtures"
	@echo "  make format-check Check Go generated binding test fixture formatting"
