# Generators

Generator declarations request target-specific output from a `.sspec` file. The language
model stays backend-neutral; generators decide how to lower the canonical model into
runtime artifacts.

## Generator Declaration

```statespec
generate mt
```

With options:

```statespec
generate mt {
  out "generated/mt"
  language cpp
  package com.example.orders
  runtime mt
}
```

The active CLI can also override the target and output directory:

```sh
./build/bin/statespec generate system.sspec mt --out build/generated/mt
```

## Supported Targets

| Target | Purpose |
|---|---|
| `mt` | Entity storage schemas, row metadata, and table mapping inputs. |
| `dl` | Distributed lock and lease metadata. |
| `qu` | Queue, message, and worker binding metadata. |
| `wf` | Workflow definition and workflow logic scaffolding. |
| `openapi` | OpenAPI contract output. |
| `proto` | Protobuf/gRPC scaffold target. |
| `docs` | Documentation scaffold target. |
| `tests` | Test scaffold target. |
| `all` | Expands to supported concrete targets. |

## Default Output Roots

When generated individually, concrete targets use target-specific roots:

```text
generated/mt/
generated/dl/
generated/qu/
generated/wf/
generated/openapi/
```

When using `all`, each target is still written to the same path it would use when run
individually.

```sh
./build/bin/statespec generate system.sspec all
```

Expected roots:

```text
generated/mt/
generated/dl/
generated/qu/
generated/wf/
generated/openapi/
```

With an explicit output root:

```sh
./build/bin/statespec generate system.sspec all --out build/generated
```

Expected roots:

```text
build/generated/mt/
build/generated/dl/
build/generated/qu/
build/generated/wf/
build/generated/openapi/
```

## `mt` Output

The `mt` target emits real `mt` codegen input for each entity:

```text
generated/mt/schemas/<entity>.mt.json
```

Those files are intended to follow the JSON Schema contract from the `mt` repository:

```text
schemas/mt-codegen.schema.json
```

The `mt` target also emits supporting metadata files:

```text
generated/mt/mt-manifest.yaml
generated/mt/mt_entities.hpp
generated/mt/mt_metadata.cpp
generated/mt/mt-state-machines.yaml
```

Example validation workflow when `mt` is checked out next to `statespec`:

```sh
./build/bin/statespec generate system.sspec mt --out build/generated/mt

for f in build/generated/mt/schemas/*.mt.json; do
  python3 -m jsonschema -i "$f" ../mt/schemas/mt-codegen.schema.json
done
```

Example `mt` codegen consumption:

```sh
python3 ../mt/tools/mt_codegen.py \
  build/generated/mt/schemas/order.mt.json \
  -o build/generated/mt/order.hpp
```

## Authoring Guidelines

- Keep generator declarations near the end of the `system` block.
- Prefer `generate all` for end-to-end examples.
- Prefer explicit single-target generation while developing one target.
- Do not put runtime implementation symbols in the canonical model just to satisfy a generator.
- Let generators lower the same canonical declarations into runtime-specific files.
