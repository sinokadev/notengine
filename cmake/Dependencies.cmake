include_guard(GLOBAL)

include(FetchContent)

# Dependency Options

option(
    NOTENGINE_FETCH_DEPENDENCIES
    "Fetch missing dependencies automatically"
    ON
)

### GLFW ###

find_package(glfw3 CONFIG QUIET)

if(NOT TARGET glfw)
    if(NOT NOTENGINE_FETCH_DEPENDENCIES)
        message(FATAL_ERROR
            "GLFW was not found and NOTENGINE_FETCH_DEPENDENCIES is OFF."
        )
    endif()

    message(STATUS "GLFW not found. Fetching GLFW...")

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(glfw)
endif()

### GLM ###

find_package(glm CONFIG QUIET)

if(NOT TARGET glm::glm)
    if(NOT NOTENGINE_FETCH_DEPENDENCIES)
        message(FATAL_ERROR
            "GLM was not found and NOTENGINE_FETCH_DEPENDENCIES is OFF."
        )
    endif()

    message(STATUS "GLM not found. Fetching GLM...")

    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.3
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(glm)
endif()

### OpenGL ###

find_package(OpenGL REQUIRED)

### OpenAL Soft ###
# Uncomment the following code to enable it.

# find_package(OpenAL QUIET)

# if(NOT TARGET OpenAL::OpenAL)
#     if(NOT NOTENGINE_FETCH_DEPENDENCIES)
#         message(FATAL_ERROR
#             "OpenAL was not found and NOTENGINE_FETCH_DEPENDENCIES is OFF."
#         )
#     endif()

#     message(STATUS "OpenAL not found. Fetching OpenAL Soft...")

#     set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)

#     set(LIBTYPE STATIC CACHE STRING "" FORCE)

#     set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
#     set(ALSOFT_INSTALL_UTILS OFF CACHE BOOL "" FORCE)

#     FetchContent_Declare(
#         openal
#         GIT_REPOSITORY https://github.com/kcat/openal-soft.git
#         GIT_TAG        1.25.2
#         GIT_SHALLOW    TRUE
#     )

#     FetchContent_MakeAvailable(openal)
# endif()

### Platform dependencies ###

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
endif()