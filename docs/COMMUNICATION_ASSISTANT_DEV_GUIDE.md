# 通信调试助手 — 开发说明

| 项 | 内容 |
|---|---|
| 文档标识 | CA-DEV-001 |
| 产品名称 | 通信调试助手（CommunicationAssistant） |
| 基线版本 | V1.2.5 |
| 读者 | 研发、移植、维护与二次开发人员 |
| 关联文档 | `docs/BASELINE.md`、`docs/AUDIT_SPEC.md`、`docs/CODE_STYLE.md`、`docs/LEGACY_PROTOCOL_DEV_GUIDE.md`、`docs/COMM_TEST_CASE_MANUAL.md` |

---

## 1. 产品定位

通信调试助手用于在开发与联调阶段验证通讯链路与协议行为，支持两类工作模式：

| 模式（界面文案） | 内部标识 | 说明 |
|---|---|---|
| 原生通道 | `WorkMode::Native` | 应用内直接驱动串口 / TCP / UDP，载荷按侧栏格式设置透明发送与显示 |
| 兼容动态库 | `WorkMode::LegacyDll` | 通过 CommHandler 完成厂商协议编解码；界面侧数值为业务入参 |

产品可能被移植到其他工程。对外说明、界面与日志应使用稳定、可移植的术语，避免口语化与仅限内部的简称。

---

## 2. 仓库与模块结构

| 路径 | 职责 |
|---|---|
| `CommunicationAssistant/` | Qt 应用程序：UI、会话、调度、抓包、兼容通道封装 |
| `CommunicationAssistant/core/` | 会话接口、记录类型、结果类型 |
| `CommunicationAssistant/transport/` | 原生传输：Serial / TcpClient / TcpServer / Udp |
| `CommunicationAssistant/session/` | `NativeSession` |
| `CommunicationAssistant/legacy/` | `LegacySession`、能力表、Worker、后端抽象 |
| `CommunicationAssistant/services/` | 发送调度 |
| `CommunicationAssistant/capture/` | JSONL 会话记录落盘 |
| `CommHandler/` | 兼容动态库实现（协议与链路） |
| `docs/` | 基线、审计规范、测试与开发文档 |

---

## 3. 架构边界（必须遵守）

```text
[ UI / MainWindow ]
        │
        ├─ NativeSession ──► ITransport（串口/TCP/UDP）
        │
        └─ LegacySession ──► LegacyWorker ──► ILegacyBackend
                                              ├─ MockBackend
                                              └─ CommHandlerBackend ──► CommHandler.dll
```

| 层级 | 允许 | 禁止 |
|---|---|---|
| UI | 展示入参、状态、未解析线帧、能力说明 | 在 UI 内复刻动态库组帧并宣称与库一致 |
| NativeSession | 真实线帧收发与连接真值 | 伪造「已连接」 |
| LegacySession | 能力门禁、入参转发、未解析可观测 | 绕过能力表发送原始十六进制 |
| CommHandler | 协议索引语义、线帧字节 | 擅自更改历史协议索引含义而不更新基线 |

---

## 4. 关键数据流

### 4.1 原生通道发送

```text
发送工作台文本
  → 侧栏「十六进制发送」决定编码
  → NativeSession::send
  → Transport::write
```

### 4.2 兼容动态库发送

```text
数值 CSV / 透明文本（业务入参）
  → LegacyCapability 校验
  → LegacySession::send → Worker → CommHandler::SendData(...)
  → SerialPortComm / SocketComm 按协议索引编码
  → 串口 write / NetworkBox::sendData
```

界面「数据显示」中的发送记录对兼容通道标注**入参**；动态库接口不回传已发送原始线帧。

### 4.3 兼容动态库接收

```text
链路字节
  → 协议 case 解析
  → 成功：emitNewData / 控制事件 → 数据显示「数值/控制」
  → 失败：emitUnparsedRx → 数据显示十六进制线帧 + 运行日志「协议解析失败」
```

设计要求：对端已发包但格式不匹配时，不得表现为「未收到」。

---

## 5. 连接真值

连通判定仅依赖传输层结果，与协议索引无关。

| 传输 | 成功条件（摘要） | 界面状态语义 |
|---|---|---|
| 串口 | 端口已打开 | 已连接 |
| TCP 客户端 | 握手成功且状态为 Connected | 已连接 |
| TCP 服务端 | 监听成功 | 已监听（等待客户端） |
| UDP | 本地绑定成功 | 已绑定 |

打开过程中对端断开或失败，不得残留「已连接」。

---

## 6. 协议能力表

权威实现：`CommunicationAssistant/legacy/LegacyCapability.cpp` 中 `legacyCapabilityFor()`。

