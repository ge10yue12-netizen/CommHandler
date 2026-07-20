# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-SCHED-001
- 变更标题：SendScheduler（锚定 Submitted）+ UI
- 开发基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- 计划：SendScheduler 应用服务、MainWindow 调度控件、CoreSelfTest、报告
- 实际：同上；顺带加固 TCP 双客户端 Channel 按端口映射（回归用例稳定性）
- 范围偏差：无 Capture / 波形 / 绝对频率

## 静态自检

| 项 | 结果 |
|---|---|
| 仅通过 ICommSession::send | 是 |
| 锚点 Submitted | 是（含延迟 Submitted 自测） |
| Failed 停任务不重试 | 是 |
| 单次/次数/无限/轮询/暂停继续停止 | 是 |
| 不为每任务建线程 | QTimer singleShot |

## 构建与测试

| 项 | 结果 | 等级 |
|---|---|---|
| MSBuild Debug\|x64 | 成功 | E1 |
| CoreSelfTest 0 failed | 含 Scheduler + 串口/TCP/UDP 回归 | E4 |

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4