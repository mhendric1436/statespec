CARGO ?= cargo
CARGO_TARGET_DIR ?= ../../build/rust-target

.PHONY: test format clean help

test:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) test

format:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) fmt

clean:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) clean
	rm -f Cargo.lock

help:
	@echo "StateSpec Rust binding targets:"
	@echo "  make test   Run Rust binding tests"
	@echo "  make format Format Rust binding source files"
	@echo "  make clean  Remove Cargo build outputs"
