# Warn when the conda environment the build uses has drifted from the pins.
#
# environment.yml carries every version, so a pull that moves a pin leaves
# an existing environment behind until it is updated. The build then
# compiles against the old packages and fails in ways that do not name the
# cause, such as a link error between two fmt majors. This compares each pin
# against the environment and names every package that does not match.
#
# conda records each installed package as
# conda-meta/<name>-<version>-<build>.json, so a pin is met when a file with
# its name and version exists. conda reads name=1.2 as 1.2 or any 1.2.x, and
# a wildcard pin such as hdf5=1.14.* becomes the same glob. The environment
# is the prefix holding the C++ compiler, since that is where the headers
# come from; a compiler from outside a conda environment has no conda-meta
# beside it and the check is skipped. The same file is solved on Linux and
# macOS, so every pin applies on both.
#
# It only warns, since building against a different environment on purpose
# is allowed. Only the pinned packages are checked, not what they pull in.
# The pin file and the environment's history are configure dependencies, so
# a pull that moves a pin or an update to the environment reruns the check.

include_guard(GLOBAL)

# The environment is the prefix above the compiler's bin directory
cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH _dx2_env_bin)
cmake_path(GET _dx2_env_bin PARENT_PATH _dx2_env)
if(NOT EXISTS "${_dx2_env}/conda-meta/history")
    return()
endif()

set(_dx2_pin_file "${CMAKE_SOURCE_DIR}/environment.yml")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${_dx2_pin_file}" "${_dx2_env}/conda-meta/history")

# Each flat "  - name=version" entry, the only form the update script writes
file(STRINGS "${_dx2_pin_file}" _dx2_pins REGEX "^  - [A-Za-z0-9_.-]+=")
set(_dx2_meta "${_dx2_env}/conda-meta")
set(_dx2_stale "")
foreach(_dx2_pin IN LISTS _dx2_pins)
    string(REGEX REPLACE "#.*" "" _dx2_pin "${_dx2_pin}")
    string(REGEX MATCH "^  - ([A-Za-z0-9_.-]+)=([^ ]+)" _dx2_unused "${_dx2_pin}")
    set(_dx2_name "${CMAKE_MATCH_1}")
    set(_dx2_version "${CMAKE_MATCH_2}")

    # The pinned version itself, or any release under it
    file(GLOB _dx2_found
        "${_dx2_meta}/${_dx2_name}-${_dx2_version}-*.json"
        "${_dx2_meta}/${_dx2_name}-${_dx2_version}.*-*.json")
    if(_dx2_found)
        continue()
    endif()

    # Name the version installed instead. A version starts with a digit,
    # which keeps hdf5 from matching hdf5-external-filter-plugins.
    file(GLOB _dx2_installed RELATIVE "${_dx2_meta}" "${_dx2_meta}/${_dx2_name}-*.json")
    list(FILTER _dx2_installed INCLUDE REGEX "^${_dx2_name}-[0-9][^-]*-[^-]+\\.json$")
    if(_dx2_installed)
        list(GET _dx2_installed 0 _dx2_have)
        string(REGEX REPLACE "^${_dx2_name}-([^-]+)-.*" "\\1" _dx2_have "${_dx2_have}")
    else()
        set(_dx2_have "not installed")
    endif()
    list(APPEND _dx2_stale "${_dx2_name} ${_dx2_have}, pinned ${_dx2_version}")
endforeach()

if(_dx2_stale)
    list(JOIN _dx2_stale "\n  " _dx2_report)
    message(WARNING
        "The conda environment at ${_dx2_env} does not match the pins in "
        "environment.yml:\n  ${_dx2_report}\n"
        "Update it with:\n  mamba env update -p ${_dx2_env} -f ${_dx2_pin_file}")
endif()
