# OKMX8MM A53 示例

## 当前作用

这个示例先在 A53 侧跑通完整流程：

`模拟 M4 数据 -> A53 接收 -> 本地存储 -> MQTT 待发送记录 -> CAN 发送记录`

当前版本用于 Windows 模拟验证：

- M4 数据使用程序内置的模拟数据。
- 存储使用本地 JSONL 文件模拟 SD 卡。
- MQTT 使用本地 JSONL 文件模拟待发送队列。
- CAN 使用文本日志模拟 CAN 帧。
- 没有连接真实开发板、真实 SD 卡、云平台或 CAN 收发器。

完整需求看：

```text
REQUIREMENTS.md
```

移植和协议说明：

```text
docs/reference-migration-map.md
docs/protocol-guide.md
docs/acceptance-checklist.md
```

## 新手操作

### 1. 编译

在 PowerShell 中执行：

```powershell
.\scripts\build-windows.ps1
```

### 2. 运行

默认使用内置模拟数据。参数 `3` 表示处理 3 批数据：

```powershell
.\build\okmx8mm-a53-demo.exe 3
```

也可以读文本输入，方便以后替换真实 M4 通信：

```powershell
.\build\okmx8mm-a53-demo.exe --file .\examples\m4-input.csv --cycles 3
```

也可以指定输出文件：

```powershell
.\build\okmx8mm-a53-demo.exe --cycles 3 `
  --storage runtime-data\sd-samples.jsonl `
  --mqtt-outbox runtime-data\mqtt.jsonl `
  --can-trace runtime-data\can.log `
  --topic truck/001 `
  --can-id 0x456
```

### 3. 查看结果

运行后查看：

```text
runtime-data\a53-storage.jsonl
runtime-data\a53-mqtt-outbox.jsonl
runtime-data\a53-can-trace.log
```

每处理一批 M4 数据，三个文件各增加一行。

常用参数：

- `--cycles N`：处理 N 批数据。
- `--file input.csv`：读取文本输入，不用内置模拟。
- `--storage file`：采集数据写入文件。
- `--mqtt-outbox file`：MQTT 待发内容写入文件。
- `--can-trace file`：CAN 内容写入文件。
- `--topic name`：设置 MQTT topic。
- `--can-id 0x321`：设置 CAN ID。

## 输出说明

### 存储文件

`a53-storage.jsonl` 保存采集批次，包含：

- 批次序号
- 采样率
- 模拟量前 10 路
- 开关量
- 转速脉冲数
- 转速周期

同时会生成：

```text
a53-storage.jsonl.cursor
```

它记录最新写入的批次序号、字节偏移、行长度和行校验，用于后续断电恢复扩展。

### MQTT 文件

`a53-mqtt-outbox.jsonl` 保存待发送内容。以后接入 MQTT 时，将写文件的函数替换为 MQTT 发布函数。

### CAN 文件

`a53-can-trace.log` 保存待发送的 CAN 内容。以后接入 Linux SocketCAN 或开发板 CAN 驱动时，将写文件的函数替换为 CAN 发送函数。

## 换成真实 M4 数据

现在已有两种输入：

- `A53_SOURCE_REPLAY`：内置模拟数据。
- `A53_SOURCE_FILE`：读取文本输入，每行代表一批 M4 数据。

文本输入格式：

```text
sequence,ai0,ai1,ai2,ai3,ai4,ai5,ai6,ai7,ai8,ai9,di_bits,speed_pulse_delta,speed_period_us
```

后续接真实 M4 通信时，优先替换：

```text
src\m4_file_source.c
```

保留 `a53_m4_batch_t` 字段含义不变，把文件读取改为真实 RPMsg、串口或其他核间通信读取。

存储、MQTT、CAN 三条处理流程可以先保持不变。

## 运行测试

```powershell
.\scripts\test-windows.ps1
```

## 交付自检

```powershell
.\scripts\package-board.ps1
.\scripts\check-deliverable.ps1
```

## 开发板运行

看：

```text
BOARD_RUN.md
```

开发板上先执行：

```sh
sh scripts/check-board-env.sh
```

MQTT 上传看 `BOARD_RUN.md` 的“上传 MQTT”部分。

MQTT 配置样例：

```text
config/mqtt.env.example
```

CAN 发送看 `BOARD_RUN.md` 的“发送 CAN”部分。

## 生成开发板压缩包

```powershell
.\scripts\package-board.ps1
```

生成后看 `packages` 目录。

## 下一步

1. 在 OKMX8MM 开发板上确认 A53 Linux 的编译环境。
2. 用真实 RPMsg 或串口替换模拟 M4 数据。
3. 把存储文件替换为 SD 卡文件。
4. 把 MQTT 待发送文件替换为真实 MQTT 发布。
5. 把 CAN 日志替换为真实 CAN 发送。
6. 接入真实硬件后重新测试，确认后再用于矿车。
