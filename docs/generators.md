# Binding Generation

StateSpec generation is selected by CLI options. `.sspec` files do not contain
`generate` declarations; text remains the canonical system model, while tooling chooses
which binding package to emit.

## Command

```sh
statespec generate bindings --lang <cpp|go|java|rust> <file.sspec> [--out DIR]
```

When `--out` is omitted, the CLI writes to `generated/<language>`.

## Supported Languages

| Language | Default output root |
|---|---|
| `cpp` | `generated/cpp/` |
| `go` | `generated/go/` |
| `java` | `generated/java/` |
| `rust` | `generated/rust/` |

## Examples

```sh
./build/bin/statespec generate bindings --lang cpp system.sspec --out build/generated/cpp
./build/bin/statespec generate bindings --lang go system.sspec --out build/generated/go
./build/bin/statespec generate bindings --lang java system.sspec --out build/generated/java
./build/bin/statespec generate bindings --lang rust system.sspec --out build/generated/rust
```

## Generated Metadata

Binding generators consume the lowered StateSpec IR, not raw AST declarations. Feature
flags are currently emitted as descriptor metadata in each binding target so downstream
tools can inspect rollout configuration consistently.

Generated feature flag metadata includes:

- name
- type
- default value
- scope
- owner
- description
- expires

Runtime feature flag storage and evaluation are not generated yet. Generated descriptors
should be treated as passive metadata until runtime support is added.

## Authoring Guidelines

- Keep generation choices in build scripts, CI, or developer commands.
- Do not add implementation-specific generation declarations to `.sspec` files.
- Generated bindings should consume the same validated model across languages.
