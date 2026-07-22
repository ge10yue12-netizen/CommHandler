# CommHandler 动态库变更说明（相对最初实现）

| 项 | 内容 |
|---|---|
| 文档目的 | 对照「最初动态库」与「当前动态库」，说明新增能力与优化点 |
| 对比基线（最初） | CommHandler 嵌套备份 Git `a82b2ad`（2026-07-09，`SSCK 串口通讯新增自动存图功能`） |
| 当前范围 | 工作区内 `CommHandler/` 源码（截至 2026-07-22 联调态） |
| 基线版本 | CommunicationAssistant / 文档体系 **V1.2.5** |
| 规模（相对最初） | 约 **9 个文件，+461 / −98 行**（核心通讯路径） |
| 纪律 | 默认 DLL 冻结；已批准例外见 `docs/BASELINE.md`（联恒索引重开及后续联调必要最小改动） |

---

## 1. 结论摘要

相对最初实现，当前动态库主要做了三件事：

1. **新增协议入口**：重开联恒光科——串口 `iProtocolType=5`、网口 `iProtoType=8`（不占用历史索引 `0`）。
2. **新增可观测通道**：`emitUnparsedRx`、可选参数 `bAssistObserve`、协议线发 ACK 事件 `W_CUSTOM_COMM_PROTO_ACK`，供通信助手联调；正式软件不设观测开关时，语义尽量与旧库一致。
3. **优化连通与线帧正确性**：TCP/UDP/串口「真正就绪」判定、中机/科新/纳百川等线帧与控制路径修复，消除一批「空报已连接 / 静默丢包 / 乱码」问题。

**未做**：整体重构其它历史协议语义；未改 Voltage/Pulse；未把助手私自组帧塞进 DLL 之外的旁路协议。

---

## 2. 对比范围与方法

| 对比对象 | 路径 / 标识 |
|---|---|
| 最初动态库源码快照 | `CommHandler/.git_nested_bak` → `HEAD = a82b2ad` |
| 当前动态库源码 | `CommHandler/CommHandler.*`、`CommSdk/SerialPortComm.*`、`CommSdk/SocketComm.*`、`CommSdk/network/tcpsocket.cpp`、`udpsocket.cpp`、`Common/UIDef.h` |
| 审计证据 | `docs/reports/CA-M2-UX-LHGK-001.md`、`CA-M2-DEBUG-ASSIST-001.md`、`CA-M2-LEGACY-RX-HOTCFG-001-SELFTEST.md` |
| 协议与用例 | `docs/LEGACY_PROTOCOL_DEV_GUIDE.md`、`docs/COMM_TEST_CASE_MANUAL.md`、`docs/fixtures/comm/` |

对比方式：以嵌套 Git 快照为「最初」，对上述文件做 `diff`，再按审计报告归类为「新增」与「优化」。

---

## 3. 涉及文件一览

| 文件 | 相对最初的主要变化 |
|---|---|
| `CommHandler.h` / `.cpp` | 新增并转发 `emitUnparsedRx` |
| `CommSdk/SerialPortComm.h` / `.cpp` | 联恒 case 5、拼帧缓冲、未解析上报、科新头/控制、开闭口判定 |
| `CommSdk/SocketComm.h` / `.cpp` | 联恒 case 8、拼帧、`bAssistObserve`、ACK 观测、中机直发、纳百川/多协议未解析 |
| `CommSdk/network/tcpsocket.cpp` | 连接等待与 `ConnectedState` 复核、生命周期 |
| `CommSdk/network/udpsocket.cpp` | bind 结果等待，禁止空报成功 |
| `Common/UIDef.h` | 协议索引注释对齐；新增 `W_CUSTOM_COMM_PROTO_ACK` |

协议编解码本体 `SerLhgkProtocol` / `NetLhgkProtocol` 在最初快照中已存在；本次是**把已有联恒实现挂到可用协议索引**，并补上 TCP/串口流式拼帧与失败可观测。

---

## 4. 新增能力（相对最初）

### 4.1 联恒光科协议索引重开（V1.2.5 批准例外）

| 通道 | 参数键 | 新索引 | 说明 |
|---|---|---|---|
| 串口 | `iProtocolType` | **5** | 发送：至少力、温两路 → `buildExternalDataFrame`；接收：联恒小端流式拼帧 |
| 网口 | `iProtoType` | **8** | 同上；错误帧类型等与网口联恒约定一致 |

设计约束（相对「把联恒挂回 0」）：

- 串口 `0` 仍为三思；网口 `0` 仍为 JSON。
- 联恒**不得**再占历史 `case 0`，避免与现有协议冲突。
- 线格式事实见 `docs/BASELINE.md` §14.4；无设备 Golden 时不得宣称现场验收通过。

