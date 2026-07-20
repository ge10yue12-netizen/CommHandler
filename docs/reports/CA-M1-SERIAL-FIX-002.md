# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-SERIAL-FIX-002
- 变更标题：串口运行时故障与 UI 错误面修复
- 基线版本：V1.2.4（docs/BASELINE.md）
- 代码规范版本：V1.0（docs/CODE_STYLE.md）
- 审计规范版本：V1.2（docs/AUDIT_SPEC.md）
- 所属里程碑：M1
- 审计日期：2026-07-20
- 前置变更：CA-M1-SERIAL-FIX-001
- 审计范围：写超时、运行时串口错误拆卸、UI 错误去重与 Opening 门控、枚举名日志

## 2. 变更目标

- 原问题：写超时可能静默成功；运行时串口错误仍保持 Connected/占 claim；UI 三重错误日志；Opening 可重复点开；日志 kind/status 为数字
- 预期结果：按对齐单 R1–R6 修复并复跑 CoreSelfTest（含环回）
- 明确不做：m_ 重命名、拆分测试文件、TCP/UDP、clang 工具文件

## 3. 工作区状态

- Git 工程：是
- 方法：MSBuild Debug|x64 + CoreSelfTest（Eltima COM4/COM5）

## 4. 实际修改

| 文件 | 操作 | 原因 |
|---|---|---|
| transport/SerialTransport.cpp | 修改 | 写超时一律失败；运行时错误先 emit 再 Faulted/close |
| session/NativeSession.h/.cpp | 修改 | tearDownAfterFault；Connected 下意外 Closed 拆卸；打开失败单 ErrorEvent |
| src/MainWindow.h/.cpp | 修改 | Opening 禁开；去掉 errorOccurred 重复日志；枚举名显示 |
| core/Types.h | 修改 | recordKindName / recordStatusName |
| tests/CoreSelfTest.cpp | 修改 | 枚举名、send 未连接、单 ErrorEvent |
| docs/reports/CA-M1-SERIAL-FIX-002.md | 新增 | 本报告 |
| docs/reports/CA-M1-SERIAL-FIX-002-SELFTEST.md | 新增 | 自检自测报告 |

## 5. 接口与格式影响

- 无 Capture/协议格式变更
- 行为：运行时串口错误会使会话进入 Closed 并释放 claim（正确性修复）
- UI：打开失败仅通过 ErrorEvent 记录展示

## 6. 需求—代码—测试追踪

| 需求 | 实现 | 测试 | 结果 | 等级 |
|---|---|---|---|---|
| R1 写超时失败 | SerialTransport::send | 代码审查 + 构建 | 通过 | E0/E1 |
| R2 运行时拆卸 | tearDownAfterFault | 打开失败单事件；环回仍绿 | 部分 [VERIFIED] | E2/E4 |
| R3 UI 单面错误 | MainWindow | CoreSelfTest ErrorEvent==1 | 通过 | E2 |
| R4 Opening 禁开 | syncUi canOpen | 代码审查 | 通过 | E0 |
| R5 枚举名 | Types + MainWindow | testEnumNames | 通过 | E2 |
| R6 构建+自测 | MSBuild + CoreSelfTest | 0 failed | 通过 | E1/E2/E4 |

## 7. 执行的验证

| 项 | 结果 |
|---|---|
| MSBuild Debug\|x64 | 成功 |
| CoreSelfTest | `=== done: 0 failed ===` EXIT=0 |
| COM4↔COM5 环回 | PASS |

## 8. 未执行验证

| 项 | 原因 | 风险 | 后续 |
|---|---|---|---|
| 真机拔线 ResourceError | 无自动化 | 中 | 用户点测 |
| 写超时真实注入 | 无 mock | 低 | 代码路径已强制失败 |

## 9. 发现问题

| 级别 | 问题 | 处理 |
|---|---|---|
| P1 | Opening 期间 transport Closed 误触发 tearDown → 双 ErrorEvent | onTransportStateChanged 仅 Connected |
| P1 | 写超时 NoError 仍 success | 一律 serial_write_timeout |
| P1 | 运行时错误不释放 claim | tearDownAfterFault |
| P2 | UI 数字枚举 / 三重日志 | 枚举名 + 去掉 errorOccurred 日志 |

## 10. 基线符合性

符合 M1 Serial；无 Codec；无永久 wait(-1)；Queued→Submitted 不变。

## 11. Diff 审计

范围限于 CommunicationAssistant 与本报告/自检报告。

## 12. 审计结论

- 结论：**PASS**（拔线 E5 未测已在 §8 登记，不阻断本编号以 E2/E4 验收的修复项）
- 遗留风险：真机拔线需人工确认 tearDown
- 允许下一步：CA-M1-TCP-001 或命名迁移 / clang 工具任务（须新确认修改）