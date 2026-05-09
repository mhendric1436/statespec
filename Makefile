CXX ?= clang++
CLANG_FORMAT ?= clang-format
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB := $(BUILD_DIR)/libstatespec.a
CLI := $(BIN_DIR)/statespec
TEST_BIN := $(BIN_DIR)/statespec_tests

HEADERS := $(wildcard include/statespec/*.hpp)
SRC_RETIRED := src/generator.cpp
SRC_ALL := $(filter-out $(SRC_RETIRED),$(wildcard src/*.cpp))
SRC := $(SRC_ALL)
CLI_SRC := $(wildcard cmd/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
TEST_SCRIPTS := $(wildcard tests/*_tests.sh)
FORMAT_FILES := $(HEADERS) $(SRC_ALL) $(wildcard src/*.hpp) $(CLI_SRC) $(TEST_SRC)

OBJ := $(patsubst src/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC))
CLI_OBJ := $(patsubst cmd/%.cpp,$(OBJ_DIR)/cmd/%.o,$(CLI_SRC))
TEST_OBJ := $(patsubst tests/%.cpp,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))

DEPS := $(OBJ:.o=.d) $(CLI_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all build cli test test-cli format format-check clean help print-files

all: test cli

build: $(LIB)

cli: $(CLI)

$(LIB): $(OBJ)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(CLI): $(CLI_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/cmd/%.o: cmd/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

test: $(TEST_BIN) test-cli
	$(TEST_BIN)

test-cli: $(CLI)
	@for script in $(TEST_SCRIPTS); do \
		echo "$$script"; \
		sh "$$script" "$(CLI)"; \
	done

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

print-files:
	@echo "HEADERS=$(HEADERS)"
	@echo "SRC_RETIRED=$(SRC_RETIRED)"
	@echo "SRC_ALL=$(SRC_ALL)"
	@echo "SRC=$(SRC)"
	@echo "CLI_SRC=$(CLI_SRC)"
	@echo "TEST_SRC=$(TEST_SRC)"
	@echo "TEST_SCRIPTS=$(TEST_SCRIPTS)"
	@echo "FORMAT_FILES=$(FORMAT_FILES)"

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "StateSpec Makefile targets:"
	@echo "  make              Build and run tests, then build CLI"
	@echo "  make build        Build libstatespec.a"
	@echo "  make cli          Build statespec CLI"
	@echo "  make test         Build and run smoke tests and CLI tests"
	@echo "  make test-cli     Build CLI and run shell-based CLI tests"
	@echo "  make format       Format source and header files with clang-format"
	@echo "  make format-check Check formatting without modifying files"
	@echo "  make clean        Remove build outputs"
	@echo "  make print-files  Show discovered source files"
	@echo "  make help         Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CLANG_FORMAT=$(CLANG_FORMAT)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"

-include $(DEPS)
