cmake_minimum_required(VERSION 3.20)
project(okmx8mm_a53_demo VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

add_library(a53_demo_core OBJECT
    src/file_utils.c
    src/m4_file_source.c
    src/m4_replay_source.c
    src/storage_writer.c
    src/mqtt_outbox.c
    src/can_trace.c
    src/pipeline.c
    src/cli.c)
target_include_directories(a53_demo_core PUBLIC include)
if(MSVC)
    target_compile_options(a53_demo_core PRIVATE /W4 /WX)
else()
    target_compile_options(a53_demo_core PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(okmx8mm-a53-demo src/main.c)
target_link_libraries(okmx8mm-a53-demo PRIVATE a53_demo_core)
if(MSVC)
    target_compile_options(okmx8mm-a53-demo PRIVATE /W4 /WX)
else()
    target_compile_options(okmx8mm-a53-demo PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(test_m4_replay tests/test_m4_replay.c)
target_link_libraries(test_m4_replay PRIVATE a53_demo_core)
add_test(NAME m4_replay COMMAND test_m4_replay)

add_executable(test_m4_file_source tests/test_m4_file_source.c)
target_link_libraries(test_m4_file_source PRIVATE a53_demo_core)
add_test(NAME m4_file_source COMMAND test_m4_file_source)

add_executable(test_outputs tests/test_outputs.c)
target_link_libraries(test_outputs PRIVATE a53_demo_core)
add_test(NAME outputs COMMAND test_outputs)

add_executable(test_pipeline tests/test_pipeline.c)
target_link_libraries(test_pipeline PRIVATE a53_demo_core)
add_test(NAME pipeline COMMAND test_pipeline)

add_executable(test_cli tests/test_cli.c)
target_link_libraries(test_cli PRIVATE a53_demo_core)
add_test(NAME cli COMMAND test_cli)
