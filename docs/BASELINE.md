# CommunicationAssistant 开发基线与执行约束

> 文档版本：V1.2.4  
> 基线状态：**正式可对齐基线**；满足 §21 开工门槛后方可开始 M1 建工程  
> 适用对象：开发者、AI 编码助手、代码审查与测试  
> 当前阶段：动态库冻结，原生通信助手优先；V1.2 为正式可对齐基线，**满足 §21 后方可建工程**  
> 稳定路径：`docs/BASELINE.md`（版本号写在文内，不写在文件名）

---

## 0. 每次开工口令

```text
请先读取 docs/BASELINE.md、docs/CODE_STYLE.md 与 docs/AUDIT_SPEC.md，
创建变更编号并填写变更对齐单；
在用户回复「确认修改」并确认范围后再实施修改；
完成后按 CODE_STYLE 完成自检自测并生成自检自测报告，
再生成代码变更审计报告；
没有构建或测试证据的项目不得标记为通过。
```

口令全文与 `docs/AUDIT_SPEC.md` §0「标准开工指令」保持一致；门禁口令与最终 PASS 判定以审计规范为准。

配套关系：

- `docs/BASELINE.md`：能做什么 / 不能突破什么（本文）
- `docs/CODE_STYLE.md`：怎么写代码与注释 / 修改者如何自检自测
- `docs/AUDIT_SPEC.md`：怎么改文件（门禁）/ 怎么证明 / 能否通过
- 门禁口令：**确认修改**（无此口令零文件改动；「可以/开始吧」等无效）

文档冲突裁决顺序（`[DECISION]`，与 `CODE_STYLE.md` §1 一致）：

```text
最新开发基线
→ 代码编写/注释规范（CODE_STYLE 第一、二部分）
→ 自检自测规范（CODE_STYLE 第三部分）
→ 代码审计规范
→ 单次变更对齐单
```

**当前禁止**：在未填写对齐单、未获「确认修改」时改文件或创建 CommunicationAssistant 工程。工具链见 §21.1；环境核验附录见 `AUDIT_SPEC.md` §19。

---

## 1. 文档用途

本文档是 CommunicationAssistant 的唯一总体开发基线，用于约束每次设计、编码、重构和测试任务。

它解决以下问题：

1. 防止未验证信息被当成仓库事实。
2. 防止开发过程中持续扩大产品范围。
3. 防止 UI、Transport、Session 和旧 DLL 再次直接耦合。
4. 统一术语、接口语义、里程碑和验收标准。
5. 让每次 AI 辅助开发都有明确输入、输出和停止条件。

本文档不能替代：

- 阅读实际源码；
- 编译和单元测试；
- 串口、网络和真实设备验证；
- 协议文档与真实 Golden 报文；
- 性能、断线和长时间运行测试。

因此，对齐本文档可以显著降低幻觉和方向漂移，但不能保证实现天然正确。

---

## 2. 信息可信度标记

| 标记 | 含义 | 使用要求 |
|---|---|---|
| `[VERIFIED]` | 已由当前源码、构建结果或测试确认 | 必须能够指出证据 |
| `[DECISION]` | 已冻结的产品或架构决定 | 修改必须进入变更记录 |
| `[ASSUMPTION]` | 为推进工作暂时采用的假设 | 必须说明验证方法 |
| `[UNVERIFIED]` | 尚未读取或验证，不得当事实引用 | 不挡当前里程碑时可继续，但禁止承诺复用 |
| `[BLOCKED]` | 缺少文档、源码、设备或权限，不能安全继续该项实现 | 不得自行猜测实现 |
| `[DEFERRED]` | 明确延后，不属于当前里程碑 | 不得顺手加入 |

禁止使用“应该存在”“大概支持”“仓库里可能有”等表述后直接编码。无法确认时必须标记为 `[ASSUMPTION]`、`[UNVERIFIED]` 或 `[BLOCKED]`。

---

## 3. 当前已知事实

### 3.1 已审计源码

`[VERIFIED]` 当前能力表与 Legacy 约束的审计依据为以下十个**历史上传文件**（成对 `.cpp/.h`）。  
**证据范围**：某次上传包审计；**不自动等于**本 git 仓库树中一定存在或路径一致。本仓库若另有 CommLab 等工程，在未按本文件要求重新核读前仍属 `[UNVERIFIED]`。

- `NetLhgkProtocol.cpp/.h`
- `SerLhgkProtocol.cpp/.h`
- `CommHandler.cpp/.h`
- `SerialPortComm.cpp/.h`
- `SocketComm.cpp/.h`

