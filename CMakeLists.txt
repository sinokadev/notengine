cmake_minimum_required(VERSION 3.15)
project(NotEngine LANGUAGES C CXX)

# C++ Standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(GAME_VERSION_MAJOR 0)
set(GAME_VERSION_MINOR 1)
set(GAME_VERSION_PATCH 10)

set(GAME_VERSION_STRING
    "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/knot/version.h"
    "${CMAKE_CURRENT_BINARY_DIR}/knot/version.h"
    @ONLY
)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Create the Engine library
file(GLOB_RECURSE SOURCES "src/*.cpp")

list(APPEND SOURCES "library/gl.c")

add_library(notengine STATIC ${SOURCES})

# 셰이더 자동 포함 로직
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaders.h")
file(GLOB SHADER_FILES "${SHADER_DIR}/*.glsl" "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

function(embed_shaders_to_header input_files output_header)
    set(header_content "#pragma once\n#include <string>\n#include <unordered_map>\n\nnamespace Shaders {\n")
    
    foreach(file ${input_files})
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_ext ${file} EXT)
        set(var_name "${file_name}${file_ext}")
        string(REPLACE "." "_" var_name "${var_name}")
        
        file(READ ${file} shader_source ENCODING UTF-8)
        string(REPLACE "\\" "\\\\" shader_source "${shader_source}")
        string(REPLACE "\"" "\\\"" shader_source "${shader_source}")
        string(REPLACE "\n" "\\n\"\n\"" shader_source "${shader_source}")
        
        set(header_content "${header_content}  inline const char* ${var_name} = \n\"${shader_source}\";\n\n")
    endforeach()

    set(header_content "${header_content}  inline std::unordered_map<std::string, const char*> Registry = {\n")
    foreach(file ${input_files})
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_ext ${file} EXT)
        set(var_name "${file_name}${file_ext}")
        string(REPLACE "." "_" var_name "${var_name}")
        
        set(header_content "${header_content}    {\"${file_name}${file_ext}\", ${var_name}},\n")
    endforeach()
    set(header_content "${header_content}  };\n")
    set(header_content "${header_content}}\n")
    
    file(WRITE ${output_header} "${header_content}")
endfunction()

embed_shaders_to_header("${SHADER_FILES}" "${GENERATED_HEADER}")

# Set include directories
target_include_directories(notengine PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link dependencies to the library
target_link_libraries(notengine PUBLIC
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

target_compile_definitions(notengine PUBLIC
    NOTENGINE_ASSET_ROOT="${CMAKE_SOURCE_DIR}"
)

# Demo executable (EXCLUDE_FROM_ALL 추가)
add_executable(demo EXCLUDE_FROM_ALL demo/demo.cpp)
target_link_libraries(demo PRIVATE notengine)

set_target_properties(demo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(benchmark EXCLUDE_FROM_ALL demo/benchmark.cpp)
target_link_libraries(benchmark PRIVATE notengine)

set_target_properties(benchmark PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(scene_render EXCLUDE_FROM_ALL demo/scene_render.cpp)
target_link_libraries(scene_render PRIVATE notengine)

set_target_properties(scene_render PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

include(GNUInstallDirs)

install(TARGETS notengine
    EXPORT notengineTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${GENERATED_HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT notengineTargets
    FILE notengineTargets.cmake
    NAMESPACE notengine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notengine
)

include(CMakePackageConfigHelpers)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake" "
include(CMakeFindDependencyMacro)
find_dependency(glfw3 REQUIRED)
find_dependency(OpenGL REQUIRED)
include(\"\${CMAKE_CURRENT_LIST_DIR}/notengineTargets.cmake\")
")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notengine
)