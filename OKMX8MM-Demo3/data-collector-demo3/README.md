# data-collector-demo3

基于原始 data-collector 新建的实际开发工程。

- RPMsg：接收 M4 转发的采样帧并校验。
- Modbus/RS485：兼容串口采集模式。
- 文件存储：保存采集记录。
- MQTT：上传云端。
- CAN：提供 CAN 输出适配。
- 运行状态：输出采集、存储、CAN、MQTT和升级暂存计数。
- OTA：暂存升级包并生成重启标记，不自动刷写。
