cmake_minimum_required(VERSION 3.16)
project(wxInspectorSample)

find_package(wxWidgets ${WXWIDGETS_CONFIG_MODE} REQUIRED COMPONENTS core base aui propgrid)
if(wxWidgets_USE_FILE)
    include(${wxWidgets_USE_FILE})
endif()

add_executable(minimal minimal.cpp)
target_link_libraries(minimal PRIVATE wxInspector ${wxWidgets_LIBRARIES})
target_include_directories(minimal PRIVATE
    ../../include
    ${wxWidgets_INCLUDE_DIRS}
)
