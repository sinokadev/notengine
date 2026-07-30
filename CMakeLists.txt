cmake_minimum_required(VERSION 3.15)
project(NotEngine LANGUAGES C CXX)

# C++ Standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Create the Engine library
file(GLOB_RECURSE SOURCES "src/*.cpp")

list(APPEND SOURCES "library/gl.c")

add_library(notengine STATIC ${SOURCES})

# [추가] 셰이더 자동 포함 로직
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders") # 셰이더 파일이 있는 경로
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaders.h")
file(GLOB SHADER_FILES "${SHADER_DIR}/*.glsl" "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

# 셰이더 헤더 생성 함수 정의
function(embed_shaders_to_header input_files output_header)
    set(header_content "#pragma once\n#include <string>\n#include <unordered_map>\n\nnamespace Shaders {\n")
    
    # 1. 각각의 셰이더 코드를 const char* 상수로 정의
    foreach(file ${input_files})
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_ext ${file} EXT)
        set(var_name "${file_name}${file_ext}")
        string(REPLACE "." "_" var_name "${var_name}")
        
        file(READ ${file} shader_source)
        string(REPLACE "\\" "\\\\" shader_source "${shader_source}")
        string(REPLACE "\"" "\\\"" shader_source "${shader_source}")
        string(REPLACE "\n" "\\n\"\n\"" shader_source "${shader_source}")
        
        set(header_content "${header_content}  inline const char* ${var_name} = \n\"${shader_source}\";\n\n")
    endforeach()

    # 2. 파일명으로 소스 코드를 찾을 수 있는 맵 생성
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

# 함수 실행 (빌드 시 셰이더 변경 감지)
embed_shaders_to_header("${SHADER_FILES}" "${GENERATED_HEADER}")

# Set include directories
target_include_directories(notengine PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Link dependencies to the library
# OpenGL::GL provides OpenGL library interface
# CMAKE_DL_LIBS is required on Linux for GLAD (dlopen/dlsym)
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