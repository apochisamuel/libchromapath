# Architecture

libchromapath separates binary decoding, typed profile state, color-science operations, transform execution, and presentation APIs into independent modules.

## Profile processing stages

1. **Input ownership** — `profile.c` stores a private copy of the profile bytes and initializes parse limits.
2. **Primitive reading** — `reader.c` provides bounded big-endian integer, fixed-point, byte-slice, and cursor operations.
3. **Header decoding** — `header.c` reads the fixed ICC header and establishes profile class, device color space, PCS, intent, platform, and size metadata.
4. **Directory construction** — `directory.c` reads tag records, verifies directory extent, and records tag signatures, offsets, lengths, and payload types.
5. **Typed payload decoding** — `text.c`, `xyz.c`, `curve.c`, `matrix.c`, and `tags.c` decode commonly used metadata, tristimulus, response-curve, and colorant data.
6. **Plan preparation** — `intent.c` summarizes channel response data for later transform selection and profile-wide analysis.
7. **Consistency checks** — `validate.c`, `canonical.c`, and `diagnostics.c` evaluate profile-wide relationships, required RGB components, payload alignment, type expectations, directory geometry, and equivalent channel representations.
8. **Transform construction** — `transform.c` builds an independent matrix/TRC transform from parsed profile state.
9. **Evaluation** — transform, pipeline, CLUT, interpolation, tone-curve, and gamut modules evaluate colors and derived representations.
10. **Queries and presentation** — profile query APIs, comparison, text reporting, JSON, CSV, signature registries, and the command-line inspector expose results.

## Supporting color-science modules

- `color_convert.c` implements conversions among XYZ, xyY, Lab, LCh, sRGB, HSL, HSV, and CMYK.
- `matrix_ops.c` implements matrix multiplication, inversion, determinant calculation, and chromatic adaptation.
- `delta_e.c` implements standard perceptual color-difference formulas.
- `spectral.c` provides sampled CIE observer data, interpolation, source models, and spectral-to-XYZ integration.
- `illuminants.c` and `rgb_working_spaces.c` provide standard reference data and matrix construction.
- `reference_grid.c` provides a dense, deterministic sRGB/Lab reference grid for quantization and nearest-color analysis.
- `named_colors.c` provides named-color lookup and perceptual nearest-neighbor selection.
- `clut.c` and `pipeline.c` provide reusable multidimensional and staged transform components.

## Ownership model

A profile owns its copied input bytes and all decoded profile structures. A transform copies the curve and matrix state required for evaluation, so it can be destroyed independently from the source profile. Public registry entries and reference datasets are immutable static data. Heap-backed pipeline, CLUT, and tone-curve objects have explicit create and destroy APIs.

## Error model

Public functions return `cp_status` values. Parsing distinguishes malformed input, unsupported features, truncation, memory exhaustion, I/O errors, state errors, and output-capacity failures. The profile object stores a short diagnostic string for parse failures.
