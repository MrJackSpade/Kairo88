cmake_minimum_required(VERSION 3.22.1)
project(kairo88 LANGUAGES C CXX)

get_filename_component(KAIRO88_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
include("${KAIRO88_ROOT}/shared/cmake/KairoMachine.cmake")

# The QUASI88 core with ymfm sound (core/), built for Android.
add_subdirectory("${KAIRO88_ROOT}/core" core)
kairo_native_debug_defaults(kairo88_core)

kairo_machine_library(kairo88 pc88_core.cpp)
target_link_libraries(kairo88 PRIVATE kairo88_core)
