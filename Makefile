CXX ?= clang++
CLANG_FORMAT ?= clang-format
PLANTUML ?= plantuml
PLANTUML_FLAGS ?= -DPLANTUML_LIMIT_SIZE=16384 -Xmx512m
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Iinclude -Ithird_party
LDFLAGS ?=

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB := $(BUILD_DIR)/libstatespec.a
CLI := $(BIN_DIR)/statespec
TEST_BIN := $(BIN_DIR)/statespec_tests
DIAGRAMS_DIR := diagrams
PUML_FILES := $(wildcard $(DIAGRAMS_DIR)/*.puml)
PNG_FILES := $(PUML_FILES:.puml=.png)

HEADERS := $(wildcard include/statespec/*.hpp)
SRC_ALL := $(wildcard src/*.cpp)
SRC := $(SRC_ALL)
CLI_SRC := $(wildcard cmd/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
CATCH_SRC := third_party/catch2/catch_amalgamated.cpp
FORMAT_FILES := $(HEADERS) $(SRC_ALL) $(wildcard src/*.hpp) $(CLI_SRC) $(TEST_SRC)

OBJ := $(patsubst src/%.cpp,$(OBJ_DIR)/src/%.o,$(SRC))
CLI_OBJ := $(patsubst cmd/%.cpp,$(OBJ_DIR)/cmd/%.o,$(CLI_SRC))
TEST_OBJ := $(patsubst tests/%.cpp,$(OBJ_DIR)/tests/%.o,$(TEST_SRC))
CATCH_OBJ := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CATCH_SRC))

DEPS := $(OBJ:.o=.d) $(CLI_OBJ:.o=.d) $(TEST_OBJ:.o=.d) $(CATCH_OBJ:.o=.d)

.PHONY: all build cli check-build-tools test test-cli test-bindings test-bindings-cpp test-bindings-go test-bindings-java test-bindings-rust format format-bindings format-bindings-cpp format-bindings-go format-bindings-java format-bindings-rust format-check diagrams-png clean help print-files

all: test cli

build: $(LIB)

cli: $(CLI)

$(LIB): $(OBJ)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(CLI): $(CLI_OBJ) $(LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJ) $(CATCH_OBJ) $(LIB)
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

$(OBJ_DIR)/third_party/catch2/%.o: third_party/catch2/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

check-build-tools:
	@missing=0; \
	check_tool() { \
		name="$$1"; \
		cmd="$$2"; \
		version_cmd="$$3"; \
		if command -v "$$cmd" >/dev/null 2>&1; then \
			echo "[ok] $$name: $$($$version_cmd 2>&1 | head -n 1)"; \
		else \
			echo "[missing] $$name: $$cmd"; \
			missing=1; \
		fi; \
	}; \
	check_tool "C++ compiler" "$(CXX)" "$(CXX) --version"; \
	check_tool "Go" "go" "go version"; \
	check_tool "Java runtime" "java" "java -version"; \
	check_tool "Java compiler" "javac" "javac -version"; \
	check_tool "Rust compiler" "rustc" "rustc --version"; \
	check_tool "Cargo" "cargo" "cargo --version"; \
	if [ "$$missing" -ne 0 ]; then \
		echo ""; \
		echo "Install missing tools using docs/build-tools.md"; \
		exit 1; \
	fi

test: $(TEST_BIN) test-cli
	$(TEST_BIN)

test-cli: $(CLI)
	$(MAKE) -C tests test-cli CLI="$(abspath $(CLI))"

test-bindings: test-bindings-cpp test-bindings-go test-bindings-java test-bindings-rust

test-bindings-cpp:
	$(MAKE) -C bindings/cpp test

test-bindings-go:
	$(MAKE) -C bindings/go test

test-bindings-java:
	$(MAKE) -C bindings/java test

test-bindings-rust:
	$(MAKE) -C bindings/rust test

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)
	$(MAKE) -C tests format
	$(MAKE) format-bindings

format-bindings: format-bindings-cpp format-bindings-go format-bindings-java format-bindings-rust

format-bindings-cpp:
	$(MAKE) -C bindings/cpp format

format-bindings-go:
	$(MAKE) -C bindings/go format

format-bindings-java:
	$(MAKE) -C bindings/java format

format-bindings-rust:
	$(MAKE) -C bindings/rust format

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)
	$(MAKE) -C tests format-check

diagrams-png: $(PNG_FILES)

$(DIAGRAMS_DIR)/%.png: $(DIAGRAMS_DIR)/%.puml
	$(PLANTUML) $(PLANTUML_FLAGS) -tpng $<

print-files:
	@echo "HEADERS=$(HEADERS)"
	@echo "SRC_ALL=$(SRC_ALL)"
	@echo "SRC=$(SRC)"
	@echo "CLI_SRC=$(CLI_SRC)"
	@echo "TEST_SRC=$(TEST_SRC)"
	@echo "FORMAT_FILES=$(FORMAT_FILES)"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(DIAGRAMS_DIR)/*.png
	$(MAKE) -C bindings/cpp clean
	$(MAKE) -C bindings/go clean
	$(MAKE) -C bindings/java clean
	$(MAKE) -C bindings/rust clean

help:
	@echo "StateSpec Makefile targets:"
	@echo "  make                     Build and run tests, then build CLI"
	@echo "  make build               Build libstatespec.a"
	@echo "  make cli                 Build statespec CLI"
	@echo "  make check-build-tools   Check C++, Go, Java, and Rust build tools"
	@echo "  make test                Build and run core tests and CLI tests"
	@echo "  make test-bindings       Run all language binding tests"
	@echo "  make test-bindings-cpp   Run C++ binding tests"
	@echo "  make test-bindings-go    Run Go binding tests"
	@echo "  make test-bindings-java  Run Java binding tests"
	@echo "  make test-bindings-rust  Run Rust binding tests"
	@echo "  make format              Format core source and all language bindings"
	@echo "  make format-bindings     Format all language bindings"
	@echo "  make format-check        Check formatting without modifying files"
	@echo "  make diagrams-png        Generate PNG diagrams from diagrams/*.puml"
	@echo "  make clean               Remove build outputs"
	@echo "  make print-files         Show discovered source files"
	@echo "  make help                Show this help"
	@echo ""
	@echo "Binding-local builds are implemented in bindings/<language>/Makefile using language-native tooling where practical."
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CLANG_FORMAT=$(CLANG_FORMAT)"
	@echo "  PLANTUML=$(PLANTUML)"
	@echo "  PLANTUML_FLAGS=$(PLANTUML_FLAGS)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"

-include $(DEPS)
