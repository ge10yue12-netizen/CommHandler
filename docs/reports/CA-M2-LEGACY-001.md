# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-001
- 变更标题：M2 Legacy DLL 兼容骨架（Mock + 能力表 + Worker/Watchdog + UI）
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M2
- 审计日期：2026-07-20

## 2. 变更目标

- 落地 M2 首刀：LegacySession / Worker / Watchdog / 能力表 / Mock 后端；UI 按能力提示；Capture 可记录 Legacy*Event
- 明确不做：改 DLL、默认链接真实 CommHandler、Voltage/Pulse、多 Legacy、Simulator

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| legacy/* | Capability、Backend、Worker、Watchdog、Session |
| session/SessionManager.h 等 | Legacy 互斥与配置 |
| capture/CaptureWriter.cpp | close lost-wakeup 修复 |
| src/MainWindow.*、main.cpp | Legacy UI + metatype |
| tests/CoreSelfTest.* | Legacy 用例 |
| docs/reports/CA-M2-LEGACY-001*.md | 本报告集 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| L1 能力表二元 + Raw 恒关 | PASS | E4 |
| L2 单 Legacy 互斥 | PASS | E4 |
| L3 Mock open/send/close | PASS | E4 |
| L4 能力拒绝发送 | PASS | E4 |
| L5 Watchdog 超时信号 | PASS | E4 |
| L6 UI Legacy 入口 | PASS | E2 |
| L7 真实 DLL Connect/Send | 未做（门控 0） | — |

## 5. 审计结论

- 结论：**CONDITIONAL PASS**
- 条件：真实 CommHandler 链接与联调留待 CA-M2-LEGACY-002
- 下一步：CA_LINK_COMMHANDLER=1、核对 setParameter 键名、端到端 DLL 冒烟