### 3.2 尚未验证、但不阻塞 M1 的资产

以下标记为 `[UNVERIFIED]`：不得作为基线事实或“可复用”承诺，但 **不阻止 M1 原生助手开工（在门槛满足后）**：

- CommLab 是否存在以及当前实现情况；
- CommController 是否已有安全复制逻辑；
- MachinePeer 是否存在以及是否可复用；
- ProtoGuide 是否存在以及协议表是否准确；
- NetworkBox 的完整线程、多客户端和消息边界语义；
- 完整 CommHandler 工程能否在当前环境构建。

### 3.3 阻塞特定实现的项

以下标记为 `[BLOCKED]`，缺证据前不得实现对应能力：

- VoltageComm、PulseComm 的 `emitNewData` 元素类型（未验证前不开放）；
- 联恒协议的正式线格式（无文档/Golden 前不得猜测）；
- 多 `CommHandler` 实例是否线程/资源安全（首版不做多 Legacy）。

---

## 4. 冻结原则

| 原则 | 基线决定 |
|---|---|
| DLL 冻结 | `[DECISION]` 当前阶段不修改 CommHandler 及其内部协议实现 |
| 原生为主 | `[DECISION]` 通用助手能力由新软件自行实现 |
| DLL 为辅 | `[DECISION]` DLL 仅用于旧协议兼容联调 |
| 能力显式 | `[DECISION]` 不确定的功能默认不开放；实现层能力仅「支持/不支持」 |
| 单 Legacy | `[DECISION]` 首版最多一个活动 LegacySession（`activeLegacySessionId`） |
| 接口先行 | `[DECISION]` UI 只依赖 ICommSession |
| 配置强类型 | `[DECISION]` SessionConfig + TransportConfig；ResourceClaim 由 SessionManager 推导 |
| 原始优先 | `[DECISION]` Native 模式保存原始字节，解析结果是派生数据 |
| 资产待验证 | `[DECISION]` 未实际读取的工程或类不得承诺复用 |
| 小步交付 | `[DECISION]` 一个功能必须同时具备实现、错误路径和测试 |

---

## 5. 产品范围

### 5.1 跨里程碑产品愿景

建设一个 Windows Qt 桌面通信助手，支持：

- Serial、TCP Client、TCP Server、UDP；
- Raw HEX/文本收发；
- 单次、周期和列表轮询发送；
- **抓包落盘**（M1）；CaptureReader、重新解析与基础回放（M4）；
- 旧 CommHandler 动态库兼容模式（M2）；
- 模拟器、波形（M3）与按需 Codec（M4）。

`[DECISION]` **§5.1 为跨里程碑愿景；当前编码范围以 §17 所属里程碑为准。**  
`[DECISION]` **M1 不创建回放占位页面。**

### 5.2 当前非目标

以下内容为 `[DEFERRED]`：

- 修改或重构现有 CommHandler DLL；
- Protocol Host 独立进程；
- Protobuf IPC；
- Lua 脚本系统；
- 通用协议可视化 IDE；
- 自动猜测完整协议；
- 一次重写全部旧协议；
- 插件市场或在线协议包；
- 分布式压力测试；
- 微秒级硬实时发送承诺；
- 在 M1 阶段进行复杂 UI 皮肤开发；
- M1 自动重连（`SessionState::Reconnecting` 枚举可保留，实现路径延后）；
- M1 任何 Codec / ProtocolFrame 路径；
- M1 回放 UI 或 CaptureReader。

---

## 6. 总体架构

```text
UI
└─ Application Services
   ├─ SessionManager
   │  ├─ NativeSession      （M1：Tool）
   │  │  ├─ Transport
   │  │  └─ Raw Record
   │  ├─ LegacySession      （M2：Tool；M1 占位且 UI 隐藏）
   │  └─ SimulatorSession   （M3：DeviceSimulator）
   ├─ SendScheduler
   └─ CaptureManager
      └─ CaptureWriter per Session
```

`[DECISION]` `SendScheduler` 与 `CaptureManager` 为应用服务：不持有 Transport 或 CommHandler；Scheduler 只通过 `ICommSession::send()` 与发送生命周期记录工作。

三种工作模式：

```cpp
enum class WorkMode
{
    Native,
    LegacyDll,
    Simulator
};
```

### 6.1 SessionRole 归属（冻结）

