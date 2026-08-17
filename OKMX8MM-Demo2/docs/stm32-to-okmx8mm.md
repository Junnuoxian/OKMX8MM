# STM32F407 参考工程到 OKMX8MM Demo2 的对应关系

## 导师最终要什么

导师要的是一个矿车信号采集网关：

```text
采集模拟量和数字量 -> 存SD卡 -> 上传云平台 -> CAN发给矿车 -> 后续支持OTA
```

`China-STM32F407-ATK-EXPLORER` 是主参考工程。我们要仿照它的功能，不是逐行复制它的代码。

## 为什么要拆成 M4 和 A53

STM32 工程是单 MCU，所有功能都在一个系统里。

OKMX8MM 有 M4 和 A53：

- M4 适合做实时采集。
- A53 适合做文件、网络、CAN业务、升级和管理。

因此 Demo2 的分工是：

```text
M4 = 采集层
A53 = 网关层
```

## 模块对应表

| STM32参考模块 | 作用 | Demo2承接位置 |
| --- | --- | --- |
| `applications/main.c` | 主流程入口 | `a53` 和 `m4` 分别承接 |
| `applications/adc` | 模拟量采集 | `m4` |
| `applications/adc-storage` | 采集数据落盘 | `a53` |
| `applications/mqtt` | 云平台上传 | `a53` |
| `applications/can` | CAN发送和控制 | `a53` |
| `applications/heart-beat` | 在线心跳 | `a53` |
| `applications/ota` | 远程升级 | `a53` |
| `applications/device-settings` | 参数配置 | `a53` |
| `applications/daemon` | 看门和恢复 | `a53` |

## Demo2 当前数据格式

A53 当前使用统一批次格式：

```text
sequence,ai0,ai1,ai2,ai3,ai4,ai5,ai6,ai7,ai8,ai9,di_bits,speed_pulse_delta,speed_period_us
```

含义：

- `sequence`：批次序号。
- `ai0` 到 `ai9`：模拟量通道。
- `di_bits`：数字量位图。
- `speed_pulse_delta`：转速脉冲增量。
- `speed_period_us`：转速周期。

如果真实硬件只有 8 路模拟量，剩余两路先填 0。后续确认真实硬件有 10 路时，再扩展 M4 寄存器映射。

## 当前达到 80% 的含义

80% 不是说已经实车可用，而是说：

- 软件架构完整。
- M4 和 A53 分工清楚。
- A53 端流程能模拟跑通。
- 文件存储、MQTT队列、CAN帧、状态、心跳都有输出。
- 后续只需要把模拟输入替换成真实 M4 数据，再做硬件实测。

剩余 20% 主要是：

- M4 和 A53 真实通信联调。
- 外接采集板实测。
- SD卡长时间写入测试。
- MQTT真实云平台测试。
- CAN总线实测。
- OTA真实升级测试。

