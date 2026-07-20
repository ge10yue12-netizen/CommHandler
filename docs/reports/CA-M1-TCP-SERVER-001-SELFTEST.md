# 变更自检自测报告

## 基本信息

- 变更编号：CA-M1-TCP-SERVER-001
- 变更标题：TCP Server Channel 隔离 + UI
- 开发基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M1

## 修改范围

- TcpServerTransport、NativeSession、SessionManager 监听互斥、MainWindow TCP-S、CoreSelfTest、报告
- 明确未做：UDP/Scheduler/Capture/Codec

## 构建与测试

| 项 | 结果 | 等级 |
|---|---|---|
| MSBuild Debug\|x64 | 成功 | E1 |
| CoreSelfTest 0 failed | 含 listen 冲突、双客户端定向/广播、串口/TCP Client 回归 | E2/E4 |

## 修改者结论

- 结论：可提交审计
- 最高证据等级：E4