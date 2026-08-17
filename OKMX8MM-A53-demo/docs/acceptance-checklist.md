# 验收清单

## Windows 模拟验收

- [ ] 执行 `.\scripts\test-windows.ps1`。
- [ ] 测试结果为 `5/5` 通过。
- [ ] 执行 `.\scripts\package-board.ps1`。
- [ ] 执行 `.\scripts\check-deliverable.ps1`。
- [ ] 交付自检提示通过。
- [ ] 执行 `.\build\okmx8mm-a53-demo.exe --cycles 3`。
- [ ] 复制 `config\a53-demo.conf.example` 为 `config\a53-demo.conf`。
- [ ] 执行 `.\build\okmx8mm-a53-demo.exe --config config\a53-demo.conf`。
- [ ] 生成 `runtime-data/a53-storage.jsonl`。
- [ ] 生成 `runtime-data/a53-storage.jsonl.cursor`。
- [ ] 生成 `runtime-data/a53-mqtt-outbox.jsonl`。
- [ ] 生成 `runtime-data/a53-can-trace.log`。
- [ ] 存储文件能看到 `ai0` 到 `ai9`。
- [ ] 存储游标能看到 `sequence`。
- [ ] 存储游标能看到 `line_checksum`。
- [ ] 执行 `.\build\okmx8mm-a53-demo.exe --check-storage runtime-data\a53-storage.jsonl`。
- [ ] 存储游标校验能通过。
- [ ] 人为追加一段不完整内容后，执行 `.\build\okmx8mm-a53-demo.exe --recover-storage runtime-data\a53-storage.jsonl`。
- [ ] 再次执行 `--check-storage` 能通过。
- [ ] CAN 记录能看到 `CAN id=0x321`。
- [ ] CAN 记录能看到 `frame=321#`。

## 开发板基础验收

- [ ] 执行 `sh scripts/check-board-env.sh`。
- [ ] 能看到 CPU 架构。
- [ ] 能看到 `runtime-data is writable`。
- [ ] 执行 `sh scripts/build-linux.sh`。
- [ ] 生成 `build-linux/okmx8mm-a53-demo`。
- [ ] 执行 `./build-linux/okmx8mm-a53-demo --cycles 3`。
- [ ] 复制 `config/a53-demo.conf.example` 为 `config/a53-demo.conf`。
- [ ] 执行 `./build-linux/okmx8mm-a53-demo --config config/a53-demo.conf`。
- [ ] 三个输出文件都有内容。
- [ ] 执行 `./build-linux/okmx8mm-a53-demo --check-storage runtime-data/a53-storage.jsonl` 能通过。
- [ ] 文件尾部不完整时，执行 `./build-linux/okmx8mm-a53-demo --recover-storage runtime-data/a53-storage.jsonl` 能恢复。

## 文本输入验收

- [ ] 执行 `./build-linux/okmx8mm-a53-demo --file examples/m4-input.csv --cycles 3`。
- [ ] 输出内容和 `examples/m4-input.csv` 对应。
- [ ] `ai0` 第一批为 `1000`。
- [ ] `ai9` 第一批为 `1009`。

## MQTT 验收

- [ ] 复制 `config/mqtt.env.example` 为 `config/mqtt.env`。
- [ ] 填写开发板现场使用的 MQTT 参数。
- [ ] 执行 `sh scripts/publish-mqtt-outbox.sh --env config/mqtt.env --dry-run`。
- [ ] 能看到 qos、topic 和 payload。
- [ ] 真实发布前确认账号不进 Git。
- [ ] 真实发布后云平台能收到数据。

## CAN 验收

- [ ] 复制 `config/can.env.example` 为 `config/can.env`。
- [ ] 确认 `CAN_IFACE=can0` 或现场实际网口名。
- [ ] 执行 `sh scripts/send-can-trace.sh --env config/can.env --dry-run`。
- [ ] 能看到 `cansend can0`。
- [ ] 真实发送前确认 CAN 线束和波特率。
- [ ] 真实发送后矿车端能收到帧。

## 真实 M4 通信验收

- [ ] 明确 M4 到 A53 使用 RPMsg、串口或其他方式。
- [ ] 替换 `src/m4_file_source.c` 的读取逻辑。
- [ ] 保持 `a53_m4_batch_t` 字段含义不变。
- [ ] A53 能收到真实 M4 批次序号。
- [ ] A53 输出的模拟量和数字量跟 M4 采集结果一致。

## 不能提前写通过的项目

- [ ] 未上开发板，不能写“开发板通过”。
- [ ] 未接云平台，不能写“云上传通过”。
- [ ] 未接 CAN 总线，不能写“CAN 实测通过”。
- [ ] 未接真实 M4，不能写“M4-A53 实测通过”。
