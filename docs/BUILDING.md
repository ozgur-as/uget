# Building uGet with CMake

uGet now uses a single CMake-based build across platforms. You will need **CMake
3.24 or newer** plus a C and C++ toolchain that can compile GTK-based
applications.

## Linux and other Unix-like platforms

```sh
cmake --preset dev
cmake --build build/dev
ctest --test-dir build/dev --output-on-failure
```

Install the GTK 3, GLib, GStreamer and libcurl development packages from your
distribution. The configure step will fail if these packages are missing while
`WITH_GUI` is enabled (the default). For headless or minimal builds you can pass
`-DWITH_GUI=OFF` to skip those requirements.

Other useful presets:

- `cmake --preset asan` – Debug build with AddressSanitizer and
  UndefinedBehaviorSanitizer enabled (requires clang or gcc with sanitizer
  support).
- `cmake --preset rel` – RelWithDebInfo configuration suitable for
  release/packaging validation.

The presets drive dependency discovery via `pkg-config`. Optional components
(such as Ayatana/AppIndicator tray icons, OpenSSL, libpwmd and GnuTLS) are
picked up automatically when their development packages are available. If a
library is missing the feature falls back gracefully or is disabled without
failing the configuration. You can also toggle features explicitly with
`-DWITH_TRAY=OFF`, `-DWITH_OPENSSL=OFF`, `-DWITH_PWMD=OFF`, or
`-DWITH_GNUTLS=ON` when running `cmake`.

## Windows (MSVC + vcpkg)

Install Visual Studio 2022 with the "Desktop development with C++" workload and
set up [vcpkg](https://github.com/microsoft/vcpkg). With `VCPKG_ROOT` pointing
at your vcpkg checkout, run the Windows preset:

```powershell
cmake --preset msvc-vcpkg
cmake --build build\msvc --config Debug
ctest --test-dir build\msvc -C Debug --output-on-failure
```

The vcpkg manifest (vcpkg.json) pulls in gtk3, glib, gstreamer, curl and
openssl. Tray integration will automatically use Ayatana/AppIndicator when those
libraries are available in vcpkg; otherwise the build continues without tray
support. Additional options described above work on Windows as well.

For release-like builds use the RelWithDebInfo configuration:

```powershell
cmake --build build\msvc --config RelWithDebInfo
```
