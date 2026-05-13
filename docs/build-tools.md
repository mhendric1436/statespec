# Build Tool Installation

This document describes how to install the language-specific build tools used by the
StateSpec reference bindings.

The bindings intentionally follow language-native build and test patterns:

| Language | Primary tooling |
|---|---|
| C++ | Make + clang++ |
| Go | go test |
| Java | javac + java |
| Rust | cargo |

The top-level `Makefile` orchestrates binding-local builds and tests.

---

## macOS

### C++

Install Xcode command-line tools:

```sh
xcode-select --install
```

Optional Homebrew tooling:

```sh
brew install llvm make
```

Verify:

```sh
clang++ --version
make --version
```

### Go

Install:

```sh
brew install go
```

Verify:

```sh
go version
```

### Java

Install:

```sh
brew install openjdk
```

Verify:

```sh
java -version
javac -version
```

### Rust

Install:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Verify:

```sh
rustc --version
cargo --version
```

---

## Ubuntu / Debian

### C++

Install:

```sh
sudo apt update
sudo apt install -y build-essential clang make
```

Verify:

```sh
clang++ --version
make --version
```

### Go

Install:

```sh
sudo apt install -y golang-go
```

Verify:

```sh
go version
```

### Java

Install:

```sh
sudo apt install -y openjdk-21-jdk
```

Verify:

```sh
java -version
javac -version
```

### Rust

Install:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Verify:

```sh
rustc --version
cargo --version
```

---

## Fedora

### C++

Install:

```sh
sudo dnf install -y clang make gcc-c++
```

### Go

Install:

```sh
sudo dnf install -y golang
```

### Java

Install:

```sh
sudo dnf install -y java-21-openjdk-devel
```

### Rust

Install:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Verify:

```sh
clang++ --version
make --version
go version
java -version
javac -version
rustc --version
cargo --version
```

---

## Binding Tests

Run all binding tests:

```sh
make test-bindings
```

Run individual binding tests:

```sh
make test-bindings-cpp
make test-bindings-go
make test-bindings-java
make test-bindings-rust
```

---

## Binding-Local Build Commands

Each binding directory contains its own build configuration.

### C++

```sh
cd bindings/cpp
make test
```

### Go

```sh
cd bindings/go
make test
```

### Java

```sh
cd bindings/java
make test
```

### Rust

```sh
cd bindings/rust
make test
```

---

## Philosophy

StateSpec intentionally uses language-native tooling wherever practical.

The repository-level `Makefile` orchestrates builds and tests across bindings, while
binding-local build files own language-specific logic.
