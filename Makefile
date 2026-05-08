CXX ?= clang++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB := $(BUILD_DIR)/libstatespec.a
CLI := $(BIN_DIR)/statespec
TEST_BIN := $(BIN_DIR)/statespec_tests

SRC := $(wildcard src/*.cpp)
OBJ := $(patsubst src/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC))

CLI_SRC := $(wildcard cmd/*.cpp)
CLI_OBJ := $(patsubst cmd/%.cpp,$(OBJ_DIR)/cmd/%.o,$(CLI_SRC))

TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(patsubst tests/%.cpp,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))

DEPS := $(OBJ:.o=.d) $(CLI_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

.PHONY: all build cli test clean help print-files

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

test: $(TEST_BIN)
	$(TEST_BIN)

print-files:
	@echo "SRC=$(SRC)"
	@echo "CLI_SRC=$(CLI_SRC)"
	@echo "TEST_SRC=$(TEST_SRC)"

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "StateSpec Makefile targets:"
	@echo "  make          Build and run tests, then build CLI"
	@echo "  make build    Build libstatespec.a"
	@echo "  make cli      Build statespec CLI"
	@echo "  make test     Build and run smoke tests"
	@echo "  make clean    Remove build outputs"
	@echo "  make print-files Show discovered source files"
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"

-include $(DEPS)
