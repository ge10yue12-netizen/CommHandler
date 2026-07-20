# 变更对齐单（实施后归档）

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-001
- 变更标题：M2 Legacy DLL 兼容骨架（Mock + 能力表 + Worker/Watchdog + UI）
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M2
- 是否已获「确认修改」：是（用户「确认修改M2」）

## 2. 目标

- LegacyWorker 线程内创建/销毁后端；Mock 路径可离线验收；CommHandler 后端以 CA_LINK_COMMHANDLER 门控（本刀默认 0）
- Direct 边界复制 + Queued 上送；能力表二元驱动 UI/发送校验
- SessionManager activeLegacySessionId 单会话；Watchdog → Unresponsive
- Legacy*Event 可经 Capture 落盘；MainWindow 暴露 Native/Legacy 模式

## 3. 明确不做

- 不修改 CommHandler DLL；本刀不强制链接真实 DLL
- 不开放 Voltage/Pulse；不做多 LegacySession；不做 Simulator/回放/Codec

## 4. 验收标准

- CoreSelfTest：0 failed
- 主程序可构建；UI 可选 Legacy + Mock
- 真实 DLL 联调列为后续刀（CA-M2-LEGACY-002）