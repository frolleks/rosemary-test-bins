# AGENTS.md

Guidance for automated agents working in this repository.

## What this repository is

`rosemary-test-bins` contains two C "probes" that print CoreGraphics behavior in
a deterministic, diffable format. They are reference programs: build them on
macOS against the real `CoreGraphics` framework to get expected output, then run
the same source under the implementation being tested and compare stdout.

- `cgtransform-probe.c` — `CGAffineTransform` behavior.
- `cgpath-probe.c` — `CGPath` / `CGMutablePath` behavior.
- `.github/workflows/build-release.yml` — builds, smoke-tests, packages, and
  releases both probes.

## Hard rules

1. **No CoreGraphics includes.** Both probes are freestanding. Do not add
   `#include <CoreGraphics/...>`. Add hand-written declarations instead, in the
   style already used in the files.
2. **Declarations must match the SDK ABI.** Match parameter types, `const`
   qualification, `bool` versus `int`, `size_t`, and return types exactly. A
   mismatch can compile and still be wrong at the call boundary.
3. **Only public, exported symbols.** Verify against Apple's documentation
   before adding a call. The macOS linker is the final check.
   `CGPathCreateCopyByLinearlyInterpolatingPaths` is not public and was removed
   for this reason.
4. **Keep linking against the framework.** The build must keep
   `-framework CoreGraphics`; the probes contain declarations only, and the
   symbols come from Apple's framework.
5. **Keep `-fblocks`.** `cgpath-probe.c` uses `CGPathApplyWithBlock`; the block
   declarations are guarded by `__BLOCKS__`.
6. **Output is a contract.** Case names, labels, ordering, indentation, and
   `%.6f` formatting are compared line for line. Do not change them casually; if
   you do, regenerate the expected files.
7. **Determinism.** No addresses, timestamps, iteration-order-dependent output,
   or uninitialized values.

## Build and verify

A full build requires macOS with the CoreGraphics framework:

```sh
cc -arch x86_64 -fblocks -framework CoreGraphics -o cgtransform_probe cgtransform-probe.c
cc -arch x86_64 -fblocks -framework CoreGraphics -o cgpath_probe cgpath-probe.c
```

On Linux (no CoreGraphics), a syntax check is still useful:

```sh
clang -std=c11 -fblocks -Wall -Wextra -fsyntax-only cgtransform-probe.c
clang -std=c11 -fblocks -Wall -Wextra -fsyntax-only cgpath-probe.c
```

`-fsyntax-only` does not validate symbol availability; only the macOS link does.

Generate expected output:

```sh
./cgtransform_probe > expected-cgtransform.txt
./cgpath_probe > expected-cgpath.txt
```

## CI behavior

The workflow runs on every push and publishes a release. Pushing to a branch
creates `build-<run>-<sha>`; pushing a `v*` tag creates that tag. Be aware that a
push triggers a release.

## Style

- C11, two-space indentation, K&R braces.
- Comments explain intent and non-obvious behavior, not the code.
- Keep the probes self-contained. Duplication of type declarations between the
  two files is intentional.

## Note

The probe header comments mention `build-guest.sh`, which is not currently part
of this repository. The authoritative build recipe is
`.github/workflows/build-release.yml`.
