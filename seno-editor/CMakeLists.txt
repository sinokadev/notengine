cmake_minimum_required(VERSION 3.15)
project(SenoEditor LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(KNOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
option(SENO_BUILD_GUI "Build the ImGui editor" ON)
include(CTest)
if(BUILD_TESTING)
    add_executable(seno_document_tests tests/document_tests.cpp)
    target_include_directories(seno_document_tests PRIVATE "${KNOT_ROOT}/include")
    add_test(NAME seno_document COMMAND seno_document_tests)
endif()
if(SENO_BUILD_GUI)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    if(NOT TARGET notengine)
        add_subdirectory("${KNOT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/knot" EXCLUDE_FROM_ALL)
    endif()
    include(FetchContent)
    # Pass -DFETCHCONTENT_SOURCE_DIR_IMGUI=/path/to/imgui for offline builds.
    FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06) # v1.91.9b
    FetchContent_MakeAvailable(imgui)
    add_library(seno_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(seno_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imgui_SOURCE_DIR}/misc/cpp)
    target_link_libraries(seno_imgui PUBLIC glfw OpenGL::GL ${CMAKE_DL_LIBS})
    add_executable(seno-editor main.cpp)
    target_link_libraries(seno-editor PRIVATE seno_imgui notengine)
    target_compile_definitions(seno-editor PRIVATE SENO_ASSET_ROOT="${KNOT_ROOT}")
endif()
