# Build Presets

Use CMake presets to configure common build types:

- `cmake --preset dev` for a standard Debug build.
- `cmake --preset asan` for a Debug build with AddressSanitizer/UndefinedBehaviorSanitizer enabled (Unix-like platforms).
- `cmake --preset rel` for a RelWithDebInfo build.

When debugging sanitizer builds you can enable stricter behavior with:

```sh
export ASAN_OPTIONS=halt_on_error=1
```

Set the variable before running the instrumented binary to stop immediately on the first sanitizer report.
