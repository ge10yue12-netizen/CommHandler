# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-UDP-001
- 变更标题：UDP Transport + UdpDatagram + UI
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1
- 审计日期：2026-07-20

## 2. 变更目标

- 新增 UdpTransport（QUdpSocket）；NativeSession 支持 Udp；接收/发送记为 UdpDatagram 并带端点；SessionManager 本地绑定互斥（与 TCP Server 分协议）；UI「UDP」页；CoreSelfTest 覆盖互通/空包/冲突
- 明确不做：SendScheduler、Capture、Codec、Legacy、自动重连；不改 CommHandler

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| transport/UdpTransport.h/.cpp | 新增 |
| session/NativeSession.* | 支持 Udp + onDatagramReceived |
| session/SessionManager.h | TCP Server / UDP 分 kind 的监听表 |
| src/MainWindow.* | UDP 参数页与计数 |
| *.vcxproj / CoreSelfTest | 纳入 UdpTransport |
| docs/reports/CA-M1-UDP-001*.md | 本报告 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| U1 本机双端 UDP 互通 | PASS | E4 |
| U2 空 Datagram | PASS | E4 |
| U3 绑定失败/占用 | PASS | E4 |
| U4 UdpDatagram + endpoints | PASS | E2/E4 |
| U5 TCP 与 UDP 同端口号可并存（claim） | PASS | E4 |
| U6 串口/TCP 回归 | PASS | E4 |

## 5. 审计结论

- 结论：**PASS**
- 下一步：`CA-M1-SCHED-001`（SendScheduler）或 CaptureManager