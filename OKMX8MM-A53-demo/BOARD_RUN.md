# A53 开发板运行说明

## 当前目的

先在 A53 Linux 上确认程序能跑起来，再接真实 M4 通信。

## 步骤

1. 复制 `OKMX8MM-A53-demo` 到开发板。
2. 进入 `OKMX8MM-A53-demo`。
3. 先检查环境：

```sh
sh scripts/check-board-env.sh
```

4. 编译：

```sh
sh scripts/build-linux.sh
```

5. 运行内置模拟数据：

```sh
./build-linux/okmx8mm-a53-demo --cycles 3
```

6. 运行文本输入数据：

```sh
./build-linux/okmx8mm-a53-demo --file examples/m4-input.csv --cycles 3
```

7. 如果要写到 SD 卡挂载目录，运行时指定输出文件：

```sh
./build-linux/okmx8mm-a53-demo --cycles 3 \
  --storage /mnt/sdcard/samples.jsonl \
  --mqtt-outbox /mnt/sdcard/mqtt-outbox.jsonl \
  --can-trace /mnt/sdcard/can-trace.log
```

8. 查看输出：

```sh
cat runtime-data/a53-storage.jsonl
cat runtime-data/a53-mqtt-outbox.jsonl
cat runtime-data/a53-can-trace.log
```

## 上传 MQTT

先只看待发送内容，不发到云：

```sh
MQTT_HOST=test sh scripts/publish-mqtt-outbox.sh --dry-run
```

确认格式后，设置真实 MQTT 参数再发布：

```sh
cp config/mqtt.env.example config/mqtt.env
vi config/mqtt.env
sh scripts/publish-mqtt-outbox.sh --env config/mqtt.env
```

发布成功后，如果确认可以清空待发送文件：

```sh
sh scripts/publish-mqtt-outbox.sh --clear-on-success
```

注意：不要把账号和密码写进源码、说明或 Git。

## 发送 CAN

先只看要发送的 CAN 帧，不发到总线：

```sh
sh scripts/send-can-trace.sh --dry-run
```

确认 `can0` 已正常后，再发送：

```sh
cp config/can.env.example config/can.env
vi config/can.env
sh scripts/send-can-trace.sh --env config/can.env
```

发送成功后，如果确认可以清空 CAN 记录：

```sh
sh scripts/send-can-trace.sh --clear-on-success
```

## 判断是否正常

- 程序提示写入 3 批数据。
- 三个输出文件都有内容。
- `a53-storage.jsonl` 能看到 `ai0` 到 `ai9`。
- `a53-can-trace.log` 能看到 `CAN id=0x321`。
- CAN dry-run 能看到 `cansend can0`。

## 接真实 M4 的位置

优先改：

```text
src/m4_file_source.c
```

把“读文本文件”换成“读 RPMsg 或串口”。其他三个输出模块先不动。

## 当前还没做

- 还没接真实 SD 卡。
- 还没和真实 M4 通信联调。
