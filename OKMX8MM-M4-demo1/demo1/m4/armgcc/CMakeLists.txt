cmake_minimum_required(VERSION 3.10.0)
project(demo1_m4)

set(MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION 2)
set(MCUXPRESSO_CMAKE_FORMAT_MINOR_VERSION 0)

include(ide_overrides.cmake OPTIONAL)

if(CMAKE_SCRIPT_MODE_FILE)
    message("${MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION}")
    return()
endif()

enable_language(ASM)

set(CMAKE_STATIC_LIBRARY_PREFIX)
set(CMAKE_STATIC_LIBRARY_SUFFIX)
set(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
set(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${CMAKE_BUILD_TYPE})
set(MCUX_SDK_PROJECT_NAME demo1_m4.elf)

if(NOT DEFINED SdkRootDirPath)
    set(SdkRootDirPath ${ProjDirPath}/../../../../..)
endif()

include(${ProjDirPath}/flags.cmake)
include(${ProjDirPath}/config.cmake)

add_executable(${MCUX_SDK_PROJECT_NAME}
    "${ProjDirPath}/../demo1_m4_main.c"
    "${ProjDirPath}/../demo1_board_hooks.c"
    "${ProjDirPath}/../board.c"
    "${ProjDirPath}/../clock_config.c"
    "${ProjDirPath}/../pin_mux.c"
    "${ProjDirPath}/../empty_rsc_table.c"
    "${ProjDirPath}/../../src/demo1_console.c"
    "${ProjDirPath}/../../src/demo1_engine.c"
    "${ProjDirPath}/../../src/demo1_modbus.c"
    "${ProjDirPath}/../../src/demo1_modbus_source.c"
    "${ProjDirPath}/../../src/demo1_mock_source.c"
    "${ProjDirPath}/../../src/demo1_rs485_transport.c"
    "${ProjDirPath}/../../src/demo1_wheel_board_source.c"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/..
    ${ProjDirPath}/../../include
)

include(${SdkRootDirPath}/tools/cmake_toolchain_files/mcux_config.cmake)
include(${SdkRootDirPath}/devices/MIMX8MM6/all_lib_device.cmake)

if(NOT DEFINED TARGET_LINK_SYSTEM_LIBRARIES)
    set(TARGET_LINK_SYSTEM_LIBRARIES "-lm -lc -lgcc -lnosys")
endif()

target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES})
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

add_custom_command(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary
    ${EXECUTABLE_OUTPUT_PATH}/${MCUX_SDK_PROJECT_NAME}
    ${EXECUTABLE_OUTPUT_PATH}/demo1_m4.bin)
