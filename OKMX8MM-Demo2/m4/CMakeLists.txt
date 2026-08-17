cmake_minimum_required(VERSION 3.25)
project(okmx8mm_m4_demo1 VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
include(cmake/warnings.cmake)

add_subdirectory(demo1)