```text
M1：NativeSession  → Tool
M2：LegacySession  → Tool
M3：SimulatorSession → DeviceSimulator
```

`[DECISION]` V1 阶段 **不允许** `NativeSession + DeviceSimulator`，避免两个模拟入口。

Transport 不判断业务角色。SimulatorSession 可选用 TCP Client、TCP Server、UDP 或 Serial，不固定为服务器。

### 6.2 Codec 出现时机

- **M1**：架构与代码路径中 **彻底移除** Codec。
- **M4**：NativeSession 才增加 Optional Codec；TCP Server「每 Channel 独立 Codec 缓存」亦属 M4。

---

## 7. 统一会话契约

```cpp
class ICommSession : public QObject
{
    Q_OBJECT

public:
    explicit ICommSession(QObject* parent = nullptr);
    ~ICommSession() override = default;

    // 仅 Created 或 Closed 允许配置；完成校验并保存不可变配置快照。
    virtual Result configure(const SessionConfig& config) = 0;

    // 返回请求是否被接受，不表示异步操作已经完成。
    // open() 不接受连接参数。
    virtual Result open() = 0;
    virtual Result close() = 0;
    virtual Result send(const SendRequest& request) = 0;

    virtual SessionState state() const = 0;
    virtual QUuid sessionId() const = 0;

signals:
    void recordReceived(const CommRecord& record);
    void stateChanged(SessionState state);
    void errorOccurred(const CommError& error);
};
```

状态基线：

```cpp
enum class SessionState
{
    Created,
    Opening,
    Connected,
    Closing,
    Closed,
    Reconnecting,   // [DEFERRED] M1 不实现自动重连进入路径
    Unresponsive,
    Faulted
};
```

语义：

- `configure()`：仅 `Created`/`Closed`；打开期间禁止改配置；UI 不直接构造 `ResourceClaim`。
- `open()` 成功 = 打开请求已接受；真正连接成功由 `stateChanged(Connected)` 表示。
- `send()` 成功 = 发送请求已接受；不能直接显示成对端已收到。
- `close()` 成功 = 关闭请求已接受；最终关闭由 `stateChanged(Closed)` 表示。
- UI 禁止直接持有或调用 `QSerialPort`、Socket 或 `CommHandler`。

---

## 7.1 SessionConfig 与 ResourceClaim 推导

`[DECISION]` 调用方只填强类型配置；`ResourceClaim` 由 SessionManager 统一计算，避免双源矛盾。

```cpp
struct SessionConfig
{
    QUuid sessionId;
    QString name;
    WorkMode mode;
    SessionRole role;
    TransportConfig transport;  // 变体：见下
};

// TransportConfig 为以下之一（C++14：用带 kind 判别的结构体 / 继承，不用 std::variant）：
//   SerialConfig
//   TcpClientConfig
//   TcpServerConfig
//   UdpConfig
//   LegacyConfig   // 仅 M2
```

```cpp
ResourceClaim claimFor(const SessionConfig& config);
```

```cpp
struct SerialConfig
{
    QString portName;      // 原始输入；规范化在 claimFor 内完成
    int baudRate = 115200;
    int dataBits = 8;
    QString parity;        // "none"/"even"/"odd"/...
    QString stopBits;      // "1"/"1.5"/"2"
};

struct TcpClientConfig
{
    QString remoteAddress;
    quint16 remotePort = 0;
    QString localAddress;  // 可选
    quint16 localPort = 0; // 可选，0=系统分配
};

struct TcpServerConfig
{
    QString localAddress;  // 可含 0.0.0.0 / ::
    quint16 localPort = 0;
    bool addressReuse = false;
};

struct UdpConfig
{
    QString localAddress;
    quint16 localPort = 0;
    QString remoteAddress; // 可选默认对端
    quint16 remotePort = 0;
    bool addressReuse = false;
};

struct LegacyConfig
{
    // M2：通信类型、协议索引、以及 DLL setParameter 所需字段
    // 具体字段在进入 M2 时按能力表补全，M1 不实现
};
```

---

## 7.2 M1 最小类型契约

`[DECISION]` 下列类型为最小契约；字段可增，不得 silently 改语义；持久化变更见 §20。

