# CFG Tracing Plan

This records the initial plan for adding control-flow graph tracing and
basic-block reconstruction to dasmxx.

## Scope

The initial feature is CFG reconstruction, not full partial evaluation.

The tracer should decode reachable instructions from known entry points, form
basic blocks, and record direct control-flow edges. It should report indirect
or unresolved exits explicitly rather than attempting to infer them.

Initial roots come from command-file code/procedure entries and, where
practical, vector-table entries.

## Output Interface

Public CFG output:

```text
-g cmd|dot|json
-G filename
```

If `-G` is omitted, the output filename is derived from the command-file path:

```text
firmware.dz80 + -g dot  -> firmware.dot
firmware.dz80 + -g json -> firmware.json
firmware.dz80 + -g cmd  -> firmware.cfg.dz80
firmware       + -g cmd  -> firmware.cfg
```

Avoid ambiguous executable-looking extensions such as `.cmd`, `.bat`, or `.sh`.

Diagnostic CFG output:

```text
-C
```

`-C` is an intentionally supported, stable, line-oriented raw CFG dump. It is
not initially documented as part of the user-facing manual, but it is a
deliverable feature for tests, debugging, and future output-format development.

## Output Formats

`cmd`
: Generate a dasmxx command-file fragment containing discovered `c` entries
  and `p` entries for known call targets.

`dot`
: Generate a Graphviz DOT CFG for quick visual inspection.

`json`
: Generate structured CFG data for downstream tools.

`-C`
: Print the internal CFG model in a simple, stable debug format.

All public emitters should be derived from the same internal CFG model that
`-C` exposes.

## Staged Implementation

### 1. CFG Metadata Plumbing

- Add internal flow metadata structures and APIs.
- Extend the decode path so instruction metadata can be captured alongside
  the existing formatted disassembly.
- Preserve existing listing behavior.

Tests:

- Build all targets.
- Existing tests should remain unchanged.

### 2. CFG Tracer With Raw Debug Output

- Add the generic worklist tracer.
- Seed tracing from `c` and `p` command-file entries.
- Decode reachable instructions by address.
- Split basic blocks at branch targets and fallthrough targets.
- Record edges for fallthrough, branch, call, return, stop, and indirect exits.
- Add `-C` raw CFG output.

Tests:

- Add small golden tests against `-C` output.

### 3. Public CFG CLI and Emitters

- Add `-g cmd|dot|json`.
- Add `-G filename`.
- Add default CFG filename derivation.
- Implement `cmd`, `dot`, and `json` emitters from the shared CFG model.

Tests:

- Cover default filename derivation.
- Add golden tests for each output format.

### 4. First Decoder Semantics

- Teach Z80 first.
- Add 6502/65C02 next if still in scope for the first tranche.
- Classify direct jumps, conditional jumps, calls, returns, halts/stops, and
  indirect exits.

Tests:

- Add target-specific CFG tests for direct branches, conditional branches,
  calls, returns, and unresolved indirect exits.

## Design Notes

Existing xrefs are useful for labels and references, but are not sufficient for
CFG construction. `X_JMP` currently does not distinguish conditional from
unconditional control flow, and returns/stops are often represented as
`X_NONE`. The CFG work therefore needs explicit instruction-flow metadata rather
than relying only on xref records.

## Future Development

Further CFG work should be driven by real firmware examples rather than added
speculatively. Likely areas are:

- Resolving common jump-table and dispatch-table patterns.
- Modeling enough register or flag state to follow simple computed branches.
- Adding richer vector-table conventions for targets that use non-trivial
  interrupt, reset, trap, or banked-vector layouts.
- Improving output ergonomics once `cmd`, `dot`, and `json` have been used on
  larger programs.

These are deliberately outside the initial CFG/basic-block reconstruction
scope. The current tracer reports unresolved indirect exits explicitly so those
cases remain visible for later analysis.
