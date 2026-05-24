CARGO ?= cargo
CARGO_TARGET_DIR ?= ../../build/rust-target
RUSTFMT ?= rustfmt
GENERATED_FORMAT_TEMPLATES := $(filter-out generated/descriptor_module.rs.tmpl generated/descriptors.rs.tmpl generated/lib.rs.tmpl generated/runtime_registration.rs.tmpl,$(wildcard generated/*.rs.tmpl))

.PHONY: test format clean help

test:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) test

format:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) fmt
	$(RUSTFMT) $(GENERATED_FORMAT_TEMPLATES)

clean:
	CARGO_TARGET_DIR=$(CARGO_TARGET_DIR) $(CARGO) clean
	rm -f Cargo.lock

help:
	@echo "StateSpec Rust binding targets:"
	@echo "  make test   Run Rust binding tests"
	@echo "  make format Format Rust binding source files"
	@echo "  make clean  Remove Cargo build outputs"
