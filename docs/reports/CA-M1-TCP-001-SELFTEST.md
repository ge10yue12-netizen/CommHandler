# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-TCP-001
- 变更标题：TCP Client 传输 + 现代化中文 UI
- 开发基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- 计划：TcpClientTransport、NativeSession 双传输、MainWindow UI、CoreSelfTest、报告
- 实际：同上；参考串口助手布局做左侧配置/右侧日志 QSS
- 范围偏差：无 TCP Server/UDP/Scheduler/Capture（对齐单明确不做）

## 静态自检

| 项 | 结果 |
|---|---|
| RawChunk 非 ProtocolFrame | 是 |
| waitForConnected/BytesWritten 有界 | 是（5s/1s） |
| 中文 UI/提示 | 是 |
| 打开失败单 ErrorEvent | CoreSelfTest 覆盖 |

## 构建

| 配置 | 结果 |
|---|---|
| CoreSelfTest Debug\|x64 | 成功 |
| CommunicationAssistant.exe | 曾被进程占用导致 LNK1104；结束后 Link 成功 |

## 自动化测试

| 测试 | 结果 | 等级 |
|---|---|---|
| CoreSelfTest 全量 | 0 failed | E2 |
| 串口环回 COM4↔COM5 | PASS | E4 |
| TCP loopback 回显 | PASS | E4 |
| TCP 连接失败单 ErrorEvent | PASS | E2 |

## 未执行

| 项 | 原因 |
|---|---|
| UI 人工观感验收 | 需用户打开 exe 目视 |
| 跨主机 TCP | 本机 loopback 已测 |

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4