```cpp
enum class TransportKind
{
    Serial,
    TcpClient,
    TcpServer,
    Udp
};

enum class SessionRole
{
    Tool,
    DeviceSimulator
};

enum class Direction
{
    Tx,
    Rx,
    System
};

struct Endpoint
{
    QString address;
    quint16 port = 0;
};

struct Result
{
    bool ok = false;
    QString code;
    QString message;
};

struct CommError
{
    QString code;
    QString message;
    QString sessionId;
    QString channelId;
    QUuid requestId;   // 可空
};

struct SendRequest
{
    QUuid requestId;           // 必填；同一次发送生命周期共享
    QUuid taskId;              // 非 Scheduler 发送时可为空
    QUuid sessionId;
    QString channelId;         // TCP Server 选中客户端；空=默认策略
    QByteArray payload;        // HEX/文本最终都转为字节
    bool broadcast = false;    // 仅 TcpServer
    QVariantMap attributes;
};
```

---

## 8. 资源声明与互斥

```cpp
struct ResourceClaim
{
    TransportKind transport;

    QString serialPort;        // 已规范化

    QString localAddress;
    quint16 localPort = 0;

    QString remoteAddress;
    quint16 remotePort = 0;

    bool exclusive = true;
    bool addressReuse = false;
};
```

内部互斥规则（Transport）：

| Transport | 内部冲突判断 |
|---|---|
| Serial | 规范化后的串口名称相同 |
| TCP Server | 本地监听地址和端口冲突（含通配） |
| TCP Client | 默认不因远端地址相同而冲突 |
| UDP | 本地绑定冲突；`addressReuse` 只表示允许尝试 |

### 8.1 Legacy 互斥（不属于 TransportKind）

`[DECISION]` Legacy **不是** Transport。M2 首版由 SessionManager 维护：

```cpp
// C++14：不用 std::optional
bool hasActiveLegacySession = false;
QUuid activeLegacySessionId;
```

同一时刻最多一个活动 LegacySession。不得把 Legacy 伪装成 `TransportKind` 枚举值。

补充规则：

- `COM3`、`com3`、`\\.\COM3` 在 Windows 上规范化为同一资源。
- `0.0.0.0:8000` 与任一具体 IPv4 的 `8000` 监听冲突。
- `[::]:8000` 与对应具体 IPv6 的 `8000` 监听冲突。
- `addressReuse` 只表示允许尝试，不保证 OS 接受。
- SessionManager 只防本软件内部冲突；最终以系统 `open()`/`bind()`/`listen()` 为准。

---

## 9. 原始记录与协议帧

```cpp
enum class RecordKind
{
    RawChunk,
    UdpDatagram,
    ProtocolFrame,          // M4 起；M1 不得产生
    LegacyValueEvent,
    LegacyControlEvent,
    LegacyParameterEvent,
    ConnectionEvent,
    ErrorEvent
};

enum class RecordStatus
{
    Observed,
    Queued,
    Submitted,
    Completed,
    Failed,
    Cancelled
};

struct CommRecord
{
    QUuid sessionId;
    QString channelId;
    quint64 sequence = 0;

    QUuid requestId;         // 顶层字段；接收类记录可为空
    QUuid taskId;            // 顶层字段；可为空

    RecordKind kind;
    RecordStatus status;
    Direction direction;

    QDateTime wallTime;
    qint64 monotonicNs = 0;

    QByteArray bytes;
    QVariantMap attributes;
    QString summary;

    Endpoint localEndpoint;
    Endpoint remoteEndpoint;

    QString errorCode;
    QString errorMessage;
};
```

### 9.1 RecordStatus 适用规则

| Record 类型 | 默认状态 |
|---|---|
| 接收 RawChunk | `Observed` |
| 接收 UdpDatagram | `Observed` |
| ProtocolFrame（M4） | `Observed` |
| Legacy 接收事件 | `Observed` |
| ConnectionEvent | `Observed` |
| ErrorEvent | `Observed` |
| 发送请求入队 | `Queued` |
| 提交 Transport | `Submitted` |
| 本地确认完成（可选） | `Completed` |
| 失败 / 取消 | `Failed` / `Cancelled` |

硬性规则：

- TCP/串口一次系统读取是 `RawChunk`，不是协议帧。
- UDP 一次接收是 `UdpDatagram`。
- **M1 不得产生 `ProtocolFrame`。**
- Legacy 无原始字节时只能产生 `Legacy*Event`，不得冒充 Raw 抓包。
- `summary` 只用于显示；结构化数据放入 `attributes`（且不得覆盖顶层保留字段）。
- 同一次发送的 `Queued`/`Submitted`/`Failed`/`Cancelled`/`Completed` 共享同一 `requestId`。

### 9.2 Error 双通道

