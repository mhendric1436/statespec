# 🤝 Contributing to StateSpec

Thank you for your interest in contributing to StateSpec.

StateSpec is a canonical design language for distributed systems. Contributions should prioritize clarity, determinism, and structure.

---

# 🧭 Contribution Philosophy

StateSpec is intentionally opinionated.

Every feature must reduce ambiguity, not introduce it.

Before contributing, ask:

- Does this reduce or increase entropy?
- Does this introduce multiple ways to express the same concept?
- Can this be validated at compile time?
- Will this improve code generation consistency?

If unclear, open a discussion first.

---

# 📂 Repository Structure Overview

statespec/
├── grammar/
├── crates/
├── packages/
├── examples/
├── docs/
├── testdata/

---

# 🚀 Getting Started

## Prerequisites

- Rust (latest stable)
- Node.js
- pnpm or npm

## Build

make build

## Test

make test

## Format

make fmt

---

# 🧪 Testing Guidelines

All changes must include tests.

### Grammar tests
grammar/testdata/

### Semantic tests
testdata/semantic/

### Formatter tests
testdata/formatter/

### Generator tests
testdata/generators/

### Example validation

statespec validate examples/

---

# 🧱 Language Changes

1. Open issue (language-proposal)
2. Describe problem and solution
3. Get feedback
4. Submit PR with:
   - grammar updates
   - semantic validation
   - formatter support
   - docs updates
   - examples

---

# 🧩 Entity and Workflow Rules

Entities:
- must declare key
- should declare ownership
- must define state if used in workflows

Relationships:
- child declares parent
- no implicit relationships

Workflows:
- must operate on entities
- must be restart-safe

---

# 🔄 Orchestration Pattern

Standard pattern:

1. generate_child_ids
2. creating_children
3. waiting_children

Do not introduce alternative hidden orchestration patterns.

---

# 💻 VS Code Extension

packages/vscode-statespec/

Rules:
- must use shared parser/semantic core
- no duplicated logic

---

# ⚙️ CLI Contributions

crates/statespec-cli/

Commands:

statespec validate
statespec fmt
statespec generate
statespec graph

---

# 🧠 Formatter Rules

- canonical output only
- no user-configurable styles

---

# 📦 Generators

- consume IR only
- deterministic output
- include golden tests

---

# 🧹 Code Style

Rust:
- rustfmt
- clippy clean

TypeScript:
- ESLint clean

---

# 🛑 What Not to Contribute

- ambiguous features
- multiple syntax forms
- hidden semantics
- partial implementations

---

# 🧪 Pull Request Checklist

- tests added
- docs updated
- examples updated
- formatter updated
- CI passing

---

# 📜 License

Apache 2.0 (recommended)

---

# 🚀 Final Note

StateSpec is a precision tool.

If a feature reduces clarity, it does not belong.

Thanks for contributing.
