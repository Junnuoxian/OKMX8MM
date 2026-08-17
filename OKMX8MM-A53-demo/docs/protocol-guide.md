# 数据与协议说明

## 数据来源

A53 当前支持两种数据来源：

- 内置模拟数据：程序自动生成。
- 文本输入数据：每行代表一批 M4 数据。

文本输入格式：

```text
sequence,ai0,ai1,ai2,ai3,ai4,ai5,ai6,ai7,ai8,ai9,di_bits,speed_pulse_delta,speed_period_us
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `sequence` | 批次序号 |
| `ai0` 到 `ai9` | 10 路模拟量 |
| `di_bits` | 数字量位图 |
| `speed_pulse_delta` | 转速脉冲增量 |
| `speed_period_us` | 转速周期，单位微秒 |

## 存储 JSONL

默认文件：

```text
runtime-data/a53-storage.jsonl
```

一行示例：

```json
{"sequence":0,"source":"m4-replay","sample_rate_hz":2000,"samples":10,"di_bits":1,"speed_pulse_delta":11,"speed_period_us":50000,"first_sample":{"ai0":1000,"ai1":1001,"ai2":1002,"ai3":1003,"ai4":1004,"ai5":1005,"ai6":1006,"ai7":1007,"ai8":1008,"ai9":1009}}
```

## 存储游标

每次写入存储文件后，会覆盖写同名 `.cursor` 文件。

示例：

```text
file=a53-storage.jsonl
sequence=0
byte_offset=268
line_bytes=268
line_checksum=1234ABCD
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `file` | 对应的存储文件名 |
| `sequence` | 最新写入批次 |
| `byte_offset` | 最新写入后的文件偏移 |
| `line_bytes` | 最新一行字节数 |
| `line_checksum` | 最新一行的 FNV-1a 校验 |

校验命令：

```sh
./build-linux/okmx8mm-a53-demo --check-storage runtime-data/a53-storage.jsonl
```

## MQTT Outbox

默认文件：

```text
runtime-data/a53-mqtt-outbox.jsonl
```

一行示例：

```json
{"topic":"mine-truck/demo1","payload":{"sequence":0,"ai0":1000,"di_bits":1,"speed_pulse_delta":11}}
```

发送方式：

```sh
MQTT_HOST=test sh scripts/publish-mqtt-outbox.sh --dry-run
```

## CAN Trace

默认文件：

```text
runtime-data/a53-can-trace.log
```

一行示例：

```text
CAN id=0x321 seq=0 ai0=1000 di=0x01 speed_pulse=11 speed_period_us=50000 frame=321#0000E803010B0000
```

当前 CAN dry-run 会转换成：

```text
cansend can0 321#0000E803010B0000
```

payload 说明：

| 字节 | 含义 |
| --- | --- |
| 0-1 | sequence，小端 |
| 2-3 | ai0，小端 |
| 4 | di_bits |
| 5-6 | speed_pulse_delta，小端 |
| 7 | 保留 |

说明：`frame=` 字段由 C 程序直接生成，`send-can-trace.sh` 会优先使用它；旧格式没有 `frame=` 时，脚本仍会按字段临时转换。

## 后续要统一的地方

- 存储 JSON 后续参考 `modules/storage` 增加恢复和空间检查。
- MQTT 后续参考 `modules/mqtt` 统一编码。
- CAN 后续参考 `modules/can` 统一字节打包。
