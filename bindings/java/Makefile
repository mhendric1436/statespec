JAVA ?= java
JAVAC ?= javac
CLANG_FORMAT ?= clang-format
JAVA_SOURCES := $(shell find com -name '*.java')
FORMAT_FILES := $(JAVA_SOURCES) generated/ExternalSystemCallAdapters.java.tmpl generated/ExternalSystemRuntime.java.tmpl

.PHONY: test format clean help

test:
	$(JAVAC) $(JAVA_SOURCES)
	$(JAVA) com.statespec.backend.JsonTest

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

clean:
	find . -name '*.class' -delete

help:
	@echo "StateSpec Java binding targets:"
	@echo "  make test   Compile and run Java binding tests"
	@echo "  make format Format Java binding source files"
	@echo "  make clean  Remove generated .class files"
	@echo ""
	@echo "Variables:"
	@echo "  CLANG_FORMAT=$(CLANG_FORMAT)"
