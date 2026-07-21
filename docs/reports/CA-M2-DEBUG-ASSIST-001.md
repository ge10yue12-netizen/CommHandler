# 代码变更审计报告



## 1. 基本信息



- 变更编号：CA-M2-DEBUG-ASSIST-001

- 标题：调试助手交互：启用勾选发送、协议边界日志、十六进制发送勾选、TX 入参/线帧语义

- 基线版本：V1.2.5

- 审计规范版本：V1.2

- 里程碑：M2

- 日期：2026-07-21

- 范围：CommunicationAssistant UI / Legacy 边界可观测 + CommHandler `emitUnparsedRx` / 联恒断帧拼帧



## 2. 变更目标



- 「发送选中行」与调度一致：只处理列表「启用」勾选行，不以鼠标高亮为准。

- Legacy 各协议发送边界均写清中文日志（含协议名）；原始 HEX 一律拒绝并说明改用 Native。

- 侧栏「发送设置」在 Legacy 下仍可见「十六进制发送」勾选，便于单条边界自测。

- 数据显示：Native 展示真实线帧；Legacy TX 标明为业务入参，避免把 CSV 的 UTF-8 误当成串口线帧。



## 3. 明确不做



- 不在助手内复刻 DLL 协议组帧（不创造线帧）。

- 不改变 Capture JSON 英文字段名。

- 具名协议（串口 0–3 / 网口 0–7）解析失败补 `emitUnparsedRx` 仍 `[DEFERRED]`。



## 4. 根因（自检）



| 现象 | 根因 |

|---|---|

| 发送选中行 | `onSendListSendSelected` 用 `selectedRows()`（高亮） |

| 调度已正确 | `onSchedStartClicked` 已按「启用」勾选过滤 |

| Legacy 无十六进制发送勾选 | `syncUi` 中 `hexSendCheck_->setVisible(!legacy)` |

| 三思等发 HEX | 已有拒绝，但文案未带协议名；示例行若被设成 HEX 易误解 |

| 数据窗 HEX 像线帧 | Legacy `rec.bytes=request.payload`（CSV 文本），勾选十六进制显示后变成 `31 32…` 误导 |



## 5. 实际修改



| 文件 | 动作 |

|---|---|

| `src/MainWindow.cpp/.h` | 启用行发送；协议名边界日志；Legacy 显示十六进制发送勾选；TX 展示语义 |

| `ui/MainWindow.ui` | 按钮文案「发送启用行」 |

| `legacy/*` | 能力拒收边界、未解析链、单包上限与限速 |

| `CommHandler` | 见 §10 |

| `docs/reports/CA-M2-DEBUG-ASSIST-001*.md` | 本报告与自检 |



## 6. 验收要点



- 仅勾选行参与「发送启用行」与启动调度。

- Legacy + 十六进制发送 / 列表格式=十六进制 → 日志含协议名并拒绝。

- Native TX/RX 十六进制可对齐比对；Legacy TX 明示「入参非线帧」。



## 7. 审计结论



**CONDITIONAL PASS**（含 §8–§11 增补）



- 编译通过；若进程占用 exe 则需关闭后重链

- 发侧长文案优化仍 `[DEFERRED]`



## 8. 续：接收边界 + 发送格式互斥（同编号增补）



- 日期：2026-07-21

- 范围：仅接收静默丢弃治理 + 十六进制发送与 Legacy 下拉互斥；发侧长文案暂不改



### 接收



| 项 | 行为 |

|---|---|

| 能力表拒收数值/控制/参数 | `[边界] 收 \| 串口0 \| 收数值✗ 丢弃N`（禁止静默） |

| 计数区 | `末收:从未` / `末收:Ns前` |



### 发送格式互斥（UI）



| 项 | 行为 |

|---|---|

| 勾选「十六进制发送」 | 最高权限；Legacy 下拉禁用；发路径按十六进制意图 |

| 改 Legacy 下拉 | 自动取消十六进制勾选 |



## 9. 续：Legacy 未解析收包可观测



- 目的：动态库设计无业务回调时，仍要知道「来数据了但协议解不出」

- DLL：`emitUnparsedRx`（串口/网口 default；联恒完整坏帧）

- 助手：数据窗展示线帧十六进制 + `[边界] 收 | 串口N | 解析失败 n字节`

- 仍「用库」：不在助手内造协议，仅把库上报的原始字节展示出来



## 10. 动态库改了什么、为什么改



### 改了什么



| 项 | 内容 |

|---|---|

| 新信号 | `SerialPortComm` / `SocketComm` / `CommHandler::emitUnparsedRx` |

| 转发 | Comm → 助手 `CommHandlerBackend`（Direct 拷贝）→ Session（Queued） |

| 联恒接收 | `probeCompleteFrameSize` + `processLhgkRxStream` 拼帧缓冲 |

| 默认协议 | `default` 分支：无具名解析时上报原始字节 |



### 为什么必须改 DLL（不能只改助手）



