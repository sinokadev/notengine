include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install target

install(
    TARGETS notengine
    EXPORT notengineTargets

    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public headers

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

## Generated shader header
install(
    FILES "${GENERATED_HEADER}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

## Generated version header
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/knot/version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/knot"
)

# Export targets

set(
    NOTENGINE_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/notengine"
)

install(
    EXPORT notengineTargets
    FILE notengineTargets.cmake
    NAMESPACE notengine::
    DESTINATION "${NOTENGINE_INSTALL_CMAKEDIR}"
)

# Package configuration

set(
    NOTENGINE_CONFIG_TEMPLATE
    "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake.in"
)

file(
    WRITE
    "${NOTENGINE_CONFIG_TEMPLATE}"
[=[
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(glfw3 CONFIG REQUIRED)
find_dependency(glm CONFIG REQUIRED)
find_dependency(OpenGL REQUIRED)
# find_dependency(OpenAL REQUIRED)

if(UNIX AND NOT APPLE)
    find_dependency(Threads REQUIRED)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/notengineTargets.cmake")

target_link_libraries(notengine::notengine INTERFACE
    glfw
    glm::glm
    # OpenAL::OpenAL
)

check_required_components(notengine)
]=]
)

configure_package_config_file(
    "${NOTENGINE_CONFIG_TEMPLATE}"
    "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake"

    INSTALL_DESTINATION
        "${NOTENGINE_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/notengineConfigVersion.cmake"

    VERSION
        "${GAME_VERSION_STRING}"

    COMPATIBILITY
        SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/notengineConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/notengineConfigVersion.cmake"

    DESTINATION
        "${NOTENGINE_INSTALL_CMAKEDIR}"
)