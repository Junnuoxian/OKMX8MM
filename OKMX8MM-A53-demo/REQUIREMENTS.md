# A53 示例需求说明

## 总目标

矿车采集系统分成两部分：

- M 核：只负责采集。
- A53：负责接收数据、保存到 SD 卡、上传 MQTT、发送 CAN。

当前 A53 示例先把流程跑通，后续再逐步接真实 M4、真实 SD 卡、真实 MQTT、真实 CAN。

## 当前已实现

- 支持内置模拟 M4 数据。
- 支持文本输入模拟 M4 数据。
- 支持保存采集数据到 JSONL 文件。
- 支持生成 MQTT 待发送文件。
- 支持生成 CAN 记录文件。
- 支持自定义输出文件、MQTT topic、CAN ID。
- 支持开发板环境检查脚本。
- 支持开发板交付压缩包。
- 支持 MQTT 待发送文件发布脚本。

## 当前未完成

- 未接真实 M4 到 A53 通信。
- 未实测真实 SD 卡长期写入。
- 未实测真实 MQTT 云平台。
- 未实测真实 CAN 总线。
- 未做断电恢复和异常重发。

## M 核要求

- M 核只做采集，不做云上传、不做 SD 存储、不做 CAN 转发。
- M 核采集模拟量、数字量和转速类数据。
- M 核后续把采集结果发送给 A53。
- M 核输出字段要和 A53 的 `a53_m4_batch_t` 含义保持一致。

## A53 输入要求

A53 需要支持三种阶段：

1. 内置模拟数据：用于 Windows 和无硬件验证。
2. 文本输入数据：用于模拟真实 M4 输出。
3. 真实 M4 通信：后续接 RPMsg、串口或其他核间通信。

当前代码已经完成前两种，第三种后续在 `src/m4_file_source.c` 基础上替换。

## A53 存储要求

- 每批数据保存一行 JSONL。
- 默认写入 `runtime-data/a53-storage.jsonl`。
- 可通过 `--storage file` 指定输出文件。
- 开发板联调时可把输出文件放到 SD 卡挂载目录。

## MQTT 要求

- A53 主程序先生成 MQTT 待发送文件。
- 默认文件为 `runtime-data/a53-mqtt-outbox.jsonl`。
- 可通过 `--mqtt-outbox file` 指定输出文件。
- 可通过 `--topic name` 指定 MQTT topic。
- 发布脚本使用 `MQTT_HOST`、`MQTT_PORT`、`MQTT_USER`、`MQTT_PASSWORD`。
- 可复制 `config/mqtt.env.example` 为 `config/mqtt.env` 后填写 MQTT 参数。
- 账号和密码不能写进源码、说明或 Git。

## CAN 要求

- 当前先生成 CAN 记录文件。
- 默认文件为 `runtime-data/a53-can-trace.log`。
- 可通过 `--can-trace file` 指定输出文件。
- 可通过 `--can-id 0x321` 指定 CAN ID。
- 可复制 `config/can.env.example` 为 `config/can.env` 后填写 CAN 网口名。
- 当前用 `scripts/send-can-trace.sh` 把 CAN 记录转换为 `cansend` 命令。
- 后续稳定后，可把写日志替换成直接调用 SocketCAN。

## 开发板验收

第一阶段验收：

- `sh scripts/check-board-env.sh` 能执行。
- `sh scripts/build-linux.sh` 能编译。
- `./build-linux/okmx8mm-a53-demo --cycles 3` 能运行。
- 三个输出文件都有内容。
- 存储文件能看到 `ai0` 到 `ai9`。
- CAN 记录能看到 `CAN id=0x321`。

第二阶段验收：

- 使用 `--file examples/m4-input.csv --cycles 3` 能运行。
- 输出内容跟输入文件的 `ai0` 到 `ai9` 对应。
- MQTT dry-run 能打印 topic 和 payload。
- CAN dry-run 能打印 `cansend can0`。

第三阶段验收：

- A53 能接收真实 M4 数据。
- SD 卡能持续写入采集文件。
- MQTT 能上传到真实云平台。
- CAN 能发到矿车总线。

## 下一步顺序

1. 在开发板上运行环境检查。
2. 在开发板上编译 A53 示例。
3. 在开发板上运行内置模拟数据。
4. 在开发板上运行文本输入数据。
5. 用真实 M4 通信替换文本输入。
6. 把存储文件切到 SD 卡。
7. 配置 MQTT 并执行发布脚本。
8. 把 CAN 记录替换成真实 CAN 发送。

## 参考移植

详细映射看：

```text
docs/reference-migration-map.md
docs/protocol-guide.md
docs/acceptance-checklist.md
```
