# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-SERIAL-FIX-001
- 变更标题：串口路径自检缺陷修复与 CoreSelfTest
- 基线版本：V1.2.3（docs/BASELINE.md）
- 审计规范版本：V1.1（docs/AUDIT_SPEC.md）
- 所属里程碑：M1
- 审计日期：2026-07-20
- 前置变更：CA-M1-SERIAL-001（结论曾为 CONDITIONAL PASS，缺 E4）
- 审计范围：SerialTransport / SessionManager / HEX 解析 / NativeSession 打开失败路径；新增 CoreSelfTest；虚拟串口环回验证

## 2. 变更目标

- 原问题：用户反馈 SERIAL-001 代码有问题；自检发现打开失败双报错、claim 换口泄漏、HEX 过松、参数未校验等
- 预期结果：修复上述缺陷；提供可重复运行的离线自测；补齐虚拟串口收发证据
- 明确不做：TCP/UDP、Scheduler、Capture、Legacy/Simulator、Codec、ProtocolFrame；不修改 CommHandler

## 3. 工作区状态

- Git 工程：是
- 本变更触及：CommunicationAssistant/ 与本报告
- 方法：MSBuild Debug|x64 + 运行 CoreSelfTest.exe（含 Eltima COM4↔COM5）

## 4. 实际修改

| 文件 | 操作 | 原因 |
|---|---|---|
| transport/SerialTransport.cpp | 修改 | 打开失败不 emit transportError；空口/非法帧参数回 Closed；clearError；有界 waitForBytesWritten；校验 dataBits/parity/stopBits |
| session/NativeSession.cpp | 修改 | 去掉 m_serial(this) 父子挂接；补充 QDateTime 头 |
| session/SessionManager.h | 修改 | 同会话换口时释放旧串口 claim；测试辅助查询 |
| core/HexUtil.h | 新增 | 严格 HEX（仅 0-9A-Fa-f 与空白） |
| src/MainWindow.cpp | 修改 | 发送走 HexUtil |
| tests/CoreSelfTest.cpp | 新增 | normalize/claim/HEX/互斥/打开失败/环回 |
| tests/CoreSelfTest.vcxproj | 新增 | 控制台自测工程 |
| CommunicationAssistant.sln | 修改 | 纳入 CoreSelfTest |
| CommunicationAssistant.vcxproj | 修改 | 登记 HexUtil.h |
| docs/reports/CA-M1-SERIAL-FIX-001.md | 新增 | 本报告 |

## 5. 接口与格式影响

- 无 Capture JSONL / 对外协议格式变更
- SessionManager 行为收紧：同 session 换 COM 时旧口立即释放（修复泄漏）
- UI HEX 解析更严：拒绝 0x 前缀与非法字符（行为变更，属正确性修复）

## 6. 需求—代码—测试追踪

| 需求 | 实现 | 测试 | 结果 | 等级 |
|---|---|---|---|---|
| R1 打开失败单次错误、状态 Closed、claim 释放 | SerialTransport + NativeSession | CoreSelfTest testNativeOpenFail | 通过 [VERIFIED] | E2 |
| R2 串口内部互斥与换口不泄漏 | SessionManager | CoreSelfTest testSessionManager | 通过 [VERIFIED] | E2 |
| R3 严格 HEX | HexUtil + MainWindow | CoreSelfTest testHex | 通过 [VERIFIED] | E2 |
| R4 claimFor / normalize | ClaimFor / ResourceClaim | CoreSelfTest testClaimFor / testNormalize | 通过 [VERIFIED] | E2 |
| R5 虚拟串口双端收发 | NativeSession×2 | CoreSelfTest testVirtualLoopback COM4→COM5 | 通过 [VERIFIED] | E4 |
| R6 主工程可编译 | CommunicationAssistant.vcxproj | MSBuild Debug|x64 | 通过 [VERIFIED] | E1 |

## 7. 执行的验证

| 验证项 | 命令/产物 | 结果 |
|---|---|---|
| MSBuild 解决方案 | MSBuild CommunicationAssistant.sln /p:Configuration=Debug /p:Platform=x64 | 成功：CommunicationAssistant.exe + CoreSelfTest.exe |
| CoreSelfTest | x64\Debug\CoreSelfTest.exe | === done: 0 failed ===；EXIT=0 |
| 环回环境 | QSerialPortInfo | COM4/COM5 Virtual Serial Port (Eltima) |
| 环回载荷 | TX CA FE 01 02 | RX 字节一致 |

## 8. 未执行验证

| 项 | 原因 | 风险 | 后续 |
|---|---|---|---|
| 真机物理串口 | 本机用虚拟对测 | 驱动/权限差异 | 用户真机点测 |
| UI 手工点测全路径 | 以 CoreSelfTest 覆盖核心路径 | 低 | 可选 |
| Release|x64 | 仅 Debug | 低 | 发布前补编 |

## 9. 发现问题（本轮修复清单）

| 级别 | 问题 | 处理 |
|---|---|---|
| P1 | 打开失败 transportError + ErrorEvent 双报 | 打开失败仅返回 Result，由会话发一次 ErrorEvent |
| P1 | SessionManager 同会话换口泄漏旧 COM | acquire 前释放旧 key |
| P1 | HEX fromHex 吞非法字符 | HexUtil 严格校验 |
| P2 | 非法 dataBits/parity/stopBits 未拦截 | open 前校验 Unknown* |
| P2 | SerialTransport 作为 QObject 子对象挂接 | 改为成员无 parent |
| P2 | 写路径无界内等待 | waitForBytesWritten(1000) + 短写失败 |
| — | 自测工程写入时 sln 曾损坏 | 已重写合法 sln 并成功编译 |

## 10. 基线符合性

- 仍为 Native Serial 切片；未链 CommHandler；无 Codec；无 ProtocolFrame
- 发送状态仍为 Queued→Submitted；RX 为 RawChunk
- 无永久 waitForBytesWritten(-1)

## 11. Diff 审计

范围限于 CommunicationAssistant（含 tests）与本报告；未改 CommLab / CommHandler / MachinePeer。

## 12. 审计结论

- 结论：**PASS**
- 依据：E1 编译通过；E2 CoreSelfTest 全绿；E4 虚拟串口 COM4↔COM5 收发一致；SERIAL-001 所缺设备 I/O 证据已由本编号补齐
- 遗留风险：真机驱动差异未测（已在 §8 登记）
- 对前置报告：CA-M1-SERIAL-001 的 CONDITIONAL PASS 缺口由本报告关闭；后续新功能仍须新变更编号与「确认修改」
- 允许的下一步：CA-M1-TCP-001 / CA-M1-UDP-001（须新对齐单 + 确认修改）

