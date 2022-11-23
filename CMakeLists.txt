cmake_minimum_required(VERSION 3.16)
project(stations)

################################
# Basic Compiler Flags
################################
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    set(stations-compile-flags
            /W3
            /DNOMINMAX
            )
else()
    set(stations-compile-flags
            -Wall
            -Wextra
            -Wno-unknown-pragmas
            -pedantic
            )
endif()

if(APPLE)
    # prevents ar from invoking ranlib, let CMake do it
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")

    # tell ranlib to ignore empty compilation units
    set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

################################
# Dependencies
################################
find_package(Threads)

if (MSVC)
    # PDB debug information is not supported by buildcache.
    # Store debug info in the object files.
    option(STATIONS_DEBUG_SYMBOLS "generate debug symbols (debug builds)" ON)
    if (STATIONS_DEBUG_SYMBOLS)
        set(STATIONS_MSVC_DEBUG_FLAGS "/Z7")
    else ()
        set(STATIONS_MSVC_DEBUG_FLAGS "")
    endif ()
    string(REPLACE "/Zi" "${STATIONS_MSVC_DEBUG_FLAGS}" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "${STATIONS_MSVC_DEBUG_FLAGS}" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "${STATIONS_MSVC_DEBUG_FLAGS}" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "${STATIONS_MSVC_DEBUG_FLAGS}" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif ()
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

################################
# Linter
################################
option(PPR_LINT "Run clang-tidy with the compiler." OFF)
if(PPR_LINT)
    # clang-tidy will be run on all targets defined hereafter
    include(cmake/clang-tidy.cmake)
endif()

################################
# benchmark executable
################################
file(GLOB_RECURSE stations-benchmark-files
        src/cmd/benchmark/*.cc
        )
add_executable(stations-benchmark ${stations-benchmark-files})
target_include_directories(stations-benchmark PUBLIC include)
target_link_libraries(stations-benchmark
        ${CMAKE_THREAD_LIBS_INIT}
        )
target_compile_features(stations-benchmark PUBLIC cxx_std_17)
set_target_properties(stations-benchmark PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(stations-benchmark PRIVATE ${stations-compile-flags})
target_compile_definitions(stations-benchmark PRIVATE ${stations-compile-definitions})

################################
# preprocess executable
################################
file(GLOB_RECURSE stations-preprocess-files
        src/cmd/preprocess/*.cc
        src/preprocessing/*.cc
        )
add_executable(stations-preprocess ${stations-preprocess-files} src/preprocessing/preprocessing.cc)
target_include_directories(stations-preprocess PUBLIC include)
target_link_libraries(stations-preprocess
        zlibstatic
        ${CMAKE_THREAD_LIBS_INIT}
        osmium
        boost
        boost-filesystem
        boost-iostreams
        protozero
        )
target_compile_features(stations-preprocess PUBLIC cxx_std_17)
set_target_properties(stations-preprocess PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(stations-preprocess PRIVATE ${stations-compile-flags})
target_compile_definitions(stations-preprocess PRIVATE ${stations-compile-definitions})