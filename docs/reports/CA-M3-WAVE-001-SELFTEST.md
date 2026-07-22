# 自检自测报告 — CA-M3-WAVE-001

| 项 | 内容 |
|---|---|
| 变更编号 | CA-M3-WAVE-001 |
| 日期 | 2026-07-22 |
| 范围 | 波形发送、原生辅助、兼容边界硬化；CommHandler **未改** |

## 构建

```text
msbuild CommunicationAssistant.vcxproj /p:Configuration=Debug /p:Platform=x64
→ CommunicationAssistant.exe 成功

msbuild tests/CoreSelfTest.vcxproj /p:Configuration=Debug /p:Platform=x64
→ CoreSelfTest.exe 成功
```

## 自测

| 项 | 结果 |
|---|---|
| wave sample / csv / sched three samples | PASS |
| 既有调度 Once/Counted/RR/Pause | PASS |
| 能力表 Raw 恒关、联恒 5/8 | PASS |
| `loop open RX` / `loop RX matches` | FAIL（COM4/5 环境环回，历史已知，非本变更逻辑） |
| CommHandler 源码 diff | 无（本变更未触碰） |

## 手工核对建议

1. 原生：打开串口/TCP → 波形发送页 → 启动波形 → 对端见 CSV 周期变化  
2. 兼容联恒 5/8：通道≥2 → 启动波形 → 入参变化；未许可时边界拒绝  
3. 兼容网口三思/威盛/时代：启动波形 → `[边界] 不能发`  
4. 勾选「接收对照 ASCII/HEX」「发送后清空单条内容」行为符合预期  

## 结论

**CONDITIONAL PASS**：功能与构建达标；串口环回环境项失败不阻塞本变更；现场协议验收仍须对照测试手册与真机 Golden。
