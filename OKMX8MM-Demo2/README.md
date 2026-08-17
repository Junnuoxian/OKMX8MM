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
|-- docs     新人说明和操作清单
|-- scripts  Demo2自检和打包脚本
|-- examples 预留样例
```

## 新手先做什么

第一步：先跑 A53 模拟流程。

```powershell
cd OKMX8MM-Demo2\a53
.\scripts\test-windows.ps1
```

也可以直接跑 Demo2 总自检：

```powershell
cd OKMX8MM-Demo2
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
OKMX8MM-Demo2\m4\demo1\README.md
```

第四步：看 Demo2 操作流程。

```text
OKMX8MM-Demo2\docs\newcomer-runbook.txt
```

## 上板实测顺序

按下面顺序做，不要一开始就全部接上：

1. 先在电脑上运行 Demo2 总自检。
2. 把 A53 程序放到 OKMX8MM 开发板 Linux 里运行。
3. 确认 SD 卡可以写入采集文件。
4. 用 CAN 工具确认 CAN 帧可以发出。
5. 配置 MQTT，确认云平台能收到数据。
6. 启动 M4，确认调试串口有日志。
7. 接外接采集板，确认 M4 能读取 RS485 数据。
8. 把 M4 采集数据送到 A53，形成完整链路。

详细步骤看：

```text
OKMX8MM-Demo2\docs\newcomer-runbook.txt
```
