# 自检自测报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-001
- 日期：2026-07-20
- 工具链：VS2019 v142 + Qt 5.15.2 msvc2019_64 + C++14

## 2. 自检要点

| 项 | 结果 |
|---|---|
| Legacy 非 TransportKind；单会话互斥 | 通过 |
| Worker 线程创建后端；析构避免 BlockingQueued 死锁 | 通过 |
| 裸指针 Mock 立即复制 | 通过 |
| 能力表 limitation 在 supported=true 时保留 | 通过 |
| CaptureWriter close 与 wait 谓词同锁 | 通过 |
| initialize 投递使用 moc 类型名 LegacyConfig | 通过 |

## 3. 构建与测试证据

```text
MSBuild CoreSelfTest (Debug|x64) → 成功
CoreSelfTest.exe → === done: 0 failed ===
主程序：临时 ProjectGuid 绕过 QtMsBuild CriticalSection 后构建成功，
已覆盖 x64\Debug\CommunicationAssistant.exe
```

## 4. 已知限制

- CA_LINK_COMMHANDLER=0：真实 CommHandler 联调未做
- Voltage/Pulse 未开放