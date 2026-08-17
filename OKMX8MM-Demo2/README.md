# OKMX8MM Demo2 矿车采集网关

## 一句话说明

本文件夹是 Demo2 主交付包，目标是把导师要求的采集、存储、云端上传、CAN通信和升级预留整理成一套 OKMX8MM 工程。

数据流：

```text
传感器 -> 外接采集板 -> M4采集 -> A53处理 -> SD卡 / 云平台 / CAN矿车
```

## 分工

M4 只做采集：

- 通过 UART3 + RS485 + Modbus 读取外接采集板。
- 采集模拟量、数字量和转速类输入。
- 把采集批次整理成统一数据帧。
- 不做云上传、SD存储、CAN业务和OTA。

A53 做网关：

- 接收 M4 数据。
- 写入 SD 卡文件。
- 生成 MQTT 上传队列。
- 生成 CAN 发送帧。
- 记录运行状态和心跳。
- 后续承接 OTA。

## 和 STM32 参考工程的关系

导师给的 STM32 工程是主参考，不是直接照搬。

STM32 是一个 MCU 把采集、存储、MQTT、CAN、OTA 都做了。OKMX8MM 是双核平台，所以要拆开：

- STM32 `adc` -> Demo2 `m4`
- STM32 `adc-storage` -> Demo2 `a53`
- STM32 `mqtt` -> Demo2 `a53`
- STM32 `can` -> Demo2 `a53`
- STM32 `heart-beat` -> Demo2 `a53`
- STM32 `ota` -> Demo2 `a53`

## 文件夹说明

```text
OKMX8MM-Demo2
|-- m4       M4采集工程
|-- a53      A53网关工程
|-- docs     新人说明和验收清单
|-- scripts  Demo2自检和打包脚本
|-- examples 预留样例
```

## 新手先做什么

第一步：先跑 A53 模拟流程。

```powershell
cd D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\a53
.\scripts\test-windows.ps1
```

也可以直接跑 Demo2 总自检：

```powershell
cd D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2
.\scripts\check-demo2.ps1
```

第二步：看输出文件。

```text
runtime-data\a53-storage.jsonl
runtime-data\a53-mqtt-outbox.jsonl
runtime-data\a53-can-trace.log
runtime-data\a53-status.json
runtime-data\a53-heartbeat.jsonl
```

第三步：看 M4 采集说明。

```text
D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\m4\demo1\README.md
```

第四步：看 Demo2 操作流程。

```text
D:\Codex_AI\YY_Demo\OKMX8MM\OKMX8MM-Demo2\docs\newcomer-runbook.txt
```

## 当前完成度

按“除真实上板实测以外的软件准备”计算，Demo2 目标是到 80% 左右：

- 架构已明确。
- M4采集框架已集中。
- A53存储、MQTT、CAN、状态、心跳已集中。
- M4和A53主机测试已接入总自检。
- STM32参考映射已整理。
- 新人操作步骤已整理。
- 自检和打包入口已提供。

还不能说 100%，因为真实开发板、真实采集板、真实SD卡、真实云平台和真实CAN总线还没有实测。
