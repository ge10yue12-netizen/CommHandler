# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-UX-LHGK-001
- 标题：控制事件可读化 / 数据窗拆分 / 异常提示 / 联恒索引重开
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2（含 DLL 冻结例外）
- 日期：2026-07-20
- 范围：CommunicationAssistant UI + Legacy 展示；CommHandler 联恒索引重开；BASELINE 升级

## 2. 变更目标

- 原问题：控制事件只显示笼统文案；无独立数据窗；边界/断连提示弱；联恒在 DLL 中被注释且无能力表行。
- 预期：指令可读；数据与日志分离；异常分级提示；串口 5 / 网口 8 接通联恒。
- 明确不做：不重构其它协议；不声称设备 Golden 已验收；不做 M4 Codec/回放。

## 3. 工作区状态

- Git 工程：是
- 用户已有大量未跟踪构建产物；本变更聚焦源码与文档
- 审计方法：`git` 未提交；以本报告文件清单 + 构建/自测证据为准

## 4. 实际修改

| 文件 | 动作 | 原因 |
|---|---|---|
| `docs/BASELINE.md` | 修改 | 升 V1.2.5：DLL 例外、§14 联恒行、§14.4 线格式 |
| `docs/AUDIT_SPEC.md` | 修改 | 配套基线版本号 |
| `CommHandler/CommSdk/SerialPortComm.cpp` | 修改 | 串口 `iProtocolType=5` 收发联恒 |
| `CommHandler/CommSdk/SocketComm.cpp` | 修改 | 网口 `iProtoType=8` 收发联恒 |
| `CommHandler/Common/UIDef.h` | 修改 | 协议索引注释 |
| `CommHandler/CommHandler.vcxproj` | 修改 | Release SDK 改为本机可用的 10.0.19041.0 |
| `CommunicationAssistant/legacy/LegacyCapability.cpp` | 修改 | 串口 5 / 网口 8 能力表 |
| `CommunicationAssistant/legacy/LegacySession.cpp` | 修改 | 控制/参数事件可读 summary；故障断连文案 |
| `CommunicationAssistant/src/MainWindow.cpp/.h` | 修改 | 数据窗+日志窗；错误/断连/边界前缀 |
| `CommunicationAssistant/include/UIDef.h` | 修改 | 索引注释 |
| `CommunicationAssistant/tests/CoreSelfTest.cpp` | 修改 | 能力矩阵扩至网口 0–8、串口 0–5 |
| `CommLab/include/UIDef.h` | 修改 | 索引注释对齐 |
| `CommunicationAssistant/runtime/CommHandler.dll` | 更新 | Release 重编拷贝 |
| `CommunicationAssistant/lib/CommHandler.lib` | 更新 | 同步拷贝 |

## 5. 接口与格式影响

- 公共 UI 接口：仍仅经 `ICommSession`
- Capture JSONL：新增 attributes `msgName`（可选字段，向后兼容）
- 线程模型：无变化
- DLL：新增协议索引，不改既有 0–4 / 0–7 语义

## 6. 需求—代码—测试追踪

| 需求 | 实现位置 | 测试 | 结果 |
|---|---|---|---|
| R1 控制指令可读 | `LegacySession::onControlEvent` | 静态 + 联调目视 | 自测未覆盖 UI 目视；代码已写 |
| R2 数据窗 | `MainWindow` QSplitter | 构建通过 | PASS（构建） |
| R3 边界/断连提示 | `MainWindow` / `tearDown` | 构建通过 | PASS（构建） |
| R4 联恒能力表 | `LegacyCapability` | CoreSelfTest ser5/net8 | PASS |
| R5 DLL 重开索引 | Serial/SocketComm case 5/8 | DLL Release 构建 | PASS（构建） |
| R6 基线升级 | BASELINE V1.2.5 | 文档审阅 | PASS |

## 7. 执行的验证

| 验证项 | 命令/步骤 | 结果 |
|---|---|---|
| CommHandler Release | MSBuild + `WindowsTargetPlatformVersion=10.0.19041.0` | 成功 → `CommHandler.dll` |
| CommunicationAssistant Debug | MSBuild | 成功 |
| CoreSelfTest | `x64\Debug\CoreSelfTest.exe` | **2 failed**（COM4/COM5 环回），其余含联恒能力项 PASS |

## 8. 未执行验证

- 联恒真实设备/对端联调（无 Golden）
- CommunicationAssistant UI 人工点验控制事件与数据窗（需用户本机运行）

## 9. 偏差与风险

- 串口环回 `[FAIL] loop open RX` / `loop RX matches TX`：环境虚拟串口，与本变更无关
- 联恒 `handleFrame` 仍按整包 `readAll`/`onRecv` 解析，分片粘包未增强（沿用历史实现）
- 数值发送依赖 DLL `bInquireSendFlag`；助手侧标记 RequiresPolling/Streaming，但未新增强制门控 UI

## 10. BLOCKED / 后续

- Voltage/Pulse 仍 BLOCKED
- 联恒设备 Golden → 另立变更后方可现场验收 PASS

## 11. 审计结论

**CONDITIONAL PASS**

理由：范围与 V1.2.5 对齐；无 P0；关键路径已编译；能力表自测通过；缺联恒设备联调与 UI 目视，不得描述为完全完成。
