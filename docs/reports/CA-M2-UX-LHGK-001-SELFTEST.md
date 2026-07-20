# 自检自测报告

- 变更编号：CA-M2-UX-LHGK-001
- 日期：2026-07-20
- 基线：V1.2.5

## 构建

| 目标 | 配置 | 结果 |
|---|---|---|
| CommHandler | Release x64（SDK 10.0.19041.0） | PASS |
| CommunicationAssistant | Debug x64 | PASS |
| CoreSelfTest | Debug x64 | PASS（链接） |

## CoreSelfTest

```
=== done: 2 failed ===
```

失败项：

- `loop open RX`
- `loop RX matches TX payload`

说明：依赖本机 COM4/COM5 虚拟串口环回；与联恒/UI 变更无直接关系。  
联恒相关断言（`cap ser5*` / `cap net8*`）均为 PASS。

## 修改者自检（CODE_STYLE）

- [x] 未改无关协议分支语义（0–4 / 0–7 保留）
- [x] 注释为专业说明（联恒索引、数据窗职责）
- [x] 未引入 C++17 公共接口
- [ ] UI 目视（控制事件 / 双窗 / 断连前缀）— 待用户本机确认
- [ ] 联恒对端联调 — 未执行
