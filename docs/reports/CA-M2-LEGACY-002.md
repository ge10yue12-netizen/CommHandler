# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-002
- 变更标题：链接真实 CommHandler + 对齐 setParameter 键名
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M2
- 审计日期：2026-07-20

## 2. 变更目标

- 关闭 CA_LINK_COMMHANDLER=0；按 CommLab 已验证键名配置 SocketComm/SerialPortComm
- UI 补充网口 TCP/UDP、服务端/客户端、本地端点；默认走真实 DLL（可勾 Mock）

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| legacy/ILegacyBackend.cpp | 真实键名 configure；COM/波特率索引映射 |
| core/SessionConfig.h | LegacyConfig 扩展 transferType/model/串口参数 |
| src/MainWindow.* | Legacy 网口角色/传输/本地端口 |
| *.vcxproj | CA_LINK_COMMHANDLER=1；lib；PostBuild 复制 DLL |
| tests/CoreSelfTest.cpp | dll smoke |
| docs/reports/CA-M2-LEGACY-002*.md | 本报告集 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| D1 链接 Debug CommHandler | PASS | E4 |
| D2 键名对齐 Serial/NetworkProtocol | PASS | E2（源码核对） |
| D3 Mock 回归仍 0 failed | PASS | E4 |
| D4 DLL configure 冒烟 | PASS | E4 |
| D5 对端联调收发 | 未做（无强制设备） | — |

## 5. 审计结论

- 结论：**CONDITIONAL PASS**
- 条件：端到端对端协议联调需用户设备；Voltage/Pulse 仍关闭
- 下一步：可选 CA-M2-LEGACY-003 指定协议双端冒烟清单