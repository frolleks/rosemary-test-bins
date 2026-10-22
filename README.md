# rosemary-test-bins

Reference binaries for CoreGraphics behavior, used to check an emulator (or any
reimplementation) against Apple's implementation line for line.

Two small C programs ("probes") exercise a slice of CoreGraphics and print every
answer in a deterministic, diffable format:

| Probe | Source | Covers |
| --- | --- | --- |
| `cgtransform_probe` | `cgtransform-probe.c` | `CGAffineTransform`: make, scale, translate, concat, invert, identity, and applying transforms to rectangles and points |
| `cgpath_probe` | `cgpath-probe.c` | `CGPath` / `CGMutablePath`: building paths, arcs, ellipses, rounded rectangles, copies, dashing, stroking, flattening, normalizing, boolean operations, introspection, `CGPathApply`, and `CGPathApplyWithBlock` |

The same source is used on both sides of the comparison: compile it on macOS
against the real framework to produce the expected output, then run it against
the implementation under test. The two outputs must match exactly.

## Design: freestanding probes

Both probes are deliberately freestanding with respect to the macOS SDK: they do
not `#include <CoreGraphics/...>`. Instead they spell out the CoreGraphics
types, enums, constants, and function declarations they use. This lets the
probes compile in environments that do not have the SDK headers.

The declarations are only declarations. The programs are still linked against
the real `CoreGraphics` framework:

```sh
cc -arch x86_64 -fblocks -framework CoreGraphics -o cgpath_probe cgpath-probe.c
```

so the reference answers come from Apple's implementation, not from a stub.

`-fblocks` is required because `cgpath-probe.c` exercises
`CGPathApplyWithBlock`; the block declarations are guarded by `__BLOCKS__`.

## Requirements

- macOS with the Xcode command line tools, to build and link against
  `CoreGraphics`.
- `rosemary` (optional), to run the emulated binary.

## Building

```sh
# x86_64 guest binaries
cc -arch x86_64 -fblocks -framework CoreGraphics -o cgtransform_probe cgtransform-probe.c
cc -arch x86_64 -fblocks -framework CoreGraphics -o cgpath_probe cgpath-probe.c

# native arm64 slices
cc -arch arm64 -fblocks -framework CoreGraphics -o cgtransform_probe_arm64 cgtransform-probe.c
cc -arch arm64 -fblocks -framework CoreGraphics -o cgpath_probe_arm64 cgpath-probe.c

# universal binaries
lipo -create -output cgtransform_probe_universal cgtransform_probe cgtransform_probe_arm64
lipo -create -output cgpath_probe_universal cgpath_probe cgpath_probe_arm64
```

## Producing expected output

Run the native binary and capture stdout:

```sh
./cgtransform_probe > expected-cgtransform.txt
./cgpath_probe > expected-cgpath.txt
```

Then run the implementation under test and compare:

```sh
rosemary run --emulated ./cgtransform_probe > actual-cgtransform.txt
diff -u expected-cgtransform.txt actual-cgtransform.txt
```

## Output format

The output is designed to diff cleanly:

- Each scenario starts with `case <name>`.
- Values are indented two spaces and labelled.
- Floating-point values are printed with `%.6f`.
- `-0.0` is normalized to `0.0` (`value + 0.0`) so a sign difference is not
  reported as a behavioral difference.
- Paths are printed element by element (`move`, `line`, `quad`, `curve`,
  `close`) followed by introspection results.

Do not change labels, ordering, or formatting without regenerating the expected
files.

## CI and releases

`.github/workflows/build-release.yml` runs on `macos-latest`:

1. Compiles x86_64, arm64, and universal binaries for both probes.
2. Verifies architectures with `file` and `lipo -info`.
3. Runs the native binaries to generate `expected-cgtransform.txt` and
   `expected-cgpath.txt`.
4. Packages the binaries, sources, and expected outputs into a tarball with a
   SHA-256 checksum.
5. Publishes a GitHub release.

Every push to any branch creates a release tagged `build-<run>-<sha>`; pushing a
`v*` tag creates a release with that tag. A manual `workflow_dispatch` run also
publishes a release.

## Adding or changing a probe

- Use only documented, exported CoreGraphics API. Check Apple's
  [CGPath](https://developer.apple.com/documentation/coregraphics/cgpath) and
  related documentation; the linker is the final arbiter. For example,
  `CGPathCreateCopyByLinearlyInterpolatingPaths` is not a public CoreGraphics
  symbol and must not be used.
- Keep the hand-written declarations ABI-compatible with the SDK: same parameter
  types, `const` qualification, `bool` versus `int`, `size_t`, and return types.
- Keep the probe deterministic: no dates, addresses, hashes, or uninitialized
  memory in the output.
- Regenerate the expected output after any behavioral or formatting change.

## License

Public domain (Unlicense). See `LICENSE`.
