cmake_minimum_required(VERSION 2.6)
project(ufo C)

set(TARNAME "ufo-filters")

set(UFO_FILTERS_VERSION_MAJOR "0")
set(UFO_FILTERS_VERSION_MINOR "6")
set(UFO_FILTERS_VERSION_PATCH "0")
set(UFO_FILTERS_VERSION_STRING_LONG "${UFO_FILTERS_VERSION_MAJOR}.${UFO_FILTERS_VERSION_MINOR}.${UFO_FILTERS_VERSION_PATCH}")
set(UFO_FILTERS_VERSION_STRING_SHORT "${UFO_FILTERS_VERSION_MAJOR}.${UFO_FILTERS_VERSION_MINOR}")

set(UFO_DESCRIPTION "UFO good filters")
set(UFO_DESCRIPTION_SUMMARY "UFO good filters")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common/cmake")

#{{{ Variables and Options
include(ConfigurePaths)
configure_paths(UFO_FILTERS)

set(UFO_FILTERS_PLUGINDIR "${UFO_FILTERS_LIBDIR}/ufo")
set(UFO_FILTERS_KERNELDIR "${UFO_FILTERS_DATADIR}/ufo")
set(PKG_UFO_CORE_MIN_REQUIRED "0.6")

option(WITH_PROFILING "Enable profiling" OFF)

if (WITH_PROFILING)
    add_definitions("-pg")
    set(CMAKE_C_FLAGS "-pg")
endif ()
#}}}
#{{{ Common dependencies
find_package(OpenCL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UFO ufo>=${PKG_UFO_CORE_MIN_REQUIRED} REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/oclfft)

link_directories(${UFO_LIBRARY_DIRS})
#}}}
#{{{ Global definitions
add_definitions("-std=c99 -pedantic -Wall -Wextra -fPIC")
add_definitions(-DG_LOG_DOMAIN="Ufo")

if (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    add_definitions("-Wno-unused-parameter")
endif ()

#}}}
#{{{ Subdirectories
add_subdirectory(deps)
add_subdirectory(src)
add_subdirectory(tests/integration_tests)
#}}}
