# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-TCP-001
- 变更标题：TCP Client + 现代化中文 UI
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1
- 审计日期：2026-07-20

## 2. 变更目标

- 原问题：仅串口；UI 简陋
- 预期：TcpClientTransport；NativeSession 支持 Serial/TcpClient；参考风格左栏右日志 UI；中文交互
- 明确不做：TCP Server、UDP、Scheduler、Capture、Codec、Legacy

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| transport/TcpClientTransport.* | 新增 |
| session/NativeSession.* | 双传输调度 |
| session/SessionManager.h | 注释：TcpClient 不互斥远端 |
| src/MainWindow.* | 重做 UI + QSS |
| * .vcxproj | 纳入 TcpClient + network |
| tests/CoreSelfTest.cpp | TCP loopback / 连接失败 |
| docs/reports/CA-M1-TCP-001*.md | 本报告与自检 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| T1 TcpClient configure/open/send | PASS | E2/E4 |
| T2 RawChunk | PASS | E0/E2 |
| T3 Queued→Submitted | PASS | E2 |
| T4 本机 TCP 互通 | PASS | E4 |
| T5 串口回归 | PASS | E4 |
| T6 中文美观 UI | 已实现，待用户目视 | E0 |

## 5. 验证

- CoreSelfTest：0 failed（含 TCP 回显与失败路径）
- CommunicationAssistant：编译链接成功（曾短暂文件锁）

## 6. 未执行

- 用户 UI 观感确认
- 跨机器 TCP

## 7. 审计结论

- 结论：**PASS**（功能与自测）；UI 美观属体验项，请用户打开 `x64\Debug\CommunicationAssistant.exe` 目视确认
- 下一步：`CA-M1-TCP-SERVER-001` 或 `CA-M1-UDP-001`