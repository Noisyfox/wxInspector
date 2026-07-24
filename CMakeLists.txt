cmake_minimum_required(VERSION 3.16)
project(wxInspector VERSION 0.1.0 LANGUAGES CXX)

if(WIN32)
    if(MSVC)
        # Force the source code encoding to UTF-8.
        add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif(MSVC)
endif(WIN32)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets 3.2.0 REQUIRED COMPONENTS core base aui propgrid)
include(${wxWidgets_USE_FILE})

# Library sources
set(WXINSPECTOR_SOURCES
    src/inspector.cpp
    src/frame.cpp
    src/tree.cpp
    src/info.cpp
    src/invoker.cpp
    src/events.cpp
    src/highlighter.cpp
    src/object.cpp
    src/property_provider.cpp
    src/method_registry.cpp
)

set(WXINSPECTOR_HEADERS
    include/wx/inspector/inspector.h
    include/wx/inspector/frame.h
    include/wx/inspector/tree.h
    include/wx/inspector/info.h
    include/wx/inspector/invoker.h
    include/wx/inspector/events.h
    include/wx/inspector/highlighter.h
    include/wx/inspector/object.h
    include/wx/inspector/property_provider.h
    include/wx/inspector/method_registry.h
    include/wx/inspector/plugin.h
)

# Build the library
add_library(wxInspector ${WXINSPECTOR_SOURCES} ${WXINSPECTOR_HEADERS})
target_include_directories(wxInspector
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        $<BUILD_INTERFACE:${wxWidgets_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(wxInspector PRIVATE ${wxWidgets_LIBRARIES})

# Samples
option(wxBUILD_SAMPLES "Build sample applications" OFF)
if(wxBUILD_SAMPLES)
    add_subdirectory(samples/minimal)
endif()

# ---- Install rules ----
include(GNUInstallDirs)

# Install the library target
install(TARGETS wxInspector
    EXPORT wxInspectorTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public headers (preserving the wx/inspector/ directory structure)
install(DIRECTORY include/wx
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Generate and install CMake package config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/wxInspectorConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/wxInspectorConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wxInspector
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wxInspectorConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wxInspector
)
install(EXPORT wxInspectorTargets
    NAMESPACE wxInspector::
    FILE wxInspectorTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wxInspector
)