CommHandler 设计是「只回调已解析业务」（数值/控制/参数）。助手拿不到原始进线字节，就无法回答「线上来了什么」。未解析可观测只能由库在协议失败路径上报；助手只做展示与限流。



### 接收断帧（与发送节流的区别）



| 侧 | 设计 |

|---|---|

| 发送 | 调度延时 / 列表间隔控制「发多快」 |

| 接收 | TCP/串口流**无消息边界**；一帧可能拆成多次 `readyRead`，也可能粘包 |



联恒帧：`AA 55 | type | len | payload | CRC | 0D 0A`。



| 状态 | 行为 |

|---|---|

| **断帧 Incomplete** | 缓冲等待，**不** `emitUnparsedRx` |

| **完整且校验通过** | `handleFrame` 走业务回调 |

| **完整但坏帧 / 无法对齐噪声** | `emitUnparsedRx`（可观测） |

| 缓冲上限 | 64KiB 溢出截断上报并清空，防 OOM |



## 11. 续：审计整改（半包 / 上限 / DLL 同源）



- 日期：2026-07-21

- 联恒：断帧不误报；完整坏帧才未解析

- 助手：`kMaxUnparsedBytes=4096`，`kUnparsedMinIntervalMs=100`（窗口内抑制并汇总）

- 产物：Debug/Release/runtime 同源重编并拷贝含 `emitUnparsedRx` 的 DLL


## 12. 续：真正连通才显示「已连接」

- 日期：2026-07-21
- 根因：TCP 客户端等待/状态复核偏弱；UDP 曾恒 true；Opening 期间对端断开被忽略导致误标 Connected
- DLL：TcpSocket 5s 等待 + ConnectedState 复核；SocketComm UDP 用 bind 结果；串口 isOpen + bComOpened
- 助手：Opening 期 disconnected → 失败收尾；TCP 服务端状态文案「已监听（等待客户端）」
- 构建：CommHandler Release + CA Debug PASS；runtime/exe 旁 DLL 已更新

## 13. 续：全通讯方式连通校验（协议无关）

- 日期：2026-07-21
- 范围：Native + Legacy；串口 / TCP 客户端 / TCP 服务端 / UDP；所有协议共用传输层判定
- Native：TCP 客户端 ConnectedState；串口 isOpen；UDP BoundState；TCP 服务端 isListening
- Legacy：TCP 客户端 5s+状态复核；UDP 等 bind 结果；串口 isOpen；Opening 断连失败收尾
- UI/日志：已连接 / 已监听 / 已绑定 分语义；协议 index 不参与连通判定
- 构建：CommHandler Release + CA/CoreSelfTest Debug PASS

## 14. 续：乱码根因 / 全协议未解析 / 侧栏几何稳定

- 日期：2026-07-21
- 用户证据：联恒串口/网口、中机网口对端文本乱码；侧栏切协议跳动；三思发送疑义；未匹配收包静默

### 14.1 乱码根因（对齐 DLL）

| 协议 | 根因 | 最优处理 |
|---|---|---|
| 联恒串口5 / 网口8 | 线帧为 `AA 55` 二进制，非 CSV ASCII | 对端开十六进制；助手能力区明示二进制；TX 只展示入参 |
| 中机网口2 | 线帧为 packed `ServerData` 二进制；且曾经 QString 路径有损坏风险 | 二进制 `sendData(char*,len)`；对端十六进制核 `04`+17B |
| 三思串口0 | ASCII 定宽+`0D`，文本窗应可读 | 与网口三思区分；能力提示 ASCII |
| 三思网口3 | `SendData(vector)` **无 case 3**，不写出线帧 | 能力关闭 + 边界日志；不在助手私自补帧 |

### 14.2 未解析 RX 扩展

- 网口：中机/三思长度不匹配；JSON 非对象；万测未匹配控制；触发存图无 `\r\n`；威盛解析失败
- 串口：三思拼帧完成后无数值；时代新材段不符；IEEE 非控制字等
- 文档：`docs/LEGACY_PROTOCOL_DEV_GUIDE.md`、`docs/LEGACY_FULL_FLOW_TEST_MANUAL.md`

### 14.3 UI 稳定性

- `connMethodStack`：「连接方式」Native=传输类型 / Legacy=网口·串口，同高切换
- Legacy 页去掉重复「通信类型」；端点全宽，表单标签左对齐、标签列宽统一
- 能力区下移至载荷提示下方并加高；广播/客户端区暂隐藏；`sideStretch` 收紧
- `paramStack` 仅 minHeight，禁止 maxHeight 压扁

## 15. 续：人机交互文案专业化与交付文档

- 日期：2026-07-21
- 界面/日志/能力提示改为书面语；术语统一为「原生通道 / 兼容动态库」
- 交付文档：`docs/COMM_TEST_CASE_MANUAL.md`、`docs/COMMUNICATION_ASSISTANT_DEV_GUIDE.md`；协议说明重写
- 构建：CommunicationAssistant / CoreSelfTest Debug PASS
