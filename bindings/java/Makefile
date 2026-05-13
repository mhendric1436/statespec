JAVA ?= java
JAVAC ?= javac
JAVA_SOURCES := $(shell find com -name '*.java')

.PHONY: test clean help

test:
	$(JAVAC) $(JAVA_SOURCES)
	$(JAVA) com.statespec.backend.JsonTest

clean:
	find . -name '*.class' -delete

help:
	@echo "StateSpec Java binding targets:"
	@echo "  make test   Compile and run Java binding tests"
	@echo "  make clean  Remove generated .class files"