`[DECISION]` `ErrorEvent` 是权威记录；`errorOccurred` 是 UI 即时通知。

```text
创建 CommError
→ 生成并发出 ErrorEvent（经 recordReceived）
→ 发出 errorOccurred
```

约束：

- Capture **只**监听 `recordReceived`。
- UI 通知系统监听 `errorOccurred`。
- 接收列表监听 `recordReceived`。
- `errorOccurred` **不得**再次生成 ErrorEvent。

---

## 10. TCP Server Channel

每个 TCP 客户端必须拥有独立 Channel：

```text
channelId
socketDescriptor
remoteEndpoint
receiveBuffer
连接时间
收发字节统计
最后活动时间
```

**M1 必须支持：**

- 发送给选中客户端；
- 广播发送；
- 断开选中客户端；
- 在 CommRecord 中记录 channelId；
- 每 Channel 独立 Socket 状态、统计与 Raw 记录。

**M4 才要求：** 不同客户端的 Codec 缓存互不共享。

---

## 11. CaptureManager 与 CaptureWriter

### 11.1 组织方式

`[DECISION]`

```text
一个 CaptureManager
→ 每个活动 Session 一个 CaptureWriter
→ 每个 Session 独立文件与有界队列
```

单 Session 写失败不污染其他会话；文件以 Session 为边界。

### 11.2 文件格式（M1 冻结：JSON Lines）

扩展名示例：`session-name_时间_sessionId.clog.jsonl`

- 第一行：文件头 JSON 对象。
- 其后每行：一条记录 JSON 对象。
- 枚举：稳定 **英文字符串**。
- UUID：小写、无花括号。
- 64 位序号与纳秒：十进制 **字符串**，避免 JSON 消费者精度丢失。
- 字节：标准 Base64，字段名 `bytesBase64`。
- **顶层字段优先**；`attributes` 不得覆盖保留字段；`attributes` 仅存类型特有数据。

文件头至少包含：`recordType=fileHeader`、`formatVersion`（当前 `"1"`）、`sessionId`、`createdAtUtc`、`magic`（如 `"CAREC01"`）。

每条记录必选顶层字段：

```text
recordType          // "record"
formatVersion
sessionId
sequence            // 十进制字符串
kind
direction
status
wallTimeUtc
monotonicNs         // 十进制字符串
channelId
requestId           // 可空字符串
taskId              // 可空字符串
bytesBase64
attributes
errorCode
errorMessage
```

异常退出：尾部不完整行丢弃；不得因尾部损坏拒绝此前完整行。

### 11.3 运行要求

- 后台写入；每 Writer 有界队列；
- 写失败 / 磁盘不足明确错误；
- UI 清空显示不清除抓包文件；
- Native / Legacy 使用正确 RecordKind；
- M1 不实现 CaptureReader / 回放页。

---

## 12. SendScheduler（应用服务）

### 12.1 周期锚点

`[DECISION]` M1 周期锚点为 **Submitted**（不是 Completed，也不是 send() Accepted）。

```text
Queued
→ Transport 接受数据
→ Submitted
→ 等待 interval
→ 下一次发送
```

不选 Completed 的原因：各 Transport「完成」语义不一致（TCP 无法表示对端收到；`write` 多表示进入本地缓冲；串口物理发送与驱动接受不同；UDP 通常只能确认本地提交）。

`Completed` 保留为可选本地完成状态；**Scheduler 不依赖它**。

若进入 `Failed`：

- 默认 **停止该任务**；
- 记录失败原因；
- **不**无限重试。

### 12.2 M1 功能范围

固定延迟模型；支持：单次、固定延迟周期、指定次数、无限循环、列表顺序轮询、暂停/继续/停止/全部停止。

不实现：绝对频率、延迟追赶、响应驱动、复杂并行、硬实时。

不为每个周期任务建线程；不依赖大量 UI Timer。

M3 波形路径：

```text
WaveformGenerator → SendScheduler → NativeSession 或 LegacySession
```

---

## 13. Legacy 模式约束（M2；M1 不链接）

### 13.1 生命周期

```text
启动 LegacyWorker 线程
→ 在线程中创建 CommHandler
→ 在线程中执行 Connect/Disconnect/SendData/setParameter
→ 正常退出时在线程中删除 CommHandler
```

禁止在 UI 线程创建 `CommHandler` 再 `moveToThread`。

### 13.2 裸指针事件

```text
DLL emitNewData
→ DirectConnection：边界检查并立即复制
→ 拥有所有权的 Legacy 事件
→ QueuedConnection 到应用层
```