### 4.2 未解析接收信号 `emitUnparsedRx`

| 项 | 内容 |
|---|---|
| 公共 API | `CommHandler` 信号：`void emitUnparsedRx(const QByteArray& raw);` |
| 来源 | `SerialPortComm` / `SocketComm` → `CommHandler` 槽转发 |
| 用途 | 库内部「只回调已解析业务」时，宿主仍能看见「线上有字节但解不出」 |
| 典型触发 | 协议 `default`；长度不匹配；联恒**完整坏帧**；无法对齐的噪声；纳百川未识别载荷等 |

联恒特判：

| 状态 | 行为 |
|---|---|
| 断帧（Incomplete） | 缓冲等待，**不**报 `emitUnparsedRx` |
| 完整且校验通过 | 走业务回调（`handleFrame`） |
| 完整但坏帧 / 噪声 | 报 `emitUnparsedRx` |
| 缓冲 > 64KiB | 截断上报并清空，防止 OOM |

### 4.3 助手观测开关 `bAssistObserve`

| 项 | 内容 |
|---|---|
| 参数 | `setParameter("bAssistObserve", bool)` / `getParameter` |
| 默认 | `false` |
| 语义 | 正式软件不设 → 观测类上报关闭，控制态机与正式一致；助手联调开库后可显式设为 `true` |

网口侧未解析多经 `emitAssistUnparsedRx`：仅在 `bAssistObserve==true` 时真正 `emit emitUnparsedRx`。

### 4.4 协议线发 ACK 观测 `W_CUSTOM_COMM_PROTO_ACK`

| 项 | 内容 |
|---|---|
| 宏 | `W_CUSTOM_COMM_PROTO_ACK`（`W_CUSTOM + 14`） |
| 触发 | 如触发存图等路径写出 JSON ACK 时，经 `notifyWireTxAck(...)` |
| 载荷 | `ackCode` / `ackMessage` / `tn` / `wireTx` / `wireDirection=tx` |
| 门控 | 仅 `bAssistObserve==true` |

用于助手侧核对「库实际写出的 ACK 线文」，不改变正式软件默认路径。

### 4.5 联恒流式拼帧 API（内部）

新增内部方法与成员（串口 / 网口对称）：

- `processLhgkRxStream(const QByteArray& chunk)`
- `m_lhgkRxBuffer`（与其它协议缓冲分用，避免串扰）
- 依赖协议层 `probeCompleteFrameSize`
- 切换 `iProtocolType` / `iProtoType` 时清空缓冲，避免旧半包污染

最初实现多为「整包 `readAll`/`onRecv` 一次当一帧」；当前对 TCP/串口流补了半包/粘包处理。

### 4.6 科新控制事件上报（串口）

最初：控制字匹配后主要置内部标志（如 `bHexSetZero`），助手侧看不到开始/停止。

当前：匹配成功后按控制位发出：

- `[1]` → `W_CUSTOM_COMM_STARTCALC`
- `[2]` → `W_CUSTOM_COMM_STOPCALC`

并修正拼帧分支误用 `m_sInputData` 的问题，改为正确使用 `m_sCtrlData`。

---

## 5. 优化与修复（相对最初）

### 5.1 连通判定：避免「假已连接」

| 传输 | 最初问题 | 当前行为 |
|---|---|---|
| TCP 客户端 | 等待偏短/弱；易误报成功 | 参数校验；`waitForConnected(5000)`；复核 `ConnectedState`；失败 `abort` |
| UDP | 曾接近恒 `true` | 启动线程后等待 bind 结果（约 3s）；失败则关闭并返回 `false` |
| 串口 | 仅信 `bComOpened` | `bComOpened \|\| isOpen()`；`open` 后复核 `isOpen`；关闭时同步清标志 |
| TCP socket 生命周期 | 裸 `new`/`delete` 风险 | `QTcpSocket(this)` 挂对象树 |

网口 `Connect` 成功后写回 `m_socketInfo.bConntction = bFlag`，与真实结果一致。

### 5.2 线帧正确性（乱码 / 毁帧）

| 协议 | 问题 | 优化 |
|---|---|---|
| 中机网口 | 经 `QString`/`toLatin1` 路径易损坏二进制 `ServerData` | 定长结构二进制 `sendData(char*, len)` 直发；入参须恰好 2 路 |
| 科新串口 | `FixedDoubleToHex` 未写前两字节 → 堆垃圾头（如 `2D F2`） | 固定写入 `0x02, 0x4C`，与收端/`MachinePeer` 黄金帧对齐 |
| 三思网口结构拷贝 | `memcpy` 长度误用 | 按 `sizeof(Data)` 拷贝 |
| 文本/JSON 收包 | 尾部 CRLF/空白导致静默失败 | `trimmed()` 后再解析；`find("\r\n")` 改为 `!= npos` |

