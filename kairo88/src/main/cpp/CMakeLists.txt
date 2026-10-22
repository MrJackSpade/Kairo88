cmake_minimum_required(VERSION 3.22.1)
project(kairo88 LANGUAGES C CXX)

get_filename_component(KAIRO88_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
include("${KAIRO88_ROOT}/shared/cmake/KairoNativeDefaults.cmake")

# The QUASI88 core with ymfm sound (core/), built for Android.
add_subdirectory("${KAIRO88_ROOT}/core" core)
kairo_native_debug_defaults(kairo88_core)

add_library(kairo88 SHARED native_bridge.cpp)
target_compile_features(kairo88 PRIVATE cxx_std_17)
kairo_native_debug_defaults(kairo88)
# The shared frame host, audio output and Slang presenter.
target_include_directories(kairo88 PRIVATE
    "${KAIRO88_ROOT}/shared/frontend/src/main/cpp"
    "${KAIRO88_ROOT}/shared/third_party/librashader/include")
target_link_libraries(kairo88 PRIVATE kairo88_core log android aaudio EGL GLESv3 dl)
