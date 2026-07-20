# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-TCP-SERVER-001
- 变更标题：TCP Server + Channel 隔离
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1
- 审计日期：2026-07-20

## 2. 变更目标

- 新增 TcpServerTransport；每客户端 Channel；定向/广播/断开；监听口互斥；UI 服务端与客户端列表
- 明确不做：UDP、Scheduler、Capture、Codec、Legacy

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| transport/TcpServerTransport.* | 新增 |
| session/NativeSession.* | 支持 TcpServer + channelsChanged |
| session/SessionManager.h | 监听地址/端口互斥（含通配） |
| src/MainWindow.* | TCP 服务端页 + 客户端下拉/断开 |
| *.vcxproj / CoreSelfTest | 纳入与测试 |
| docs/reports/CA-M1-TCP-SERVER-001*.md | 本报告 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| S1 监听/占用提示 | PASS | E2/E4 |
| S2 ≥2 客户端隔离 | PASS | E4 |
| S3 定向与广播 | PASS | E4 |
| S4 RawChunk+channelId | PASS | E0/E2 |
| S5 串口/TCP Client 回归 | PASS | E4 |

## 5. 审计结论

- 结论：**PASS**
- 下一步：`CA-M1-UDP-001` 或 `CA-M1-SCHED-001` / Capture