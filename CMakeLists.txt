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
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${INCLUDE_DIR}/*.h")

# Report how many sources were found (helpful when debugging CMake behavior)
list(LENGTH SOURCES SOURCES_COUNT)
message(STATUS "Discovered ${SOURCES_COUNT} C source files in src/")

# Create executable from all discovered sources (include headers so IDEs list them)
add_executable(SafeTrix ${SOURCES} ${HEADERS})

# Prefer modern target-based configuration
target_include_directories(SafeTrix PRIVATE ${INCLUDE_DIR})
# Require C11 for this target
target_compile_features(SafeTrix PRIVATE c_std_11)
