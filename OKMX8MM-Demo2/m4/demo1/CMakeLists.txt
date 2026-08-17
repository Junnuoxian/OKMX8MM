add_executable(demo1-host
    src/demo1_console.c
    src/demo1_engine.c
    src/demo1_host_main.c
    src/demo1_modbus.c
    src/demo1_modbus_source.c
    src/demo1_mock_source.c
    src/demo1_rs485_transport.c
    src/demo1_wheel_board_source.c)
target_include_directories(demo1-host PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(demo1-host)

add_executable(test_demo1_mock_source
    tests/test_demo1_mock_source.c
    src/demo1_mock_source.c)
target_include_directories(test_demo1_mock_source PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_mock_source)
add_test(NAME demo1_mock_source COMMAND test_demo1_mock_source)

add_executable(test_demo1_engine
    tests/test_demo1_engine.c
    src/demo1_engine.c
    src/demo1_mock_source.c
    src/demo1_console.c)
target_include_directories(test_demo1_engine PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_engine)
add_test(NAME demo1_engine COMMAND test_demo1_engine)

add_executable(test_demo1_modbus
    tests/test_demo1_modbus.c
    src/demo1_modbus.c)
target_include_directories(test_demo1_modbus PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_modbus)
add_test(NAME demo1_modbus COMMAND test_demo1_modbus)

add_executable(test_demo1_modbus_source
    tests/test_demo1_modbus_source.c
    src/demo1_modbus_source.c
    src/demo1_modbus.c)
target_include_directories(test_demo1_modbus_source PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_modbus_source)
add_test(NAME demo1_modbus_source COMMAND test_demo1_modbus_source)

add_executable(test_demo1_wheel_board_source
    tests/test_demo1_wheel_board_source.c
    src/demo1_wheel_board_source.c
    src/demo1_modbus.c)
target_include_directories(test_demo1_wheel_board_source PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_wheel_board_source)
add_test(NAME demo1_wheel_board_source COMMAND test_demo1_wheel_board_source)

add_executable(test_demo1_rs485_transport
    tests/test_demo1_rs485_transport.c
    src/demo1_rs485_transport.c
    src/demo1_modbus.c)
target_include_directories(test_demo1_rs485_transport PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/tests)
demo_enable_warnings(test_demo1_rs485_transport)
add_test(NAME demo1_rs485_transport COMMAND test_demo1_rs485_transport)
