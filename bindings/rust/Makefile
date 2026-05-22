CARGO ?= cargo
CARGO_TARGET_DIR ?= ../../build/rust-target
RUSTFMT ?= rustfmt

.PHONY: test format clean help

test:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) test

format:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) fmt
	$(RUSTFMT) generated/entity_repository.rs.tmpl generated/external_system_call_adapters.rs.tmpl generated/external_system_metadata_runtime.rs.tmpl generated/external_system_runtime.rs.tmpl

clean:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) clean
	rm -f Cargo.lock

help:
	@echo "StateSpec Rust binding targets:"
	@echo "  make test   Run Rust binding tests"
	@echo "  make format Format Rust binding source files"
	@echo "  make clean  Remove Cargo build outputs"
