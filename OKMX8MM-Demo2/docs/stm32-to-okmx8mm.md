# STM32F407 参考工程到 OKMX8MM Demo2 的对应关系

## 导师最终要什么

导师要的是一套矿车数据采集系统：

```text
采集模拟量和数字量 -> 存SD卡 -> 上传云平台 -> CAN发给矿车 -> 后续支持OTA
```

导师给的 STM32 工程是主参考。我们要保留它的功能思路，但不是逐行复制它的代码。

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

## Demo2 数据格式

A53 使用统一批次格式：

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

## 操作顺序

新手按这个顺序推进：

1. 先跑 Demo2 总自检，确认电脑端流程正常。
2. 再把 A53 程序放到开发板 Linux 里运行。
3. 确认 SD 卡能写入 `samples.jsonl`、`status.json`、`heartbeat.jsonl`。
4. 确认 MQTT 待发文件能被脚本上传到云平台。
5. 确认 CAN 记录能转换成真实 CAN 帧发出。
6. 再启动 M4，确认 M4 调试串口输出正常。
7. 接外接采集板，确认 M4 能读取模拟量和数字量。
8. 最后把 M4 数据送到 A53，检查 SD、云平台、CAN 三路输出。