Direct 槽禁止 UI / 写盘 / 复杂解析。`size` 必须 `>0` 且 `<= kMaxLegacyValues`。Voltage/Pulse 未验证前不开放。

### 13.3 超时与 Watchdog

Watchdog 在 Worker **之外**。超时 = 调用方停止等待 ≠ 调用已取消。

超时后：`Unresponsive`；禁新命令；不 terminate 线程；不销毁可能仍在跑的对象；不向阻塞 Worker 排 `close()`；提示重启。

---

## 14. Legacy 能力基线

```cpp
enum class LegacyCapability
{
    ReceiveValues,
    ReceiveControlEvents,
    ReceiveParameterEvents,
    SendEncodedValues,
    SendTransparentText,
    RequiresPollingPermission,
    RequiresStreamingState,
    RawReceive,   // 恒不支持
    RawSend       // 恒不支持
};
```

### 14.1 实现层二元状态

`[DECISION]` 运行时能力表 **只允许「支持 / 不支持」**。  
原「待验证 / 部分 / 默认关闭」一律映射为 **不支持**，细节写入：

```cpp
QString limitation;
QString verificationNote;
```

验证通过后须经基线变更才能开放。

### 14.2 网口（实现层）

| 协议 | 数值接收 | 控制/参数事件 | 协议数值发送 | 透明文本发送 |
|---|:---:|:---:|:---:|:---:|
| 0 JSON | 不支持 | 支持 | 支持 | 支持 |
| 1 万测 | 不支持 | 支持 | 支持 | 支持 |
| 2 中机 | 支持 | 支持 | 支持 | 不支持 |
| 3 三思 | 支持 | 不支持 | 不支持 | 不支持 |
| 4 触发存图 | 不支持 | 支持 | 不支持 | 不支持 |
| 5 福建威盛 | 支持 | 不支持 | 不支持 | 不支持 |
| 6 纳百川自动化 | 不支持 | 支持 | 支持 | 支持 |
| 7 纳百川线条 | 不支持 | 支持 | 不支持 | 支持 |

说明（写入 verificationNote，不开放例外）：三思控制事件、触发存图/福建威盛文本、纳百川线条「需 4 值」的数值发送等，在未验证前均为不支持。中机/三思透明文本保持不支持。

### 14.3 串口（实现层）

| 协议 | 数值接收 | 控制事件 | 数值发送 | 文本发送 |
|---|:---:|:---:|:---:|:---:|
| 0 | 支持 | 支持 | 支持 | 不支持 |
| 1 | 支持 | 不支持 | 支持 | 不支持 |
| 2 CSV | 支持 | 不支持 | 不支持 | 不支持 |
| 3 | 不支持 | 支持 | 支持 | 不支持 |
| 4 | 不支持 | 不支持 | 支持 | 不支持 |

说明（写入 limitation / verificationNote）：

- 串口 1 控制事件原为待验证 → 实现层不支持。
- 串口 3 数值发送支持，但 UI/适配器必须校验至少 5 个数值，否则拒绝发送。
- 串口 4 数值发送支持，但必须校验至少 2 个数值，否则拒绝发送。
- `SendData(QString)` 为空实现 → 文本发送一律不支持。

---

## 15. Simulator 与故障规则（M3）

通用 Raw 故障：延迟、不响应、截断、分片、重复、前置噪声、指定偏移字节翻转、主动断开。

不得在 Raw 规则声称“自动 CRC 错误”。仅 Codec 声明算法/范围/字节序后才提供错 CRC / 重算 CRC。

---

## 16. 建议工程结构

```text
CommunicationAssistant/
├─ core/
├─ transport/
├─ session/
├─ capture/
├─ send/
├─ simulator/     // M3 前可建空目录，不得暴露 UI
├─ legacy/        // M1 占位，不链接 DLL，不得暴露 UI
└─ ui/
```

`[DECISION]` **M1 界面不得出现 Legacy / Simulator / 回放入口。**

---

## 17. 里程碑

### M1：原生助手骨架

- 新工程与基础构建；
- `SessionConfig` / `configure` / `ICommSession` / `NativeSession`；
- Serial、TCP Client、TCP Server Channel、UDP；
- Raw HEX/文本发送；`requestId`/`taskId` 顶层字段；
- RawChunk / UdpDatagram 显示（不产生 ProtocolFrame）；
- SendScheduler（锚定 Submitted）；
- CaptureManager + 每 Session Writer（JSONL）；
- ResourceClaim 由 `claimFor` 推导；内部互斥；
- `legacy/` 占位不链接；UI 无 Legacy/回放。

