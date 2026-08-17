# 参考工程移植映射

## 参考来源

参考工程：

```text
D:\Codex_AI\YY_Demo\项目一\OKMX8MM-Sensor-Gateway
```

当前 A53 示例：

```text
OKMX8MM-A53-demo
```

## 移植原则

- 不整包复制，先移植最关键模块。
- 当前 A53 示例保持简单可运行。
- 每次只替换一个模块，替换后必须跑测试。
- 没有开发板实测前，只写“模拟通过”，不写“实测通过”。

## 模块对应关系

| 功能 | 参考工程模块 | 当前 A53 文件 | 后续动作 |
| --- | --- | --- | --- |
| 存储 | `modules/storage` | `src/storage_writer.c` | 已有最小游标和尾部恢复；后续移植空间检查 |
| MQTT | `modules/mqtt` | `src/mqtt_outbox.c`、`scripts/publish-mqtt-outbox.sh` | 后续移植 payload 编码、重发队列 |
| CAN | `modules/can` | `src/can_trace.c`、`scripts/send-can-trace.sh` | 已在 C 侧生成 frame；后续移植 SocketCAN |
| 文件工具 | `modules/util/fs_io.c`、`modules/util/fs_utils.c` | `src/file_utils.c` | 后续增强目录创建、剩余空间检查 |
| JSON | `modules/util/sample_json.c` | `src/storage_writer.c`、`src/mqtt_outbox.c` | 后续统一 JSON 生成 |
| 配置 | `src/app_config.c`、`config/demo.ini` | `src/cli.c`、`config/*.env.example` | 后续增加配置文件读取 |
| 主流程 | `src/app_runtime.c` | `src/pipeline.c` | 后续增加错误计数、不中断输出 |
| 测试 | `tests/*` | `tests/*` | 每移植一个模块，补对应测试 |

## 推荐移植顺序

1. 存储增强：后续移植参考工程的空间检查和更完整游标格式。
2. CAN 编码：移植参考工程的 `can_codec`，让 CAN payload 不只靠脚本拼接。
3. MQTT 编码：移植参考工程的 `mqtt_codec`，统一 topic 和 payload 格式。
4. 配置文件：移植 `app_config` 的配置校验，不再只靠命令行参数。
5. 主流程错误统计：移植 `app_runtime` 的计数和错误隔离。

## 当前不要做的事

- 不提交真实 MQTT 账号。
- 不提交真实云平台密钥。
- 不把运行数据放进 Git。
- 不把 Windows 构建产物放进 Git。
- 不在未实测时写“开发板已通过”。