说明：联恒/中机线帧本身是二进制；对端用「纯文本窗」会显示乱码——属协议形态，不是再改一层转义；助手侧用十六进制与能力说明对齐。

### 5.3 静默丢包与可观测性

| 场景 | 最初 | 当前 |
|---|---|---|
| 纳百川线条/自动化 | 严格字节相等、未 trim；采集中再 `calcStart` 直接 return；未识别不回调 | trim；重复开始仍可观测；未识别 → 未解析上报 |
| 多协议解析失败 | 常常无任何信号 | 补 `emitUnparsedRx` / `emitAssistUnparsedRx`（网口受观测开关约束） |
| 串口无数值且非已知控制字 | 易静默 | 在非组合态上报原始字节 |

### 5.4 其它稳健性

- 协议切换清空联恒拼帧缓冲。
- 联恒接收缓冲上限 64KiB，溢出截断上报。
- UDP 非法地址/端口直接失败；重复 `initConnection` 防护。
- TCP 断开等待与必要时 `abort`，减少半开连接。

---

## 6. 协议索引对照（注释对齐）

### 网口 `iProtoType`

| 索引 | 含义（当前注释） |
|---|---|
| 0 | JSON |
| 1 | 万测 |
| 2 | 中机 |
| 3 | 三思 |
| 4 | 触发存图 |
| 5 | 威盛 |
| 6 | 纳百川自动化 |
| 7 | 纳百川线条 |
| **8** | **联恒光科（新增可用入口）** |

### 串口 `iProtocolType`

| 索引 | 含义（当前注释） |
|---|---|
| 0 | 三思 |
| 1 | 科新 |
| 2 | 时代新材 |
| 3 | IEEE |
| 4 | 冠腾 |
| **5** | **联恒光科（新增可用入口）** |

历史注释曾把联恒写在网口「协议一」位置；实现上联恒挂在 **8 / 5**，与 V1.2.5 能力表一致。

---

## 7. 对正式软件与助手的影响边界

| 角色 | 影响 |
|---|---|
| 正式宿主（不设 `bAssistObserve`） | 观测类信号默认关闭；既有 0–4 / 0–7 索引语义原则上保持；连通与线帧修复会改善真实联通与部分协议正确性 |
| 通信助手（联调） | 可开观测；能看见未解析 RX、部分 ACK；联恒 5/8 可测；须部署与导入库同源的 Release DLL，避免 Debug/Release CRT 混用 |
| 能力表 / 文档 | 须与 DLL 同步；用例与黄金帧见测试手册与 `docs/fixtures/comm/` |

---

## 8. 变更追溯（审计编号）

| 编号 | 主题 | 与 DLL 的关系 |
|---|---|---|
| CA-M2-UX-LHGK-001 | 联恒索引重开、能力表、基线 V1.2.5 | 串口 5 / 网口 8 收发接通 |
| CA-M2-DEBUG-ASSIST-001 | 未解析可观测、联恒拼帧、连通判定、观测门控 | `emitUnparsedRx`、`bAssistObserve`、TCP/UDP/串口就绪 |
| CA-M2-LEGACY-RX-HOTCFG-001 | 纳百川静默、科新头与控制字、热重连（助手侧） | 科新/`paeseNaBaiChuan*` 等 DLL 修复 |

---

## 9. 明确未纳入 / 仍受限

- Voltage / Pulse 未纳入本次「相对最初」联调优化范围。
- 网口三思：`SendData(vector)` 仍无写出分支（能力表关闭），助手不得私自补帧。
- 联恒缺真实设备 Golden 时，不得宣称现场验收通过。
- 部分协议本身只解析 1 路数值（如科新、时代新材）——属协议定义，不是 UI 丢通道。

---

## 10. 建议回归（改 DLL 后）

按 `docs/COMM_TEST_CASE_MANUAL.md` 优先级至少覆盖：

1. DLL smoke：configure / open / 连通真假。
2. 联恒网口：`TC-NET-8` TX/RX/CRC（含断帧不误报）。
3. 联恒串口：`TC-SER-5` 环回与 CRC。
4. 科新头字节与控制字开始/停止。
5. 纳百川线条 ASCII/HEX `calcStart` 与未知载荷未解析。
6. 中机二进制发送对端十六进制核对。

---

## 11. 修订记录

| 日期 | 说明 |
|---|---|
| 2026-07-22 | 首版：相对嵌套备份 `a82b2ad` 的新增与优化汇总 |
