# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-SERIAL-FIX-002
- 变更标题：串口运行时故障与 UI 错误面修复
- 开发基线版本：V1.2.4
- 本代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- 计划修改文件：SerialTransport、NativeSession、MainWindow、Types、CoreSelfTest、报告
- 实际修改文件：同上（含 Types.h 枚举名辅助）
- 范围偏差：无
- 已有修改处理：在 FIX-001 之上增量修复

## 静态自检

| 检查项 | 命令/方式 | 结果 | 证据 |
|---|---|---|---|
| Diff 范围 | 计划对照 | 通过 | 仅 CommunicationAssistant + docs/reports |
| 打开失败双 ErrorEvent | CoreSelfTest | 已修并通过 | Opening 时忽略 transport Closed |
| 写超时静默成功 | 代码阅读 + 逻辑 | 已修 | wait 失败一律 fail |

## 构建

| 配置 | 命令 | 结果 | 日志 |
|---|---|---|---|
| Debug\|x64 | MSBuild CommunicationAssistant.sln | 成功 | CommunicationAssistant.exe + CoreSelfTest.exe |

## 自动化测试

| 测试 | 命令 | 结果 | 证据等级 |
|---|---|---|---|
| CoreSelfTest 全量 | x64\Debug\CoreSelfTest.exe | 0 failed | E2 |
| 虚拟串口环回 | testVirtualLoopback COM4→COM5 | PASS | E4 |
| 单次 ErrorEvent | open fail | PASS | E2 |

## 本机I/O与设备测试

| 场景 | 环境 | 结果 | 证据等级 |
|---|---|---|---|
| 环回收发 | Eltima COM4/COM5 | PASS | E4 |
| 拔线运行时故障 | 未插拔真机 | 未运行 | — |

## 未执行测试

| 测试 | 原因 | 风险 | 后续动作 |
|---|---|---|---|
| 物理拔线/ResourceError | 无自动化注入 | 中 | 用户真机拔线点测 |
| waitForBytesWritten 超时注入 | 无 mock 端口 | 低 | 代码路径已强制失败 |

## 需求—代码—测试追踪

| 需求 | 实现文件 | 测试 | 结果 |
|---|---|---|---|
| R1 写超时失败 | SerialTransport.cpp | 代码+构建 | 可提交审计 |
| R2 运行时错误拆卸 | NativeSession tearDownAfterFault | 环回+打开失败单事件 | 可提交审计（拔线未测） |
| R3 UI 单面错误 | MainWindow | 打开失败单 ErrorEvent | 可提交审计 |
| R4 Opening 禁开 | MainWindow::syncUi | 代码 | 可提交审计 |
| R5 枚举名 | Types.h + MainWindow | testEnumNames | 可提交审计 |
| R6 构建+自测 | MSBuild + CoreSelfTest | 0 failed | 可提交审计 |

## 已知问题

- BLOCKED：无
- 其他：成员仍为 m_ 前缀（本编号明确不做重命名）

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4（环回）；运行时拔线为未测
- 遗留风险：真机拔线/驱动 ResourceError 需人工点测
- 注意：最终结论由代码变更审计规范产生。