# STM32F407

负责 10 路 ADC、2 路数字量、TIM4 转速、CAN 发送和 USART3 RS485/Modbus 从机响应。

## 默认引脚

- ADC1：PA0 至 PA7、PB0、PB1，对应 ADC 通道 0 至 9。
- 数字量：PC6、PC7。
- 转速：TIM4，PB6/PB7。
- RS485：USART3，PB10/PB11；收发控制 PB12。
- CAN1：PB8/PB9，默认 500 kbit/s。

这组引脚是可编译的默认方案，接线前必须和实际 PCB 对照；如果 PA0 至 PA7 被其他外设占用，只修改 `Bsp/demo3_stm32f407_board.c` 的 ADC 引脚初始化和通道表。

## 数据流程

1. `demo3_acquisition_read` 读取 ADC、数字量和转速。
2. `demo3_can_send_sample` 发送 6 个标准 CAN 帧。
3. `demo3_modbus_slave_poll` 响应 M4 的 26 个寄存器读取请求。
4. M4 读取后通过 RPMsg 转给 A53。

## 接入参考工程

将 `SConscript` 加入工程构建，并把 `Core/Src/main.c` 中的初始化循环作为独立启动入口；如果使用已有 RT-Thread 主程序，只加入 `Bsp` 和 `applications` 源文件，不重复加入 `Core/Src/main.c`。

当前代码已经是真实 HAL 调用，不包含具体 PCB 的最终确认；首次上电前先确认 ADC、RS485 收发控制、CAN 收发器和 TIM4 引脚没有复用冲突。
