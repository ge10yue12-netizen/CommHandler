# 变更对齐单 — CA-M3-WAVE-001

| 项 | 内容 |
|---|---|
| 变更编号 | CA-M3-WAVE-001 |
| 基线版本 | V1.2.5 |
| 审计规范版本 | V1.2 |
| 所属里程碑 | M3（波形切片；不做试验机模拟） |
| 需求来源 | 用户确认修改：波形双模式 + 原生完善 + 兼容硬化；**动态库不动** |
| 问题描述 | 助手缺随机/正弦波形发送；兼容需贴近真实宿主；原生需补常见调试能力 |
| 预期结果 | Native/Legacy 均可波形发送；不能发协议边界清晰；原生 HEX/ASCII 对照与发送后清空 |
| 明确不做 | 不修改 CommHandler；不做试验机/MachinePeer 角色；不在助手内复刻协议组帧 |
| 是否已获「确认修改」 | 是 |

## 计划文件

**新增**

- `CommunicationAssistant/services/WaveformGenerator.h/.cpp`
- `docs/reports/CA-M3-WAVE-001.md`
- `docs/reports/CA-M3-WAVE-001-SELFTEST.md`

**修改**

- `SendScheduler.*`（Waveform 模式）
- `MainWindow.*`（波形页、原生辅助选项）
- `LegacyCapability.cpp` / `ILegacyBackend.cpp`（不能发说明与边界）
- `CommunicationAssistant.vcxproj`、`tests/CoreSelfTest.*`
- `docs/COMMUNICATION_ASSISTANT_DEV_GUIDE.md`、`docs/BASELINE.md`（M3 说明）

## 验收标准

1. Debug\|x64 助手编译通过  
2. CoreSelfTest：波形相关项 PASS；既有环回环境 FAIL 可记为环境项  
3. 兼容不能发协议：能力说明含「不能发」文案  
4. DLL 目录无改动  
