cmake_minimum_required(VERSION 3.20)
project(SafeTrix C)

set(CMAKE_C_STANDARD 11)

# Place runtime binaries in build/bin for convenience
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directory for headers
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
include_directories(${INCLUDE_DIR})

# Discover all .c source files under src/ (recursively) and header files under include/
# CONFIGURE_DEPENDS makes generators re-run when files are added/removed during development
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${INCLUDE_DIR}/*.h")

# Report how many sources were found (helpful when debugging CMake behavior)
list(LENGTH ALL_SOURCES ALL_SOURCES_COUNT)
message(STATUS "Discovered ${ALL_SOURCES_COUNT} C source files in src/")

# MAIN_SOURCES: exclude test sources
set(MAIN_SOURCES ${ALL_SOURCES}
        src/ui/startup.c)
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*/src/tests/.*\\.c$")

# TEST_SOURCES: include everything except main.c (so tests can link core implementation)
set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/src/main\\.c$")

# Create executable for main application using MAIN_SOURCES
add_executable(SafeTrix ${MAIN_SOURCES} ${HEADERS} src/data/persistence.h)

# Create tests executable linking core implementation (no main.c)
add_executable(SafeTrixTests ${TEST_SOURCES} src/tests/test_security_flow.c)

if (MSVC)
    target_compile_options(SafeTrix PRIVATE /utf-8)
    target_compile_options(SafeTrixTests PRIVATE /utf-8)
endif ()

# Prefer modern target-based configuration
target_include_directories(SafeTrix PRIVATE ${INCLUDE_DIR})
target_include_directories(SafeTrixTests PRIVATE ${INCLUDE_DIR})
# Require C11 for these targets
target_compile_features(SafeTrix PRIVATE c_std_11)
target_compile_features(SafeTrixTests PRIVATE c_std_11)
