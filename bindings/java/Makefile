JAVA ?= java
JAVAC ?= javac
CLANG_FORMAT ?= clang-format
JAVA_SOURCES := $(shell find com -name '*.java')

.PHONY: test format clean help

test:
	$(JAVAC) $(JAVA_SOURCES)
	$(JAVA) com.statespec.backend.JsonTest

format:
	$(CLANG_FORMAT) -i $(JAVA_SOURCES)

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