M1 验收：

- 串口双端稳定互发；
- TCP 字节顺序正确且 Chunk 不冒充帧；
- TCP Server 区分至少两个客户端；
- UDP 记录源/目标端点；
- 周期/轮询可暂停、继续、停止；Failed 停任务；
- UI 清空不影响抓包文件；
- 队列满、写失败、端口占用有明确提示；
- 发送记录能按同一 `requestId` 区分 Queued / Submitted / Failed（及可选 Completed）。

### M2：DLL 兼容

- Worker 内创建/销毁 CommHandler；Direct 复制；能力表（二元）驱动 UI；
- `activeLegacySessionId` 单会话；Legacy 事件写入 Capture；
- Worker 外 Watchdog；Unresponsive 提示。

### M3：试验机与波形

- SimulatorSession + DeviceSimulator；
- ResponseRule；波形 → Scheduler → Native 或受支持的 Legacy 数值路径。

### M4：按需 Codec 与回放

- Optional Codec + 每 Channel Codec 缓存；
- Golden 测试；抓包重新解析；
- CaptureReader 与基础回放。

联恒协议无文档/Golden 时 `[BLOCKED]`。

---

## 18. 变更对齐与完成核对

`[DECISION]` **正式模板以 `docs/AUDIT_SPEC.md` §6（变更对齐单）与 §15（代码变更审计报告）为准。**  
本节仅作基线侧索引，避免与审计规范双份漂移。

开工前必须填写变更对齐单（字段见审计规范 §6），至少包括：变更编号、基线版本、审计规范版本、里程碑、明确不做、计划增删改文件、验收标准、是否已获「确认修改」。

修改完成后必须按审计规范 §15 出具《代码变更审计报告》（建议路径 `docs/reports/CA-….md`），并给出 PASS / CONDITIONAL PASS / FAIL。

未完成变更对齐单、或未获「确认修改」，不开始跨文件编码。  
没有构建或测试证据的项目不得标记为通过。

---

## 19. AI 执行约束

1. 先读 `docs/BASELINE.md` 与 `docs/AUDIT_SPEC.md`，再读任务涉及源码。
2. 不声称看过未提供或未读取的文件。
3. 不虚构类、接口、协议字段、测试结果或构建成功。
4. 不能编译时写“未编译”；静态检查≠构建成功。
5. 缺协议事实则 `[BLOCKED]`，停止猜测。
6. 不把当前里程碑外框架“顺便”做进来。
7. 不修改 CommHandler DLL，除非基线正式变更。
8. 不把 RawChunk 写成 ProtocolFrame；M1 不实现 Codec。
9. 不把 Legacy 事件写成原始抓包。
10. 不把 send 接受写成对端已收到；Scheduler 锚定 Submitted。
11. 与基线冲突先报告，不静默改基线。
12. 新需求先归类：本里程碑 / 后续 / 范围外。
13. **未满足 §21 不得创建工程或宣称可开工。**

---

## 20. 变更控制

必须升级版本的情况包括：

- 开始修改现有 DLL；
- 增加 Protocol Host；
- 允许多个 LegacySession；
- 改变 Capture JSONL 字段或语义；
- 改变 Session 异步语义或 Scheduler 锚点；
- 扩大 M1 范围（含 Codec/回放）；
- 开放此前不支持的 Legacy 能力；
- 将 `[BLOCKED]` 转为已实现协议事实。

| 版本 | 日期 | 变更 | 原因 | 验证证据 |
|---|---|---|---|---|
| V1.0 | 2026-07-20 | 初始冻结 | 建立可执行开发基线 | 上传源码审计与方案评审 |
| V1.1 | 2026-07-20 | 路径稳定化与部分补强 | 讨论审计稿；**未达到可直接开工** | 审计讨论 |
| V1.2 | 2026-07-20 | SessionConfig/claimFor；requestId 顶层；Scheduler=Submitted；CaptureManager+JSONL；ErrorEvent 权威；M1 去 Codec/回放；能力表二元；Legacy 非 Transport | 消除 M1 分叉 | P0/P1 审计与确定答案 |
| V1.2.1 | 2026-07-20 | §21 工具链核对；交叉引用唯一审计规范 `docs/AUDIT_SPEC.md` | 满足工具链门槛并统一审计入口 | vswhere + qmake；AUDIT_SPEC |
| V1.2.2 | 2026-07-20 | §0 强制双文档开工口令；唯一审计入口改为 `docs/AUDIT_SPEC.md`；废弃分散工具链审计文件 | 审计与基线分离且唯一 | 用户确认合并审计规范 |
| V1.2.3 | 2026-07-20 | §0 开工口令与 AUDIT_SPEC 全文一致；§18 改为变更对齐索引并以审计规范为准 | 消除口令与模板双份漂移 | 用户确认修改 |
| V1.2.4 | 2026-07-20 | §0 纳入 CODE_STYLE；三文档配套与冲突裁决顺序；开工口令同步 | 统一代码编写/注释/自检自测入口 | CA-DOC-CODESTYLE-001 |

