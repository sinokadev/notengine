cmake_minimum_required(VERSION 3.15)
project(NotEngine LANGUAGES C CXX)

include(GNUInstallDirs)

# C++ Standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Version configuration
set(GAME_VERSION_MAJOR 0)
set(GAME_VERSION_MINOR 1)
set(GAME_VERSION_PATCH 10)
set(GAME_VERSION_STRING "${GAME_VERSION_MAJOR}.${GAME_VERSION_MINOR}.${GAME_VERSION_PATCH}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/knot/version.h"
    "${CMAKE_CURRENT_BINARY_DIR}/knot/version.h"
    @ONLY
)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Sources (CONFIGURE_DEPENDS 추가하여 파일 추가 감지)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(APPEND SOURCES "library/gl.c" "library/miniaudio.c")

add_library(notengine STATIC ${SOURCES})

# Shader embedding setup
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaders.h")
file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADER_DIR}/*.glsl" "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

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

# Include directories
target_include_directories(notengine PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# miniaudio.c includes its companion header as "miniaudio.h".  Keep this
# implementation-only include path private; users include <miniaudio/miniaudio.h>.
target_include_directories(notengine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/miniaudio
)

# Link dependencies
target_link_libraries(notengine PUBLIC
    glfw
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(notengine PUBLIC Threads::Threads m)
elseif(APPLE)
    target_link_libraries(notengine PUBLIC
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    target_link_libraries(notengine PUBLIC ole32 avrt winmm)
endif()

target_compile_definitions(notengine PUBLIC
    NOTENGINE_ASSET_ROOT="${CMAKE_SOURCE_DIR}"
)

# Demo executables
add_executable(demo EXCLUDE_FROM_ALL demo/demo.cpp)
target_link_libraries(demo PRIVATE notengine)
set_target_properties(demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(benchmark EXCLUDE_FROM_ALL demo/benchmark.cpp)
target_link_libraries(benchmark PRIVATE notengine)
set_target_properties(benchmark PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(scene_render EXCLUDE_FROM_ALL demo/scene_render.cpp)
target_link_libraries(scene_render PRIVATE notengine)
set_target_properties(scene_render PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

install(TARGETS notengine
    EXPORT notengineTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${GENERATED_HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/knot/version.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/knot)

install(EXPORT notengineTargets
    FILE notengineTargets.cmake
    NAMESPACE notengine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notengine
)

# Config generation
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake" "
include(CMakeFindDependencyMacro)
find_dependency(glfw3 REQUIRED)
find_dependency(OpenGL REQUIRED)
if(UNIX AND NOT APPLE)
find_dependency(Threads REQUIRED)
endif()
include(\"\${CMAKE_CURRENT_LIST_DIR}/notengineTargets.cmake\")
")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notengine
)
