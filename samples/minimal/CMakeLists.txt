cmake_minimum_required(VERSION 3.16)
project(wxInspectorSample)

find_package(wxWidgets 3.2.0 REQUIRED COMPONENTS core base aui propgrid)
include(${wxWidgets_USE_FILE})

add_executable(minimal minimal.cpp)
target_link_libraries(minimal PRIVATE wxInspector ${wxWidgets_LIBRARIES})
target_include_directories(minimal PRIVATE
    ../../include
    ${wxWidgets_INCLUDE_DIRS}
)
