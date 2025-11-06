cmake_minimum_required(VERSION 3.28...3.30)

project(dx2 LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only include global build-configuration features if we are the root CMakeLists.txt
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
    include(SetDefaultBuildRelWithDebInfo)
    include(AlwaysColourCompilation)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# #######################################################################
# External Dependencies
find_package(HDF5 REQUIRED)
find_package(gemmi CONFIG REQUIRED)

# #######################################################################
# Automatic Dependencies
set(FETCHCONTENT_QUIET OFF)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG b514bdc898e2951020cbdca1304b75f5950d1f59 # v1.15.2
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(
    Eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(
    mdspan
    GIT_REPOSITORY https://github.com/kokkos/mdspan
    GIT_TAG mdspan-0.6.0
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Eigen3 fmt nlohmann_json mdspan)

# GTest could have been made available under a different name
if(NOT TARGET GTest::gtest_main)
    FetchContent_MakeAvailable(GTest)
endif()

# #######################################################################
# Targets
enable_testing()

add_subdirectory(dx2)
add_subdirectory(tests)

# #######################################################################
# Installation

# Install FetchContent dependencies if they were fetched (not found via find_package)
if(NOT nlohmann_json_FOUND)
    install(TARGETS nlohmann_json EXPORT DX2Targets)
endif()
if(NOT fmt_FOUND)
    install(TARGETS fmt EXPORT DX2Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()
# Eigen3 is header-only, doesn't need runtime installation
# mdspan is header-only, doesn't need runtime installation
# GTest is test-only, doesn't need installation

install(
    TARGETS dx2
    EXPORT DX2Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

install(
    DIRECTORY include/dx2/
    DESTINATION include/dx2
    COMPONENT Runtime
    FILES_MATCHING PATTERN "*.hpp"
)

# Export targets
install(
    EXPORT DX2Targets
    FILE DX2Targets.cmake
    NAMESPACE dx2::
    DESTINATION lib/cmake/DX2
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/DX2Config.cmake"
    INSTALL_DESTINATION "lib/cmake/DX2"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DX2ConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/DX2Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/DX2ConfigVersion.cmake
    DESTINATION lib/cmake/DX2
)