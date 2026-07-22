# 自检自测报告 — CA-M2-UX-BASELINE-UI-001

| 项 | 内容 |
|---|---|
| 变更编号 | CA-M2-UX-BASELINE-UI-001 |
| 日期 | 2026-07-22 |
| 基线 | V1.2.5 |
| 范围 | 助手 UI 分区/清除/发送列表/下拉三角；DLL 相对 CommHandler_1 兼容核查（未改 DLL） |

## 1. 构建证据

| 命令 | 结果 |
|---|---|
| MSBuild Debug\|x64（默认 OutDir） | 编译通过；链接 LNK1168（`CommunicationAssistant.exe` 被占用，拒绝写入） |
| MSBuild Debug\|x64，OutDir=`x64\Debug_ui001\` | **PASS** → `CommunicationAssistant\x64\Debug_ui001\CommunicationAssistant.exe` |
| UI contract | **PASS**（90 bound names） |

## 2. 静态核对

| 需求 | 核对 |
|---|---|
| 侧栏内收 + 分区名 | `.ui`：root 左边距 10；sideLay 左 12；连接区/通信配置区/接收设置区/发送设置区/能力说明区 |
| 数据/日志清除 | `btnClearData` / `btnClearLog` 在对应窗右下；侧栏旧「清空」已移除 |
| 发送列表 | 无「发送启用行」；无延时列；有「全部清空」；删除按勾选；右键菜单；「发送」按钮 |
| 模式文案 | 单次 / 轮询列表 / 周期性发送 / 指定次数；间隔/次数按模式显隐 |
| 下拉三角 | QSS `::down-arrow` → `:/ca/icons/combo_down.png`；`app.qrc` 已入工程 |
| 能力说明区 | `refreshLegacyCapabilityTips` 按协议展开收发/边界/日志关注点 |

## 3. DLL 正式基线（CommHandler_1）

| 项 | 结论 |
|---|---|
| 对照对象 | `CommHandler_1`（正式宿主所用库树，HEAD 与最初 `a82b2ad` 同线） |
| 公开头差分 | 当前 `CommHandler.h` 相对 `_1` **新增**信号 `emitUnparsedRx`；其余公共 API 形状保持 |
| 正式路径风险 | 正式软件不连接新信号、不设 `bAssistObserve` 时，观测路径关闭；连通/线帧修复为行为增强，非索引破坏 |
| 本变更动作 | **未修改** `CommHandler` 源码；助手仅对齐说明与 UI |
| 部署注意 | 助手须使用与导入同源的 Release/Debug CRT；正式替换 DLL 前按 `COMM_TEST_CASE_MANUAL` 冒烟 |

## 4. 未运行 / 待目视

- 未在本机启动 UI 目视下拉三角与分区间距（请用户打开 `Debug_ui001` 或关闭占用后重链默认目录）
- CoreSelfTest 未因本变更强制重跑（调度语义：单次多勾选改为发完一轮）

## 5. 结论

**CONDITIONAL PASS**（构建在备用 OutDir 通过；默认目录被进程占用；UI 目视待确认）
