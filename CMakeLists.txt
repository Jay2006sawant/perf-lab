# SPDX-License-Identifier: CC-BY-4.0
cmake_minimum_required(VERSION 3.16)
project(ran_perf_lab C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RAN_PERF_LAB_NATIVE "Tune for host CPU (-march=native)" ON)

add_executable(ran_perf_lab
  src/main.c
  src/timing.c
  src/dotproduct.c
  src/cplx_dot.c
  src/phaserot.c
  src/rng.c
  src/bench.c
  src/manifest.c
)

target_include_directories(ran_perf_lab PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ran_perf_lab PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(RAN_PERF_LAB_NATIVE)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ran_perf_lab PRIVATE -O3 -march=native)
  endif()
else()
  target_compile_options(ran_perf_lab PRIVATE -O2)
endif()

include(CheckCCompilerFlag)
check_c_compiler_flag(-mavx2 HAVE_COMPILER_AVX2)
if(HAVE_COMPILER_AVX2)
  target_compile_options(ran_perf_lab PRIVATE -mavx2 -mfma)
  target_compile_definitions(ran_perf_lab PRIVATE RAN_PERF_HAVE_AVX2=1)
endif()

target_link_libraries(ran_perf_lab PRIVATE m)

enable_testing()
add_test(NAME ran_perf_lab_verify COMMAND ran_perf_lab verify)
add_test(NAME ran_perf_lab_manifest COMMAND ran_perf_lab manifest --output ${CMAKE_CURRENT_BINARY_DIR}/manifest.json)
add_test(NAME ran_perf_lab_bench_smoke COMMAND ran_perf_lab bench --iterations 5 --warmup 2 --seed 1 --output ${CMAKE_CURRENT_BINARY_DIR}/smoke_bench.json)

add_custom_target(ran_perf_lab_ci
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH= ${CMAKE_CURRENT_SOURCE_DIR}/python
          python3 ${CMAKE_CURRENT_SOURCE_DIR}/python/validate_json.py ${CMAKE_CURRENT_BINARY_DIR}/smoke_bench.json
  DEPENDS ran_perf_lab
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Local CI: tests + JSON validation"
)
