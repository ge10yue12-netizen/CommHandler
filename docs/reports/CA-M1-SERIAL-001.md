# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-SERIAL-001
- 变更标题：SerialTransport + NativeSession 串口收发
- 基线版本：V1.2.3（docs/BASELINE.md）
- 审计规范版本：V1.1（docs/AUDIT_SPEC.md）
- 所属里程碑：M1
- 审计日期：2026-07-20
- 审计范围：串口 Transport、NativeSession、SessionManager 串口互斥、UI 打开/关闭/HEX/文本发送、RawChunk 显示

## 2. 变更目标

- 原问题：CORE-001 仅骨架，无真实串口 I/O
- 预期结果：可配置串口、打开/关闭、发送、接收记为 RawChunk；Queued→Submitted；不链 CommHandler
- 明确不做：TCP/UDP、Scheduler、Capture 落盘、Legacy/Simulator、Codec、ProtocolFrame

## 3. 工作区状态

- Git 工程：是；本变更仅触及 CommunicationAssistant/ 与本报告

## 4. 实际修改

| 文件 | 操作 |
|---|---|
| core/ITransport.h | 新增 |
| transport/SerialTransport.h/.cpp | 新增 |
| session/SessionManager.h | 新增 |
| session/NativeSession.h/.cpp | 新增 |
| src/MainWindow.h/.cpp | 修改为串口面板 |
| CommunicationAssistant.vcxproj | 加入源文件与 include |
| docs/reports/CA-M1-SERIAL-001.md | 新增 |

## 5. 接口与格式影响

- 新增 ITransport / NativeSession 行为；无 Capture 文件格式变更

## 6. 需求—代码—测试追踪

| 需求 | 实现 | 测试 | 结果 | 等级 |
|---|---|---|---|---|
| R1 Serial open/close/send | SerialTransport + NativeSession | MSBuild | 通过 | E1 |
| R2 RawChunk 非 Frame | onBytesReceived | 代码审查 | 通过 | E0 |
| R3 requestId + Queued/Submitted | NativeSession::send | 代码审查 | 通过 | E0 |
| R4 串口内部互斥 | SessionManager | 代码审查 | 通过 | E0 |
| R5 无永久 waitForBytesWritten(-1) | SerialTransport::send | 代码审查 | 通过 | E0 |
| R6 真实串口互通 | — | 未运行（无虚拟串口/设备） | 未运行 | — |

## 7. 执行的验证

| 验证项 | 结果 |
|---|---|
| MSBuild Debug\|x64 | 成功 |

## 8. 未执行验证

| 项 | 原因 | 风险 | 后续 |
|---|---|---|---|
| 真实/虚拟串口双端互发（E4） | 当前环境未跑设备测试 | 打开失败/驱动差异需实机验证 | 用户用虚拟串口点测 |
| 端口占用冲突实机 | 未测 | 中 | 手动双开验证 resource_busy |

## 9. 发现问题

无 P0/P1。SessionManager 初带 Q_OBJECT 导致链接失败，已改为无 moc 的 QObject 子类。

## 10. 基线符合性

符合 M1 Serial 切片；无 Legacy UI；无 Codec；未链 DLL。

## 11. Diff 审计

范围限于 CommunicationAssistant 与报告。

## 12. 审计结论

- 结论：**CONDITIONAL PASS**
- 依据：E1 编译通过；关键串口路径代码符合基线；**缺少 E4 真实 I/O 证据**，不得标为串口功能完全验收通过
- 遗留风险：需虚拟串口或真机验证打开/收发
- 允许的下一步：用户点测串口；或开 CA-M1-TCP-001 / UDP-001（须新确认修改）

---

> 后续补强：CA-M1-SERIAL-FIX-001（2026-07-20）已修复缺陷并用 CoreSelfTest + Eltima COM4/COM5 环回取得 E4 证据，结论见该报告。
