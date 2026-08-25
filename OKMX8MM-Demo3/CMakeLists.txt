cmake_minimum_required(VERSION 3.16)

project(OKMX8MM_Demo3_Protocol LANGUAGES C)

add_library(demo3_m4_gateway STATIC
    okmx8mm-m4/src/demo3_m4_gateway.c
    okmx8mm-m4/src/rs485/demo3_m4_modbus.c
    okmx8mm-m4/src/rpmsg/demo3_m4_rpmsg.c
)
target_include_directories(demo3_m4_gateway PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/okmx8mm-m4/include
    ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
)

add_library(demo3_protocol STATIC
    common/protocol/demo3_protocol.c
)

target_include_directories(demo3_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
)

add_library(demo3_can STATIC
    data-collector-demo3/src/can/demo3_can.c
)
target_include_directories(demo3_can PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src/can
    ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
)
target_link_libraries(demo3_can PUBLIC demo3_protocol)

add_library(demo3_mqtt STATIC
    data-collector-demo3/src/mqtt/demo3_mqtt.c
)
target_include_directories(demo3_mqtt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src/mqtt
    ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
)
target_link_libraries(demo3_mqtt PUBLIC demo3_protocol)

add_library(demo3_runtime_status STATIC
    data-collector-demo3/src/runtime/demo3_runtime_status.c
)
target_include_directories(demo3_runtime_status PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src
)

add_library(demo3_ota STATIC
    data-collector-demo3/src/ota/demo3_ota.c
)
target_include_directories(demo3_ota PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src
)

add_library(demo3_a53_bridge STATIC
    data-collector-demo3/src/bridge/demo3_m4_rpmsg.c
    data-collector-demo3/src/bridge/demo3_rpmsg_bridge.c
    data-collector-demo3/src/bridge/demo3_rpmsg_reader.c
    data-collector-demo3/src/bridge/demo3_storage.c
    data-collector-demo3/src/bridge/demo3_rpmsg_storage.c
)
target_include_directories(demo3_a53_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src/bridge
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src
    ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
)
target_link_libraries(demo3_a53_bridge PUBLIC demo3_protocol)

add_library(demo3_config STATIC
    data-collector-demo3/src/config/demo3_config.c
)

target_include_directories(demo3_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src
)

option(DEMO3_BUILD_HOST_TESTS "Build host-side protocol tests" ON)

if(DEMO3_BUILD_HOST_TESTS)
    enable_testing()
    add_executable(test_demo3_protocol
        tests/test_demo3_protocol.c
    )
    target_link_libraries(test_demo3_protocol PRIVATE demo3_protocol)
    add_test(NAME demo3_protocol COMMAND test_demo3_protocol)

    add_executable(test_demo3_config
        data-collector-demo3/tests/test_demo3_config.c
    )
    target_link_libraries(test_demo3_config PRIVATE demo3_config)
    add_test(NAME demo3_config COMMAND test_demo3_config)

    add_executable(test_demo3_modbus_adapter
        data-collector-demo3/tests/test_demo3_modbus_adapter.c
        data-collector-demo3/src/bridge/demo3_modbus_adapter.c
    )
    target_include_directories(test_demo3_modbus_adapter PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/data-collector-demo3/src
        ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
    )
    add_test(NAME demo3_modbus_adapter COMMAND test_demo3_modbus_adapter)

    add_executable(test_demo3_m4_modbus
        tests/test_demo3_m4_modbus.c
        okmx8mm-m4/src/rs485/demo3_m4_modbus.c
    )
    target_include_directories(test_demo3_m4_modbus PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/okmx8mm-m4/include
        ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
    )
    add_test(NAME demo3_m4_modbus COMMAND test_demo3_m4_modbus)

    add_executable(test_demo3_m4_rpmsg
        tests/test_demo3_m4_rpmsg.c
        okmx8mm-m4/src/rpmsg/demo3_m4_rpmsg.c
    )
    target_include_directories(test_demo3_m4_rpmsg PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/okmx8mm-m4/include
        ${CMAKE_CURRENT_SOURCE_DIR}/common/protocol
    )
    add_test(NAME demo3_m4_rpmsg COMMAND test_demo3_m4_rpmsg)

    add_executable(test_demo3_a53_bridge
        data-collector-demo3/tests/test_demo3_a53_bridge.c
    )
    target_link_libraries(test_demo3_a53_bridge PRIVATE demo3_a53_bridge)
    add_test(NAME demo3_a53_bridge COMMAND test_demo3_a53_bridge)

    add_executable(test_demo3_rpmsg_reader
        data-collector-demo3/tests/test_demo3_rpmsg_reader.c
    )
    target_link_libraries(test_demo3_rpmsg_reader PRIVATE demo3_a53_bridge)
    add_test(NAME demo3_rpmsg_reader COMMAND test_demo3_rpmsg_reader)

    add_executable(test_demo3_can
        data-collector-demo3/tests/test_demo3_can.c
    )
    target_link_libraries(test_demo3_can PRIVATE demo3_can)
    add_test(NAME demo3_can COMMAND test_demo3_can)

    add_executable(test_demo3_mqtt
        data-collector-demo3/tests/test_demo3_mqtt.c
    )
    target_link_libraries(test_demo3_mqtt PRIVATE demo3_mqtt)
    add_test(NAME demo3_mqtt COMMAND test_demo3_mqtt)

    add_executable(test_demo3_runtime_status
        data-collector-demo3/tests/test_demo3_runtime_status.c
    )
    target_link_libraries(test_demo3_runtime_status PRIVATE demo3_runtime_status)
    add_test(NAME demo3_runtime_status COMMAND test_demo3_runtime_status)

    add_executable(test_demo3_ota
        data-collector-demo3/tests/test_demo3_ota.c
    )
    target_link_libraries(test_demo3_ota PRIVATE demo3_ota)
    add_test(NAME demo3_ota COMMAND test_demo3_ota)
endif()
