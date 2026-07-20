# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-SEND-UX-001
- 标题：数据区收指令+测数；收发控件解锁；Legacy 发送解析与列表
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2
- 日期：2026-07-20
- 范围：CommunicationAssistant UI / LegacySession 提示文案（不改 DLL）

## 2. 变更目标

- 接收的指令与测数均进入数据显示区，便于核对
- 收发相关操作不按协议灰锁；不可用时日志说明原因
- 修复 `3D` 被误判为数值导致发送失败；兼容模式明确拒绝原始 HEX
- Legacy 支持列表轮询（每行一组）

## 3. 实际修改

| 文件 | 动作 |
|---|---|
| `CommunicationAssistant/src/MainWindow.cpp` | 数据路由、解锁 syncUi、发送解析、列表载荷、占位符 |
| `CommunicationAssistant/src/MainWindow.h` | `legacySendModeCombo_`、`formatRecordForDisplay`、`updateSendPlaceholders` |
| `CommunicationAssistant/legacy/LegacySession.cpp` | 发送拒绝文案更明确 |

## 4. 验证

| 项 | 结果 |
|---|---|
| CommunicationAssistant Debug 构建 | PASS |
| UI 目视 / 协议联调 | 待用户本机确认 |
| CoreSelfTest | 未重跑（本变更以 UI 为主） |

## 5. 审计结论

**CONDITIONAL PASS** — 构建通过；现场点验待确认。
