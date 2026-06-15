cmake_minimum_required(VERSION 3.15)
project(NotEngine LANGUAGES C CXX)

# C++ Standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Create the Engine library
file(GLOB_RECURSE SOURCES "src/*.cpp")

list(APPEND SOURCES "library/gl.c")

add_library(notengine STATIC ${SOURCES})

# Set include directories
target_include_directories(notengine PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link dependencies to the library
# OpenGL::GL provides OpenGL library interface
# CMAKE_DL_LIBS is required on Linux for GLAD (dlopen/dlsym)
target_link_libraries(notengine PUBLIC
    glfw
    glm::glm
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

target_compile_definitions(notengine PUBLIC
    NOTENGINE_ASSET_ROOT="${CMAKE_SOURCE_DIR}"
)

# Demo executable
add_executable(demo demo/demo.cpp)
target_link_libraries(demo PRIVATE notengine)

set_target_properties(demo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_executable(toolkit_demo demo/toolkit_demo.cpp)
target_link_libraries(toolkit_demo PRIVATE notengine)

set_target_properties(toolkit_demo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
