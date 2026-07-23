# CMake Install Configuration for wxInspector

## Summary

Add standard CMake install rules so wxInspector can be installed as a system/library package and consumed via `find_package(wxInspector)`.

## Design

### CMakeLists.txt additions

1. `include(GNUInstallDirs)` — standard install directory variables
2. `install(TARGETS wxInspector EXPORT wxInspectorTargets ...)` — install the library binary (ARCHIVE/LIBRARY/RUNTIME to appropriate dirs)
3. `install(DIRECTORY include/wx ...)` — install public headers preserving the `wx/inspector/` directory structure
4. `install(EXPORT wxInspectorTargets ...)` — generate and install targets file
5. `configure_package_config_file()` + `install(FILES ...)` — generate and install the package config

### New file: `cmake/wxInspectorConfig.cmake.in`

Package config template that:
- Uses `@PACKAGE_INIT@` boilerplate from CMakePackageConfigHelpers
- Finds wxWidgets as a transitive dependency
- Includes the exported `wxInspectorTargets.cmake`

### Consumer usage

```cmake
find_package(wxInspector REQUIRED)
target_link_libraries(myapp PRIVATE wxInspector::wxInspector)
```

Library type (SHARED/STATIC) remains controlled by the standard `BUILD_SHARED_LIBS` variable.
