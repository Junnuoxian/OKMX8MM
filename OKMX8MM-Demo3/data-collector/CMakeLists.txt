CMAKE_MINIMUM_REQUIRED (VERSION 3.0.0)

PROJECT(CRRC_DATA_COLLECTOR)
INCLUDE_DIRECTORIES(src test /opt/mysql-5.7.41/include)
LINK_DIRECTORIES(/opt/mysql-5.7.41/lib)

SET(CMAKE_SYSTEM_NAME linux)
SET(CMAKE_C_COMPILER armv7-imx6ul-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER armv7-imx6ul-linux-gnueabihf-g++)
SET(CMAKE_C_FLAGS "${CFLAGS} -g -O0 -Wno-implicit-function-declaration -lm -lc -lpthread -lmysqlclient ")
SET(CMAKE_CXX_FLAGS "${CXXFLAGS} -g -O0 -Wall -Wno-implicit-function-declaration -lm -lc -lpthread -lmysqlclient ")


ADD_LIBRARY(pthread SHARED IMPORTED GLOBAL)
ADD_LIBRARY(mysql SHARED IMPORTED GLOBAL)

FILE(GLOB_RECURSE MAIN_SRC_LIST RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c")
ADD_EXECUTABLE(crrc-data-collector ${MAIN_SRC_LIST})

FILE(GLOB_RECURSE MOCKER_SRC_LIST RELATIVE ${CMAKE_SOURCE_DIR} "test/*.c" "src/log/*.c" "src/signal-handler/*.c" "src/util/socketcan/*.c")
ADD_EXECUTABLE(crrc-data-mocker ${MOCKER_SRC_LIST})

