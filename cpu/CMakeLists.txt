# cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
# cmake --build build
#
# cmake -B build-debug -S . -DCMAKE_BUILD_TYPE=Debug
# cmake --build build-debug

cmake_minimum_required(VERSION 3.20)

project(PathTracerCPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(glm CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
# Exclude every build dir (build/, build-debug/, ...) and CMake's own scratch sources
list(FILTER SOURCES EXCLUDE REGEX "^${CMAKE_CURRENT_SOURCE_DIR}/build[^/]*/")
list(FILTER SOURCES EXCLUDE REGEX "/CMakeFiles/")

add_executable(pathtracer ${SOURCES})
target_link_libraries(pathtracer PRIVATE glm::glm)

target_compile_options(pathtracer PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror=return-type>
  $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3 -march=native>
  $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Debug>>:-O0 -g3 -fno-omit-frame-pointer>
)

# Debug: bounds-checked STL + AddressSanitizer/UBSan so crashes report file:line
option(PT_SANITIZE "Enable ASan/UBSan in Debug builds" ON)
target_compile_definitions(pathtracer PRIVATE
  $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
)
if(PT_SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(pathtracer PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
  target_link_options(pathtracer PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

# Tests: ctest --test-dir build --output-on-failure
# In a Debug build these also run under ASan/UBSan, so memory errors fail the test.
enable_testing()

set(PT_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test-output)
file(MAKE_DIRECTORY ${PT_TEST_DIR})

# Delete any stale image first so render_writes_png can't pass on an old file
add_test(NAME clean_render_output
  COMMAND ${CMAKE_COMMAND} -E rm -f test.png
  WORKING_DIRECTORY ${PT_TEST_DIR})

# Renders the built-in scene; fails on non-zero exit, crash, or sanitizer report
add_test(NAME render_smoke
  COMMAND pathtracer
  WORKING_DIRECTORY ${PT_TEST_DIR})

# md5sum fails if the file doesn't exist
add_test(NAME render_writes_png
  COMMAND ${CMAKE_COMMAND} -E md5sum test.png
  WORKING_DIRECTORY ${PT_TEST_DIR})

set_tests_properties(clean_render_output PROPERTIES FIXTURES_SETUP render_clean)
set_tests_properties(render_smoke PROPERTIES
  FIXTURES_REQUIRED render_clean
  FIXTURES_SETUP render_output
  TIMEOUT 300)
set_tests_properties(render_writes_png PROPERTIES FIXTURES_REQUIRED render_output)
