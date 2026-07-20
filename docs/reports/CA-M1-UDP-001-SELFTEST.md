# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-UDP-001
- 变更标题：UDP 传输 + UdpDatagram 端点记录 + UI
- 开发基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- 计划：UdpTransport、NativeSession、SessionManager UDP 绑定互斥、MainWindow UDP 页、CoreSelfTest、报告
- 实际：同上；UDP 允许空 Datagram；TCP/UDP 同端口号 claim 分协议并存
- 范围偏差：无（未做 Scheduler/Capture/Codec/Legacy）

## 静态自检

| 项 | 结果 |
|---|---|
| RX 记为 UdpDatagram（非 RawChunk/ProtocolFrame） | 是 |
| TX/RX 含 local/remote endpoint | 是 |
| 空 Datagram 可提交 | 是（仅 Udp） |
| UDP 绑定互斥（含通配同类） | SessionManager kind 区分 |
| 中文 UI | 「UDP」页：本地/远端地址端口 |

## 构建

| 配置 | 结果 |
|---|---|
| CoreSelfTest Debug\|x64 | 成功 |
| CommunicationAssistant.exe Debug\|x64 | 成功 |

## 自动化测试

| 项 | 结果 | 等级 |
|---|---|---|
| CoreSelfTest 0 failed | 含 UDP 双端互通、空包、绑定冲突、TCP 同端口并存、无远端发送失败；串口/TCP 回归 | E4 |

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4