---

## 21. 工具链与开工门槛

### 21.1 工具链（已核对本机）

| 项 | 决定 | 标记 | 证据 |
|---|---|---|---|
| IDE / 工具集 | Visual Studio 2019 Community，`PlatformToolset=v142`，x64 | `[VERIFIED]` | vswhere：`16.11.52`，路径 `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community` |
| Qt | Qt **5.15.2**，kit **`msvc2019_64`** | `[VERIFIED]` | `C:\Qt\5.15.2\msvc2019_64` 与 `D:\Qt\5.15.2\msvc2019_64`；`qmake -query QT_VERSION` → `5.15.2` |
| 推荐 QT 前缀 | 优先 `C:\Qt\5.15.2\msvc2019_64`（与现有 CommLab `QtInstall=msvc2019_64` 一致） | `[DECISION]` | 本仓库 CommLab/MachinePeer 工程注释与属性 |
| 语言标准 | **C++14**（`/std:c++14` / `LanguageStandard=stdcpp14`） | `[DECISION]` | 用户指定；同仓 MachinePeer 已用 `stdcpp14`。CommLab 现为 `stdcpp17`，**新工程不以之为准** |
| 构建系统 | Qt VS Tools + `.vcxproj` / `.sln`（与现有工程一致） | `[DECISION]` | 本阶段不引入 CMake 作为主构建 |

C++14 约束（实现必须遵守）：

- **禁止**依赖 C++17：`std::optional`、`std::variant`、`std::string_view`、结构化绑定作为公共接口要求等。
- `TransportConfig`：用 `TransportKind` + 分结构体 / 简单多态，不用 `std::variant`。
- Legacy 活动会话：用 `bool hasActiveLegacySession` + `QUuid`，不用 `std::optional`。
- 可使用 Qt5.15 已提供的类型与 API（如 `QString`、`QVector`、`QVariant`）。

验证方法（以后环境变更时重跑）：

```text
"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version [16.0,17.0)
C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe -query QT_VERSION
```

### 21.2 开工门槛

开始 **M1 建工程** 前必须确认：

- 本文档版本为 **V1.2.4**（含本节工具链）且本任务已填变更对齐单；已同时对齐 `docs/CODE_STYLE.md` 与 `docs/AUDIT_SPEC.md`；
- 工程名称与本地目录（建议 `CommunicationAssistant/`，与 CommLab 并列）；
- 工具链按 §21.1：`VS2019 + v142 + Qt5.15.2 msvc2019_64 + C++14`；
- M1 不链接、不修改 CommHandler；
- 计划修改文件均已实际读取；
- 验收方式可在当前环境执行，或明确哪些测试需用户设备。

未满足时：只允许只读审计、设计澄清和测试规划，**不应开始大范围写代码或建工程**。

`[DECISION]` 本节工具链已在本机核对后，**门槛中的 Qt/VS 项视为已满足**；开 M1 仍须单独任务对齐与「确认修改：开工 M1」。

环境核验与变更审计强制规范见：`docs/AUDIT_SPEC.md`（唯一审计文档；含附录 A 工具链核验）。

---

## 22. 最终结论

V1.2.4 在 V1.2.3 基础上纳入 `docs/CODE_STYLE.md` 三文档体系与冲突裁决顺序；变更流程强制对齐 `docs/AUDIT_SPEC.md`，可作为**正式可对齐基线**。

但：

- **满足 §21 之前，不得宣称“可以开始写代码 / 直接开工”。**
- 未验证资产与 DLL 阻塞风险仍然存在。
- 协议 Codec 与回放属于 M4。

在不突破本文档范围的前提下，下一步应是：填写任务对齐 → 确认门槛 → 再创建 CommunicationAssistant 工程并实施 M1。