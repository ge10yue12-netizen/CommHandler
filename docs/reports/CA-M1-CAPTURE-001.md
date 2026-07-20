# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-CAPTURE-001
- 变更标题：CaptureManager JSONL 落盘
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1
- 审计日期：2026-07-20

## 2. 变更目标

- CaptureManager → 每 Session CaptureWriter；有界队列后台写；`.clog.jsonl`；UI 可选抓包
- 明确不做：CaptureReader、回放页、改 JSONL 字段语义以外的格式

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| capture/CaptureJson.h | 新增 |
| capture/CaptureWriter.* | 新增 |
| capture/CaptureManager.* | 新增 |
| src/MainWindow.* | 抓包开关与接线 |
| *.vcxproj / CoreSelfTest | 纳入与测试 |
| docs/reports/CA-M1-CAPTURE-001*.md | 本报告 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| C1 JSONL 字段稳定 | PASS | E4 |
| C2 字节 Base64 往返 | PASS | E4 |
| C3 会话隔离 | PASS | E4 |
| C4 队列满 | PASS | E4 |
| C5 残缺尾行可丢弃 | PASS | E4 |
| C6 回归 | PASS | E4 |

## 5. 审计结论

- 结论：**PASS**
- M1 原生路径（Serial/TCP/UDP/Scheduler/Capture）已齐；下一步可为 M2 Legacy 或文档收尾