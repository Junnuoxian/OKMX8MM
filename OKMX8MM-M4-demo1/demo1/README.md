# Demo1 M4 Acquisition

`demo1` 是一个最小可跑的 M4 采集骨架，先把采集节拍、批次拼装和串口输出跑通，A53 侧功能后续再接 RPMsg、存储和网络。

本项目的硬件采集板是“电动轮第三版硬件板”，OKMX8MM 不直接接模拟量，OKMX8MM 通过 RS485 读取这块硬件板整理好的采集数据。

当前边界只有三件事：

- 每 `500 us` 采样一次
- 每 `10` 个 tick 组成一批
- 通过 UART4 输出批次状态
- 已预留 RS485/Modbus RTU 采集源
- 已新增电动轮第三版硬件板采集源

目录说明：

- `include/`：对外头文件
- `src/`：主机侧可运行的采集引擎、模拟源和状态输出
- `m4/`：M4 裸机入口、板级钩子和 SDK 工程模板
- `tests/`：Windows 主机测试

电动轮第三版硬件板 RS485/Modbus 默认参数：

- 协议：Modbus RTU
- 功能码：`0x03`，读保持寄存器
- 默认站号：`1`
- 起始寄存器：`0`
- 读取数量：`10`
- 映射方式：寄存器 `0~7` 对应 AI `1~8`
- 寄存器 `8`：DI `1~2`，低2位有效
- 寄存器 `9`：SPEED `1`
- 已实现文件：`demo1_modbus.c`、`demo1_modbus_source.c`、`demo1_wheel_board_source.c`

真实连接电动轮第三版硬件板时，只需要补 UART3/RS485 读写回调：

```c
demo1_rs485_transport_init(&transport, port, write_bytes, read_byte, now_us, 100000);
demo1_wheel_board_source_init(&source,
                              &transport,
                              demo1_rs485_transport_write,
                              demo1_rs485_transport_read,
                              1,
                              0);
demo1_engine_init_with_source(&engine, &source, demo1_wheel_board_source_read_tick);
```

注意：当前按电路图已确认的 `8路4-20mA + 2路开关量 + 1路转速` 做真实采集框架，不再按10路模拟量写死。如果实物后续确认还有2路模拟量，再扩展寄存器映射。

M4侧默认参数：

- 业务串口：UART3
- 调试串口：UART4
- UART3默认波特率：`9600`
- 如果硬件板协议不同，编译时覆盖 `DEMO1_RS485_UART_BAUDRATE`
- UART3引脚：`UART3_RXD`、`UART3_TXD`

注意：RJ45 网口的 `A+`、`B-` 不是 RS485，不要接到网口。

主机侧验证命令：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Test
```

M4交叉编译命令：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-m4.ps1 `
    -SdkRoot '<SDK_ROOT>' `
    -ArmToolchainRoot '<ARM_TOOLCHAIN_ROOT>'
```

没有安装 `arm-none-eabi-gcc` 时，脚本会直接提示缺少工具，不会生成虚假的M4构建结果。

主机可执行文件：

```text
build/windows/demo1/demo1-host.exe
```

M4 侧工程位于：

```text
demo1/m4/armgcc
```

它基于 NXP SDK 2.16 的 EVK-MIMX8MM M4 裸机模板，默认使用：

- UART4
- 500 us SysTick
- 80 MHz M4 root clock

说明：

- 主机测试已经可运行
- M4 侧现在已经补齐工程骨架和板级启动代码
- Modbus 协议层和采集源已加入主机测试
- 电动轮第三版硬件板采集源已加入主机测试
- RS485传输层已加入主机测试
- M4主循环已切到电动轮第三版硬件板采集源
- 真正交叉编译还需要外部 `arm-none-eabi-gcc` 和 NXP SDK
- UART3/RS485 底层函数已经接入 NXP SDK UART API，仍需要上板确认A/B线、波特率、校验位和硬件板寄存器编号
