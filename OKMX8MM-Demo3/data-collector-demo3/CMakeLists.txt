cmake_minimum_required(VERSION 3.16)

project(DEMO3_DATA_COLLECTOR C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DEMO3_MYSQL_INCLUDE_DIR "" CACHE PATH "MySQL client include directory")
set(DEMO3_MYSQL_LIBRARY_DIR "" CACHE PATH "MySQL client library directory")

if(DEMO3_MYSQL_INCLUDE_DIR STREQUAL "" OR DEMO3_MYSQL_LIBRARY_DIR STREQUAL "")
    message(FATAL_ERROR "Set DEMO3_MYSQL_INCLUDE_DIR and DEMO3_MYSQL_LIBRARY_DIR before building the collector")
endif()

find_package(Threads REQUIRED)

include_directories(
    src
    test
    "${DEMO3_MYSQL_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/protocol"
)
link_directories("${DEMO3_MYSQL_LIBRARY_DIR}")

file(GLOB_RECURSE MAIN_SRC_LIST CONFIGURE_DEPENDS "src/*.c")
list(APPEND MAIN_SRC_LIST
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/protocol/demo3_protocol.c"
)
add_executable(crrc-data-collector ${MAIN_SRC_LIST})
target_link_libraries(crrc-data-collector PRIVATE Threads::Threads m mysqlclient)

file(GLOB_RECURSE MOCKER_SRC_LIST CONFIGURE_DEPENDS
    "test/*.c"
    "src/log/*.c"
    "src/signal-handler/*.c"
    "src/util/socketcan/*.c"
)
add_executable(crrc-data-mocker ${MOCKER_SRC_LIST})
target_link_libraries(crrc-data-mocker PRIVATE Threads::Threads m)
