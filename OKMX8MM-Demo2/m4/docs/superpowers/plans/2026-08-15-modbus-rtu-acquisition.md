# Modbus RTU Acquisition Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Modbus RTU protocol layer so `demo1` can read the external acquisition board data over RS485: 8 analog inputs, 2 digital inputs, and 1 speed signal.

**Architecture:** Keep the acquisition engine independent from the wire protocol. Add a small `demo1_modbus` module for CRC, request-frame creation, response parsing, and conversion into one tick sample. Keep the current mock source for host testing and fallback M4 bring-up.

**Tech Stack:** C11, CMake/Ninja, existing lightweight C test harness, OKMX8MM M4 ARM GCC project.

## Global Constraints

Use Modbus RTU function `0x03` to read holding registers.
Default request is slave `1`, start register `0`, register count `10`.
Do not treat the RJ45 Ethernet `A+`/`B-` pins as RS485.
Keep UART2 free for A53 debug; use UART3 for the board RS485 wiring.
Keep M4 code buildable with the existing mock source until real UART3 RS485 driver work begins.

---

### Task 1: Protocol Module

**Files:**
- Create: `demo1/include/demo1_modbus.h`
- Create: `demo1/src/demo1_modbus.c`
- Create: `demo1/tests/test_demo1_modbus.c`
- Modify: `demo1/CMakeLists.txt`
- Modify: `demo1/m4/armgcc/CMakeLists.txt`

**Interfaces:**
- Produces: `uint16_t demo1_modbus_crc16(const uint8_t *data, size_t length)`
- Produces: `int demo1_modbus_build_read_request(uint8_t slave_id, uint16_t start_register, uint16_t register_count, uint8_t *out_frame, size_t out_capacity)`
- Produces: `int demo1_modbus_parse_read_response(uint8_t slave_id, const uint8_t *frame, size_t frame_length, uint16_t *out_registers, size_t max_registers, uint16_t *out_count)`
- Produces: `int demo1_modbus_registers_to_tick_sample(const uint16_t *registers, uint16_t register_count, demo1_tick_sample_t *out_sample)`

- [ ] **Step 1: Write failing tests**

Add tests for CRC, request-frame bytes, 10-register response parsing, CRC rejection, and register-to-sample conversion.

- [ ] **Step 2: Run tests and verify red**

Run: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Clean -Test`

Expected: build fails because `demo1_modbus.h` or functions are not implemented yet.

- [ ] **Step 3: Implement minimal protocol module**

Add the header and source with only the functions required by the tests.

- [ ] **Step 4: Run tests and verify green**

Run: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Clean -Test`

Expected: all CTest tests pass.

### Task 2: Handoff Notes

**Files:**
- Modify: `demo1/README.md`
- Modify: hardware wiring confirmation notes

**Interfaces:**
- Consumes: Task 1 Modbus function names and default request settings.
- Produces: Newcomer-readable hardware/software handoff notes.

- [ ] **Step 1: Document defaults**

Write the default slave, baud placeholder, register mapping, and RS485 wiring warning.

- [ ] **Step 2: Verify documentation contains no RJ45-as-RS485 ambiguity**

Search for `RJ45` and `RS485` in the edited files and confirm the warning is explicit.
