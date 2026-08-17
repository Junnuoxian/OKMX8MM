# OKMX8MM Demo2 使用说明

这个文件夹给接手项目的人用。先不要急着看所有代码，按下面顺序走一遍，能少踩很多坑。

## 先看这张流程

```text
传感器
  |
外接采集板
  |
M4：只采集
  |
A53：保存数据、准备上传、准备发CAN、记录状态
```

这里先记住一句话：M4 不管云平台和 SD 卡，A53 不直接采模拟量。

## 文件夹怎么分

```text
OKMX8MM-Demo2
|-- a53      先跑这里，验证存储、MQTT、CAN和心跳
|-- m4       再看这里，验证采集和RS485
|-- docs     操作说明和检查清单
|-- scripts  总自检和打包脚本
|-- examples 预留样例
```

## 第一次打开先做什么

先跑总自检：

```powershell
cd OKMX8MM-Demo2
.\scripts\check-demo2.ps1
```

正常情况会分别看到两组测试通过，最后显示：

```text
100% tests passed, 0 tests failed
Demo2 self-check passed.
```

如果这一步没过，不要接开发板，先把电脑端问题处理掉。

## 单独跑 A53

```powershell
cd OKMX8MM-Demo2\a53
.\scripts\test-windows.ps1
```

运行后重点看这些文件：

```text
runtime-data\a53-storage.jsonl
runtime-data\a53-mqtt-outbox.jsonl
runtime-data\a53-can-trace.log
runtime-data\a53-status.json
runtime-data\a53-heartbeat.jsonl
```

怎么看：

- `a53-storage.jsonl` 有内容，说明数据能保存。
- `a53-mqtt-outbox.jsonl` 有内容，说明上传数据已经准备好。
- `a53-can-trace.log` 有内容，说明 CAN 数据已经准备好。
- `a53-status.json` 里 `ok` 为 `true`，说明本次运行正常。
- `a53-heartbeat.jsonl` 有内容，说明程序在持续处理数据。

## 单独看 M4

```powershell
cd OKMX8MM-Demo2\m4
.\scripts\build-windows.ps1 -Test
```

M4 这边重点看两个文件：

```text
demo1\README.md
docs\board-test-guide.txt
```

M4 要做的事很窄：通过 UART3、RS485、Modbus 读外接采集板。

## 上开发板时怎么排顺序

不要一开始就把采集板、云平台、CAN 全部接上。建议这样做：

1. 先让 A53 程序在开发板 Linux 里跑起来。
2. 再确认 SD 卡能写文件。
3. 再用 CAN 工具看能不能发出帧。
4. 再配置 MQTT，看云平台能不能收到数据。
5. 再启动 M4，看调试串口有没有日志。
6. 再接外接采集板，看 M4 能不能读到数据。
7. 最后再把 M4 数据送到 A53，看 SD、云平台、CAN 三路是否都有结果。

如果出问题，就按这个顺序倒查：先查 M4 有没有采到，再查 A53 有没有收到，再查三个输出文件有没有新增。
