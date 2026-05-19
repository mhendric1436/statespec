CLANG_FORMAT ?= clang-format

TEST_SCRIPTS := generate_tests.sh
FORMAT_FILES := $(wildcard *.cpp)
REPO_DIR := ../../..

.PHONY: test-cli format format-check help

test-cli:
	@for script in $(TEST_SCRIPTS); do \
		echo "tests/bindings/cpp/$$script"; \
		(cd $(REPO_DIR) && sh "tests/bindings/cpp/$$script" "$(CLI)") || exit $$?; \
	done

format:
	@if [ -n "$(FORMAT_FILES)" ]; then $(CLANG_FORMAT) -i $(FORMAT_FILES); fi

format-check:
	@if [ -n "$(FORMAT_FILES)" ]; then $(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES); fi

help:
	@echo "StateSpec C++ generated binding test targets:"
	@echo "  make test-cli     Run C++ generated binding shell tests"
	@echo "  make format       Format C++ generated binding test fixtures"
	@echo "  make format-check Check C++ generated binding test fixture formatting"
