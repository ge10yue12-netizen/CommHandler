# 变更对齐单（实施后归档）

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-002
- 变更标题：链接真实 CommHandler + 对齐 setParameter 键名
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M2
- 是否已获「确认修改」：是（「确认修改，继续」）

## 2. 目标

- CA_LINK_COMMHANDLER=1；Debug/Release 分别链接对应 CommHandler.lib
- setParameter 键名对齐 CommLab NetworkProtocol / SerialProtocol（经源码核对）
- PostBuild 复制 CommHandler.dll 及硬依赖厂商 DLL
- CoreSelfTest 增加 DLL configure/open 冒烟（无对端允许 Connect 失败）

## 3. 明确不做

- 不修改 CommHandler 源码；不开放 Voltage/Pulse
- 不保证无厂商 DLL 的机器可加载（须本机 ART-DAQ / CommLab 侧车 DLL）

## 4. 验收标准

- CoreSelfTest：0 failed（含 dll smoke）
- 主程序可构建并带 CommHandler.dll