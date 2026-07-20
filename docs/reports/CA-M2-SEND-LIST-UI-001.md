# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-SEND-LIST-UI-001
- 标题：发送工作台 + 列表表格 + txt/csv 导入导出
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2
- 日期：2026-07-20

## 2. 变更目标

- 主区底部「发送工作台」：单条发送 / 发送列表 Tab
- 发送列表表格（启用、名称、载荷、延时、格式）
- 支持选择本机路径导入/导出 `.txt` / `.csv`
- 调度从列表启用行取载荷；Legacy 行格式经 Scheduler attributes 传递

## 3. 实际修改

| 文件 | 动作 |
|---|---|
| `src/MainWindow.h/.cpp` | 工作台布局、列表、导入导出、调度对接 |
| `services/SendScheduler.h/.cpp` | `payloadAttributes` / `payloadIntervals` |
| 侧栏 | 仅保留连接 + 显示/抓包；发送迁出 |

## 4. CSV 约定

`enabled,name,payload,interval_ms,mode`  
mode：`自动` / `数值` / `文本` / `HEX`  
TXT：一行一组载荷；`#` 注释

## 5. 验证

| 项 | 结果 |
|---|---|
| Debug 构建 | PASS |
| UI 目视 / 导入导出联调 | 待用户确认 |

## 6. 结论

**CONDITIONAL PASS**
