cmake_minimum_required(VERSION 3.16)

project(DEMO3_STM32F407_HAL LANGUAGES C)

set(DEMO3_HAL_ROOT "" CACHE PATH "STM32F4xx_HAL package root")
set(DEMO3_RTTHREAD_ROOT "" CACHE PATH "RT-Thread root")
set(DEMO3_BOARD_INCLUDE_DIR "" CACHE PATH "Board generated include directory")
set(DEMO3_REFERENCE_ROOT "" CACHE PATH "Reference project root for rtconfig.h")

if(DEMO3_HAL_ROOT STREQUAL "" OR DEMO3_RTTHREAD_ROOT STREQUAL "" OR
   DEMO3_BOARD_INCLUDE_DIR STREQUAL "" OR DEMO3_REFERENCE_ROOT STREQUAL "")
    message(FATAL_ERROR
        "Set DEMO3_HAL_ROOT, DEMO3_RTTHREAD_ROOT, DEMO3_BOARD_INCLUDE_DIR and DEMO3_REFERENCE_ROOT")
endif()

add_library(demo3_stm32f407 OBJECT
    Core/Src/main.c
    Bsp/demo3_stm32f407_board.c
    applications/acquisition/demo3_acquisition.c
    applications/can/demo3_can.c
    applications/rs485/demo3_modbus_codec.c
    applications/rs485/demo3_modbus_slave.c
    ../common/protocol/demo3_protocol.c
)

target_compile_features(demo3_stm32f407 PRIVATE c_std_11)
target_compile_definitions(demo3_stm32f407 PRIVATE STM32F407xx USE_HAL_DRIVER)
target_include_directories(demo3_stm32f407 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Bsp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/applications/acquisition
    ${CMAKE_CURRENT_SOURCE_DIR}/applications/can
    ${CMAKE_CURRENT_SOURCE_DIR}/applications/rs485
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/protocol
    ${DEMO3_REFERENCE_ROOT}
    ${DEMO3_BOARD_INCLUDE_DIR}
    ${DEMO3_RTTHREAD_ROOT}/include
    ${DEMO3_RTTHREAD_ROOT}/components
    ${DEMO3_RTTHREAD_ROOT}/components/legacy
    ${DEMO3_RTTHREAD_ROOT}/components/drivers/include
    ${DEMO3_RTTHREAD_ROOT}/components/finsh
    ${DEMO3_HAL_ROOT}/STM32F4xx_HAL_Driver/Inc
    ${DEMO3_HAL_ROOT}/CMSIS/Device/ST/STM32F4xx/Include
    ${DEMO3_HAL_ROOT}/CMSIS/Include
)
