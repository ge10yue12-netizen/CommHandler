# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-CAPTURE-001
- 变更标题：CaptureManager + CaptureWriter（JSONL）
- 开发基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- CaptureJson / CaptureWriter / CaptureManager；MainWindow 抓包开关；CoreSelfTest；报告
- 明确未做：CaptureReader / 回放 UI

## 静态自检

| 项 | 结果 |
|---|---|
| 每 Session 独立 Writer/文件/队列 | 是 |
| JSONL 文件头 magic=CAREC01 formatVersion=1 | 是 |
| 必选顶层字段 + bytesBase64 + 序号/ns 字符串 | 是 |
| 仅听 recordReceived | 是 |
| UI 清空不删抓包文件 | 是 |
| 队列满明确错误 | 是 |

## 构建与测试

| 项 | 结果 | 等级 |
|---|---|---|
| MSBuild Debug\|x64 | 成功 | E1 |
| CoreSelfTest 0 failed | JSON 往返、会话隔离、队列满、残缺尾行 | E4 |

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4