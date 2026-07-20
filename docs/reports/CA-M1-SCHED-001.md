# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M1-SCHED-001
- 变更标题：SendScheduler + 周期/轮询 UI
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1
- 审计日期：2026-07-20

## 2. 变更目标

- 落地 SendScheduler：固定延迟；单次/指定次数/无限/列表轮询；暂停/继续/停止；锚点 Submitted；Failed 停任务
- UI：发送调度区
- 明确不做：Capture、波形、绝对频率、延迟追赶、硬实时

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| services/SendScheduler.h/.cpp | 新增 |
| src/MainWindow.* | 调度 UI 与接线 |
| *.vcxproj | 纳入 services |
| tests/CoreSelfTest.cpp | Scheduler FakeSession 自测；TCP Channel 按端口映射 |
| docs/reports/CA-M1-SCHED-001*.md | 本报告 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| S1 单次/次数/无限 | PASS | E4 |
| S2 列表轮询 | PASS | E4 |
| S3 暂停/继续/停止 | PASS | E4 |
| S4 锚点 Submitted | PASS | E4 |
| S5 Failed 停任务 | PASS | E4 |
| S6 传输回归 | PASS | E4 |

## 5. 审计结论

- 结论：**PASS**
- 下一步：`CA-M1-CAPTURE-001`（CaptureManager JSONL）