原则：

1. 与 CommHandler 源码分支对齐；未验证路径默认「不支持」。  
2. 网口三思（索引 3）无 `SendData(vector)` 分支 → 发数值关闭。  
3. 兼容通道恒关闭 RawSend / RawReceive；线帧级调试使用原生通道。  
4. 变更能力开关须同步基线、测试手册与本说明。

协议线帧细节见 `docs/LEGACY_PROTOCOL_DEV_GUIDE.md`；完整用例见 `docs/COMM_TEST_CASE_MANUAL.md`。

---

## 7. 用户界面与人机交互文案

### 7.1 术语对照（对外）

| 对外文案 | 内部符号 |
|---|---|
| 原生通道 | Native |
| 兼容动态库 | LegacyDll / CommHandler |
| 模拟后端 | Mock backend |
| 会话记录落盘 | Capture JSONL |
| 协议能力 | Capability tips |
| 入参 | 业务 CSV/文本，非线帧 |

### 7.2 文案规范

1. 使用书面语，避免口语（如「不要填」「点发送」「在跑」）。  
2. 错误与边界统一前缀：`[边界]`（策略拒绝）、`[异常]`（故障）、`[连接]`（状态迁移）。  
3. 二进制协议须说明：对端应以十六进制核对，勿以文本乱码判定发送失败。  
4. 移植时保留中文说明或按目标市场做完整本地化，禁止中英口语混杂。

### 7.3 布局约束（侧栏）

| 控件 | 约束 |
|---|---|
| `connMethodStack` | 原生：传输类型；兼容：网口/串口；同高切换 |
| `paramStack` | 仅设最小高度，禁止用最大高度压扁内容 |
| `legacyEndpointStack` | 网口/串口端点等高 |
| 协议能力区 | 独立于参数表单，保证可读高度 |
| 客户端广播区 | 当前版本默认隐藏，逻辑保留 |

修改 `.ui` 时禁止大块无校验替换；改后须经 `uic` 与 `tools/check_ui_contract.ps1` 验证。

**Designer 可手改约束（强制）**

1. `QFormLayout` 的 `<item>` **禁止** `colspan` / `rowspan`（Designer 保存后易损坏；工程预检会直接失败）。跨列内容用外层 `QVBoxLayout`（如 `layLegacyPage`）承接。  
2. **可自由增删改布局与文案**；新增仅展示控件不必改 C++。  
3. **不可删除或改名** `tools/ui_contract.txt` 中的绑定 objectName；若必须改名/删除，须同步改 `MainWindow.cpp`（及合同文件）。  
4. `mainPanel` / `sendWorkbenchPane` 为可选样式外壳：可拆可留，C++ 用 `findChild` 兼容，不再因此 C2039。  
5. 对齐与密度优先在代码 `applySideFormAlign` / QSS 调整。  
6. 手改保存后：Designer 无报错 → 本地编译（预检 + uic）通过为准。

编译前自动执行：`msbuild/UiContract.targets` → `tools/check_ui_contract.ps1`。
---

## 8. 构建与部署

| 目标 | 配置建议 |
|---|---|
| CommHandler | Release\|x64 |
| CommunicationAssistant | Debug\|x64 或 Release\|x64（与导入库 CRT 一致） |
| 部署 | 将 `CommHandler.dll` 复制到 `runtime/` 与可执行文件目录 |

注意：Release 导入库链接 Debug 动态库易导致 CRT 冲突崩溃。覆盖 DLL 前关闭占用进程。

自测入口：`CommunicationAssistant/tests/CoreSelfTest`。

---

## 9. 审计与变更纪律

1. 功能变更按 `docs/AUDIT_SPEC.md` 出具审计报告与自检记录。  
2. 协议索引、线帧、能力表变更必须三方同步：基线、开发说明、测试手册。  
3. 动态库行为变更属例外路径，须在审计报告中单列根因与回归项。  
4. 提交前完成构建；涉及 UI 时验证 Designer 可打开 `.ui`。

---

## 10. 扩展与移植指引

移植到其他产品时建议：

1. 保留 `ICommSession` / `ITransport` / `ILegacyBackend` 边界，避免 UI 直连动态库。  
2. 重新配置协议能力表与产品术语表。  
3. 使用 `docs/COMM_TEST_CASE_MANUAL.md` 作为验收清单模板。  
4. 动态库协议表以 CommHandler 源码为准，不以界面示例字符串为准。

---

## 11. 修订记录

| 日期 | 说明 |
|---|---|
| 2026-07-21 | 初版：架构、数据流、连接真值、文案规范、构建与移植 |
