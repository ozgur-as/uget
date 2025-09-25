# Building on Windows (MSVC + vcpkg, x64)

## Prereqs
- Visual Studio 2022 (Desktop development with C++)
- vcpkg (https://github.com/microsoft/vcpkg) and set `VCPKG_ROOT`

## One-time setup
```powershell
# In an x64 Native Tools for VS 2022 shell:
git clone https://github.com/microsoft/vcpkg %USERPROFILE%\vcpkg
setx VCPKG_ROOT %USERPROFILE%\vcpkg
```

## Configure, build, test
```powershell
cmake --preset msvc-vcpkg
cmake --build build\msvc --config Debug
ctest --test-dir build\msvc -C Debug --output-on-failure
```

## Notes
- Dependencies come from vcpkg.json (gtk3, glib, gstreamer, curl, openssl).
- Tray support (Ayatana/AppIndicator) may be unavailable on Windows: the build will continue without it.
- Use RelWithDebInfo for release-like builds: `cmake -S . -B build\msvc -DCMAKE_BUILD_TYPE=RelWithDebInfo --preset msvc-vcpkg`
