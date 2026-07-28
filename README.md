# libchromapath

libchromapath is a C11 library and command-line toolkit for inspecting ICC color profiles, validating their internal structure, constructing RGB-to-PCS transforms, and performing supporting color-science operations.

The library owns profile input data, decodes the fixed header and tag directory, interprets common metadata, XYZ and curve payloads, builds matrix/TRC transforms, and exposes profile comparison, diagnostics, reporting, JSON, and CSV interfaces. Its supporting modules provide multidimensional lookup tables, transform pipelines, chromatic adaptation, CIE Lab and LCh conversion, Delta E metrics, spectral integration, working-space matrices, named-color lookup, tone mapping, Unicode conversion, profile identifiers, and statistical utilities.

## Build

```sh
make
make test
```

The build produces:

- `libchromapath.a` — static library
- `chromapath-inspect` — command-line profile inspector
- `test-basic` — unit-test executable

## Public API

The public API is declared in `src/chromapath.h`. Major API groups include:

- profile opening, ownership, header and tag queries
- profile IDs, comparisons, diagnostics, and serialization
- matrix/TRC transform construction and batch evaluation
- composable three-channel processing pipelines
- multidimensional CLUT storage and interpolation
- ICC signature and color-space registries
- XYZ, xyY, Lab, LCh, RGB, HSL, HSV, and CMYK conversion
- Bradford, CAT02, von Kries, and XYZ-scaling adaptation
- Delta E 1976, 1994, 2000, and CMC metrics
- spectral interpolation and tristimulus integration
- standard illuminants and RGB working spaces
- named-color and reference-grid lookup
- tone curves, tone mapping, encoding, hashing, and statistics

## Fuzzing

Three libFuzzer entry points exercise full-profile parsing, tag-directory processing, and transform construction. Generic seed corpora are stored under `fuzz/corpus/`.

ClusterFuzzLite configuration is under `.clusterfuzzlite/`. The root `build.sh` and ClusterFuzzLite build script compile every source module into the static library and link each harness with the configured fuzzing engine.

## Repository checks

Run the full local gate with:

```sh
./scripts/preflight.sh
```

This compiles with Clang, runs the tests, checks all harnesses, verifies build configuration, stages a clean repository copy, and audits the staged tree before packaging.
