# CommHandler 动态库使用与数据包详解

> **文档定位**：操作级 + 字节级说明。回答「如何调用库」「数据怎么定义」「包长什么样」「怎么发怎么收」。  
> **真理源**：`g:\My_work\2026-7\7-10\CommHandler\CommSdk\SocketComm.cpp`、`SerialPortComm.cpp`、`CommHandler.cpp`、`Common\UIDef.h`。  
> **配套**：[《CommHandler-验收问答手册》](./CommHandler-验收问答手册.md)（验收人 Q&A 形态，内容与本册对齐）。  
> **Demo**：`CommLab/` 与 `MachinePeer/` **均链接** `CommHandler.dll`（只调库 API；库无发送的能力不旁路、不自研组包）。  
> **对齐原则**：只写库中**现行可运行**协议；旧「联恒」已注释废弃，见文末「废弃项」。

---

## 目录

1. [你在系统中的身份与端到端角色](#1-你在系统中的身份与端到端角色)
2. [动态库架构与必须掌握的 API](#2-动态库架构与必须掌握的-api)
3. [完整生命周期：从创建到断开（逐步）](#3-完整生命周期从创建到断开逐步)
4. [数据结构定义（字段级）](#4-数据结构定义字段级)
5. [测量数据如何进入 `emitNewData`](#5-测量数据如何进入-emitnewdata)
6. [网口协议数据包圣经（iProtoType 0–7）](#6-网口协议数据包圣经iprototype-07)
7. [串口协议数据包圣经（iProtocolType 0–4）](#7-串口协议数据包圣经iprotocoltype-04)
8. [控制指令与事件宏](#8-控制指令与事件宏)
9. [用 CommLab / MachinePeer 走通三条主流程](#9-用-commlab--machinepeer-走通三条主流程)
10. [电压与脉冲通道入口](#10-电压与脉冲通道入口)
11. [参数 Key 总表](#11-参数-key-总表)
12. [如何新增 / 修改协议与指令](#12-如何新增--修改协议与指令)
13. [废弃项（不对齐 Demo）](#13-废弃项不对齐-demo)
14. [故障对照表](#14-故障对照表)

---

## 1. 你在系统中的身份与端到端角色

### 1.1 两套角色

| 角色 | 典型程序 | 是否加载 CommHandler.dll | 职责 |
|------|----------|---------------------------|------|
| **宿主软件**（DIC / 试验软件侧） | CommLab、真实产品进程 | **是** | 配置通道、连接、收测量、发控制意图、把回调变成界面/业务逻辑 |
| **试验机 / 外设对端** | MachinePeer、真实试验机 | **是**（MachinePeer Demo） | 经库 `SendData` 回测量、经库回调收事件/数值 |

动态库活在**宿主软件进程内**。它打开串口/UDP，把线路字节解析成 `double[]` 或事件；需要时把 `vector<double>` 组回线路。

### 1.2 三条报文种类（不要混）

| 种类 | 典型方向 | 例子 | 库入口 |
|------|----------|------|--------|
| **测量数值** | 机→软 或 软→机 | 力/位移/应变 | `emitNewData` / `SendData(vector)` |
| **控制指令** | 多为 软→机 或 机→软事件 | `3D0D`、中机 `0x0A`、JSON `tn` | `emitEventMsg`；串口字符串发送为空，见 §8 |
| **状态/返回** | 双向 | 中机 ReturnCommand、JSON code | 事件或回写 socket |

### 1.3 方向速查（联调默认）

```
软件 CommLab ──────────控制指令(3D0D 等)──────────► 试验机 MachinePeer
软件 CommLab ◄─────────测量帧(三思串口/UDP24B等)──── 试验机 MachinePeer
```

UDP 端口交叉默认：

| 程序 | 绑定 | 发往 |
|------|------|------|
| CommLab | `127.0.0.1:19105` | `127.0.0.1:19106` |
| MachinePeer | `127.0.0.1:19106` | `127.0.0.1:19105` |

---

## 2. 动态库架构与必须掌握的 API

### 2.1 模块图

```
CommHandler（门面，QObject）
  m_commModules[NETWORK]  → SocketComm
  m_commModules[SERIAL]   → SerialPortComm
  m_commModules[VOLTAGE]  → VoltageComm
  m_commModules[PULSE]    → PulseComm
  m_curCommModule         → SetCommType 选中的那个
```

构造时四个模块一次性 `new`，默认当前模块是 **NETWORK**。子模块信号在 `ConnectSlots()` 汇总到门面再 `emit`。

### 2.2 你应固定使用的门面方法

| 方法 | 作用 | 注意 |
|------|------|------|
| `SetCommType(CommType)` | 切换当前模块 | 写参数前必须先设对 |
| `setParameter(key, value)` | 写入当前（失败时模板会遍历其它模块） | key 写错会抛或空操作 |
| `getParameter(key)` | 读取 | |
| `Connect(int type=-1)` | 建连 | 电压时 type=AD/DA/CI/DI |
| `Disconnect(int type=0)` | 断连 | `0` 当前；门面 `-1` 路径语义见源码 |
| `SendData(vector<double>, CommType)` | 按**参数里指定的协议**组包发送 | **看当前/目标模块的协议号** |
| `SendData(QString, CommType)` | 字符串发送 | **网口有实现；串口 override 为空** |
| `SetSysParInfo` / `GetSysParInfo` | 整包结构体 | 产品多用，Demo 多用零散 setParameter |
| `SaveParInfo` / `LoadParInfo` | 存本地配置 | |
| `saveCmdToFile` / `loadCmdFromFile` | 命令配置 | |

串口额外门面转发：`SendRequireMsg`、`SendData2ComPort`、`DataPackAndWrite`、`WaitForBytesWritten`。  
电压：`HardTrigger`、`ReleaseHardTrigger`、`ReadADData`、`ReadCIFreq`。  
脉冲：`SendCaliData`。

### 2.3 必须连接的信号

```cpp
void emitNewData(void* data, int size, int type);
// data: double*；size: double 个数；type: 协议/通道侧附带类型（视模块，如中机收力用 2）

void emitEventMsg(int ctrlCmd, int ViewID, int msg);
void emitEventMsgAndData(const int& ctrlCmd, const int& ViewID, const int& msg, const QVariantMap& extra);

void emitNewConn(int iType, QString ip, int port);
// iType: 0 TCP Server, 1 TCP Client, 2 UDP Server, 3 UDP Client

void emitClientDisConn();
```

**`void*` 生命期**：仅在发射线程、槽返回前有效。CommLab 用 `Qt::DirectConnection` + `memcpy` 到 `QVector<double>`，禁止把裸指针 `QueuedConnection` 到 UI 线程。

### 2.4 发送许可 `bInquireSendFlag`

网口/串口模块在 `SendData(vector)` 开头：

```cpp
if (!m_socketInfo.bInquireSendFlag) return;   // 网口
if (!m_SerialPortInfo.bInquireSendFlag) return; // 串口同类逻辑
```

另：网口若 `output.iTransferType == 1`（轮询语义），**成功发送一次后会把 `bInquireSendFlag` 置 false**，下次要再打开才能发。

---

## 3. 完整生命周期：从创建到断开（逐步）

### 3.1 伪代码（任何协议共用骨架）

```cpp
#include "CommHandler.h"
#include "UIDef.h"
#include <QObject>
#include <cstring>
#include <vector>

CommHandler* comm = new CommHandler;

// ① 选通道
comm->SetCommType(SERIAL); // 或 NETWORK / VOLTAGE / PULSE

// ② 写参数（见各协议章节完整清单）
comm->setParameter(QStringLiteral("iComPort"), 3);
// ... 其它 key ...

// ③ 接信号（建议在 Connect 之前）
QObject::connect(comm, &CommHandler::emitNewData, /*...*/);
QObject::connect(comm, &CommHandler::emitEventMsg, /*...*/);

// ④ 连接
if (!comm->Connect()) {
    // 失败：端口占用、地址非法、硬件无等
}

// ⑤ 业务循环：收回调 / SendData / 发指令

// ⑥ 断开
comm->Disconnect(0);
delete comm;
```

### 3.2 安全接收槽（强烈建议照抄语义）

```cpp
QObject::connect(comm, &CommHandler::emitNewData,
    object,
    [object](void* data, int size, int type) {
        if (!data || size <= 0) return;
        QVector<double> values(size);
        std::memcpy(values.data(), data,
                    static_cast<size_t>(size) * sizeof(double));
        // 再投递到 UI / 业务
        Q_UNUSED(type);
    },
    Qt::DirectConnection); // 关键：先拷贝再排队
```

### 3.3 `SetCommType` 与 `SendData(..., CommType)` 的关系

- `setParameter` 默认打在 **当前** `m_curCommModule`。  
- `SendData(v, SERIAL)` 会走到串口模块的 `SendData(vector)`，但其内部仍用该模块已配置的 `iProtocolType`。  
- 实践建议：**先 `SetCommType` → 再写该通道全部参数 → Connect → SendData 时 CommType 与当前通道一致**，避免「参数写在串口、却按网口发送」的混乱。

---

## 4. 数据结构定义（字段级）

以下均来自 `UIDef.h`（Demo：`CommLab/include/UIDef.h`）。

### 4.1 通道枚举 `CommType`

| 值 | 名 | 模块 |
|----|-----|------|
| 0 | `NETWORK` | SocketComm |
| 1 | `SERIAL` | SerialPortComm |
| 2 | `VOLTAGE` | VoltageComm |
| 3 | `PULSE` | PulseComm |

### 4.2 通用测量结构 `Data`（三思 UDP 等）

```cpp
struct Data {
    double dForce;   // 力
    double dMove;    // 位移
    double dStrain;  // 应变
};
```

**线长**：`sizeof(Data) == 24`（3×8，无 pack 时按自然对齐为 24）。  
UDP 三思收包条件：`data.size() == sizeof(Data)`。

### 4.3 `DataInput` / `DataOutput`

```cpp
struct DataInput {
    int iChannelState;     // 通道状态
    int iChannelSize;      // 通道数
    int iTriggerCtrl;      // 触发控制：非 0 时串口/部分网口才响应开始/停止等
    int iTransferType;     // 0 数据流；1 轮询（与 output 侧配合发送许可）
    int iDataType[4];      // 0 力；1 温度等
    int iUnit[4];
};

struct DataOutput {
    int iChannelState;
    int iChannelSize;
    int iTriggerCtrl;
    int iTransferType;     // 网口：成功发一次后若为 1 会关 bInquireSendFlag
    int iDataModel[8];
    int iDataType[8];
    int iData[8];
    bool bReverse[8];
};
```

CommLab 串口最小注入：

```cpp
DataInput input{};
input.iChannelState = 1;
input.iTriggerCtrl = 1;   // 允许识别 3D0D 等
comm.setParameter("input", input);
```

### 4.4 `SocketInfo`（网口整包）

关键字段：

| 字段 | 含义 |
|------|------|
| `iTransferType` | 0 TCP / 1 UDP |
| `iModel` | 0 服务端 / 1 客户端 |
| `sLocalIP` / `iLocalPort` | 本机 |
| `sPurIP` / `iPurPort` | 目的（**setParameter 用别名 `sDestIP` / `iDestPort`** 写入这两个成员） |
| `iProtoType` | 现行 0–7，见 §6 |
| `bOpenCMD[3]` / `cCMD[3][16]` | 万测等：自定义开始/结束/退出命令串 |
| `iSymbolType` | 万测分隔：1 `,` 2 空格 3 `;` |
| `bOnlineCollect` / `bAllOpenCamera` | 业务状态（JSON/中机逻辑会读） |
| `bInquireSendFlag` | 发送许可 |

### 4.5 `SerialPortInfo`（串口整包）

| 字段 | 含义 |
|------|------|
| `iComPort` | **0 = COM1** |
| `iComBaud` | 0…5 → 4800…115200 |
| `iParity` | 0 无 1 偶 2 奇 3 space 4 mark |
| `iDataBits` | 0→5，1→6，**3→8**（库无 7 位映射） |
| `iStopBits` | 0→1，1→1.5，2→2 |
| `iProtocolType` | 0–4，见 §7 |
| `dInForceRange` | 力值量程（部分解码用） |
| `bInquireSendFlag` | 发送许可 |
| `input` / `output` | 同 §4.3 |

### 4.6 中机 / 南航 DIC 相关 `SocketProtol`（`#pragma pack(push,1)`）

```cpp
enum MessageType {
    CONTROL_MESSAGE = 0x01,
    DATA_MESSAGE = 0x02,
    RETURN_MESSAGE = 0x03,
    SERVER_DATA_MESSAGE = 0x04,
};

struct ControlMessage { // 2 字节
    uint8_t msgType;
    uint8_t command;
};

struct ForceData { // 1 + 32 = 33 字节
    uint8_t msgType;
    double forceValues[4]; // x1,y1,x2,y2
};

struct ServerData { // 1 + 16 = 17 字节
    uint8_t msgType; // 固定 SERVER_DATA_MESSAGE
    double valueX;
    double valueY;   // -1 表示无数据
};
```

控制命令字（节选）：`START_CALC=0x0A`，`STOP_CALC=0x0B`，`DATA_COLL_POLLING=0x0C`，`DATA_COLL_FLOW=0x0D`，`ACQ_STATE=0x0E`。  
返回字：`START_RUNNING=0x64`（100）… 见头文件。

### 4.7 电压 / 脉冲结构

- `VoltageInfo`：AD 最多 16 通道名/单位/标定/开关，DA 2 通道量程与缩放。  
- `PulseInfo`：卡类型、精度、标定、帧率、USB/NET 速度等。  
字段极多，接入时用 `SetSysParInfo` 整包或按产品页 `setParameter`；Demo 未做 UI。

---

## 5. 测量数据如何进入 `emitNewData`

### 5.1 宿主侧视角

1. 线路到达 Socket / Serial 模块。  
2. 按 `iProtoType` / `iProtocolType` 分支解析。  
3. `new double[]`（或单指针）填值。  
4. `emit emitNewData(ptr, count, type)`。  
5. 门面 `onNewDataReceived` 再 `emit`。  
6. **随后库常 `delete` 该缓冲** → 你必须在槽内立刻拷贝。

### 5.2 `size` / `type` 常见情况

| 场景 | size | type（观察到的） |
|------|-------|------------------|
| UDP 三思 | 1（仅力） | 0 |
| 中机 ForceData | 4 | 2 |
| 福建威盛 | 1 | 0 |
| 串口三思解码 | 视 `MsgDataInput` 等路径 | 视实现 |

`size<=0`：CommLab 记为「忽略空回调」，不是有效测量。

### 5.3 宿主如何「定义要发送的数据」

统一容器：

```cpp
std::vector<double> v = { 12.34, 56.78, 0.1 };
comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
comm->SendData(v, NETWORK); // 或 SERIAL
```

含义**完全由当前协议的组包函数解释**：  
- JSON：`values` 数组原样进 JSON；  
- 中机：只认恰好 2 个数；  
- 三思串口：每个 double 变成一段 ASCII；  
- 协议 7：必须 4 个数填入 L1/b1/Le1/bo1。

---

## 6. 网口协议数据包圣经（iProtoType 0–7）

真理源：`SocketComm::SendData` / `onRecvDataSlots`。

总表：

| iProtoType | 名称 | 收 | `SendData(vector)` |
|------------|------|----|---------------------|
| 0 | JSON | ✅ `ParsingProtocolI` | ✅ `CreateJsonDataPack` |
| 1 | 万测 | ✅ `ParsingProtocolII` | ✅ `CreateWanceSimbolDataPack` |
| 2 | 中机四通道 | ✅ `ParsingProtocolZhongji` | ✅ 恰好 2 值 → `ServerData` |
| 3 | 三思 | ✅ `ParsingProtocolSansi` | ❌ |
| 4 | 触发存图 | 事件（含 `\r\n`） | ❌ |
| 5 | 福建威盛 | ✅ | ❌ |
| 6 | 纳百川全自动 | ✅ | ✅ `CreateHNBCDataPack` |
| 7 | 纳百川识别线条 | ✅ | ✅ 4 值 → `GenJsonDocument` |

### 6.0 网口连接完整示例（UDP 服务端骨架）

```cpp
void configureUdp(CommHandler& comm, int protoType)
{
    comm.SetCommType(NETWORK);
    comm.setParameter(QStringLiteral("iTransferType"), 1);      // UDP
    comm.setParameter(QStringLiteral("iModel"), 0);              // 服务端
    comm.setParameter(QStringLiteral("sLocalIP"), QStringLiteral("127.0.0.1"));
    comm.setParameter(QStringLiteral("iLocalPort"), 19105);
    comm.setParameter(QStringLiteral("sDestIP"), QStringLiteral("127.0.0.1"));
    comm.setParameter(QStringLiteral("iDestPort"), 19106);
    comm.setParameter(QStringLiteral("iProtoType"), protoType);
    comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
}
```

---

### 6.1 协议 0 — JSON

#### 发送数据包

函数：`CreateJsonDataPack`。

逻辑：把每个 `double` 放进 JSON 数组 `values`，并带上固定字段：

```json
{"code":0,"values":[1.0,2.0,3.0],"tn":2}
```

完整发送：

```cpp
configureUdp(comm, 0);
comm.Connect();
std::vector<double> v{1.0, 2.0, 3.0};
comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
comm.SendData(v, NETWORK);
```

`bInquireSendFlag=false` 时函数直接 return，线上**无包**。CommLab 界面勾选「发送许可」控制此门闩。

#### 接收路径

`ParsingProtocolI`：按 JSON 对象解析。其中一种控制路径会根据数值/键触发开始计算等，并可能回写：

```json
{"code":0,"message":null,"ylength":[...],"xlength":[...],"tn":1}
```

错误例：相机未开 → `code:70`；标距不存在 → `code:100`。  

**要点**：入站报文是**文本 JSON**；出站测量也是文本 JSON。用 Wireshark / UDP 调试助手按文本看即可。

---

### 6.2 协议 1 — 万测

#### 发送数据包

`CreateWanceSimbolDataPack`：用 `iSymbolType` 选分隔符拼接数值。

| iSymbolType | 分隔符 | 例（1.1,2.2,3.3） |
|-------------|--------|-------------------|
| 1 | `,` | `1.1,2.2,3.3` |
| 2 | 空格 | `1.1 2.2 3.3` |
| 3 | `;` | `1.1;2.2;3.3` |

```cpp
configureUdp(comm, 1);
comm.setParameter(QStringLiteral("iSymbolType"), 1);
comm.Connect();
comm.SendData(std::vector<double>{1.1, 2.2, 3.3}, NETWORK);
```

#### 接收路径

`ParsingProtocolII`：当 `input.iTriggerCtrl` 为真，且：

- `bOpenCMD[0]` 且报文等于 `cCMD[0]` → `STARTCALC`  
- `bOpenCMD[1]` / `cCMD[1]` → `STOPCALC`  
- `bOpenCMD[2]` / `cCMD[2]` → `EXITPROG`  

即：**命令内容完全由配置字符串决定**，不是固定 `3D0D`。

需在产品侧配置：

```cpp
comm.setParameter(QStringLiteral("bOpenCMD"), /* 数组 true */); // 以库实际 setter 为准；或 SetSysParInfo 整包
// cCMD 三个命令串写入 SocketInfo
```

Demo 未暴露万测命令配置 UI；联调时请用 `SetSysParInfo` 或扩展 Demo。

---

### 6.3 协议 2 — 中机四通道

#### 发送数据包（软件 → 对端）

`CreateZhongJiShuangZhouFour`：

- **要求** `vData.size() == 2`，否则返回空串 → **不发送**。  
- 填充 `ServerData`：`msgType=0x04`，`valueX=v[0]`，`valueY=v[1]`。  
- 线长 **17 字节**（pack 1）。

```cpp
configureUdp(comm, 2);
comm.Connect();
comm.SendData(std::vector<double>{12.0, 34.0}, NETWORK); // 必须 2 个
```

内存布局示意（小端）：

```
偏移0: msgType = 0x04
偏移1: double valueX
偏移9: double valueY
```

#### 接收数据包

| 条件 | 行为 |
|------|------|
| 长度 == 2（ControlMessage） | 解析控制/返回；可能回写 ReturnMessage |
| 长度 == 33（ForceData）且 msgType==DATA | `emitNewData` 4 个 double，type=2 |

控制开始 `command=0x0A`：可能 emit `STARTCALC`，并回 `NORMAL_RUNNING` / `ALREADY_RUNNING` / `CAMERA_NOT_OPEN`。  
停止 `0x0B`：emit `STOPCALC`，回 `NORMAL_STOP` / `ALREADY_STOPPED`。  
`0x0C`/`0x0D`：切换 `output.iTransferType` 并打开发送许可。

控制开始 `command=0x0A` 等：库解析后 `emitEventMsg`。对端可用网口 `SendData(QString, NETWORK)` 发出原始字节（若协议支持写出）。

---

### 6.4 协议 3 — 三思试验（仅收测量）

#### 接收数据包（唯一主用）

条件：`data.size() == sizeof(Data)` → **24**。

布局：

```
偏移 0  : double dForce
偏移 8  : double dMove
偏移 16 : double dStrain
```

库行为：只把 `dForce` 放入一个 `double`，`emitNewData(resdata, 1, 0)`。  
即应用层常常**只看到力值**，位移/应变虽在包内但当前解析未回调。

MachinePeer 组包须走 `SendData(vector)`；本协议库无 vector 发送 case，故 Demo 仅验收**接收**侧 `emitNewData`。

#### 发送

`SendData(vector)` 无 `case 3` → **静默不发**。测发送请换 0/1/2/6/7。

完整接收示例：

```cpp
configureUdp(comm, 3);
QObject::connect(&comm, &CommHandler::emitNewData, ...);
comm.Connect();
// 等待对端 24 字节包 → 回调 size=1
```

---

### 6.5 协议 4 — 触发存图

收：报文转为 `std::string`，若 `find("\r\n") > 0`，则：

```cpp
emit emitEventMsg(..., W_CUSTOM_COMM_SINGALTRIIMAGESAVE);
```

无数值 SendData。联调：对端发任意含 `\r\n` 的文本即可。

---

### 6.6 协议 5 — 福建威盛

收：文本，`T` 或 `S` 开头，形如注释：

```
TL123.45E67.89D12.34M56.78
```

解析 `L` 与随后 `E` 之间的力值 → `emitNewData` 一个 double。  
无向量发送实现。

---

### 6.7 协议 6 — 纳百川全自动

#### 发送

```json
{"values":[1,2,3],"name":"LVEComputedValues"}
```

```cpp
configureUdp(comm, 6);
comm.Connect();
comm.SendData(std::vector<double>{1, 2, 3}, NETWORK);
```

#### 接收（按 JSON `name`）

| name | 行为 |
|------|------|
| `alphaStartTest` | 开始采集状态；可能 `STARTCALC`；打开发送许可 |
| `alphaStopTest` | 停止；`STOPCALC` |
| `alphaSetLD` | `emitEventMsgAndData` 重置标距，extra 含 L/D |
| `alphaSetGauge` | extra 含 length/Gnum/Gexnum |

---

### 6.8 协议 7 — 纳百川识别线条

#### 发送

必须 **4** 个 double，否则不组包：

```cpp
comm.SendData(std::vector<double>{L1, b1, Le1, bo1}, NETWORK);
```

生成紧凑 JSON（字段均为字符串形式数字）：

```json
{"hsm":{"L2":"0","b2":"0","Le2":"0","bo2":"0","bu":"0","val":"0","YMaxval":"0","XMaxval":"0","L1":"...","b1":"...","Le1":"...","bo1":"..."}}
```

#### 接收

文本 `calcStart` / `calcEnd` 等（见 `paeseNaBaiChuanDataCalcLine`）触发开始/停止事件。

---

### 6.9 网口字符串发送

```cpp
comm.SendData(QStringLiteral("hello\r\n"), NETWORK);
```

SocketComm：`m_controlBox.sendData(data)` —— **有效**。  
可用于协议 4 触发存图、或自定义文本（仍建议设对 `iProtoType` 以便收侧逻辑正确）。

---

## 7. 串口协议数据包圣经（iProtocolType 0–4）

### 7.0 串口连接完整示例

```cpp
void configureSerial(CommHandler& comm, int protocolType)
{
    comm.SetCommType(SERIAL);
    comm.setParameter(QStringLiteral("iComPort"), 3);       // COM4
    comm.setParameter(QStringLiteral("iComBaud"), 5);       // 115200
    comm.setParameter(QStringLiteral("iDataBits"), 3);      // 8
    comm.setParameter(QStringLiteral("iParity"), 0);
    comm.setParameter(QStringLiteral("iStopBits"), 0);
    comm.setParameter(QStringLiteral("iProtocolType"), protocolType);
    comm.setParameter(QStringLiteral("dInForceRange"), 100.0);
    DataInput in{};
    in.iChannelState = 1;
    in.iTriggerCtrl = 1;
    comm.setParameter(QStringLiteral("input"), in);
    comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
}
```

`SendData(QString)` 在串口模块为 **空实现** —— 控制字节见 §8。

---

### 7.1 协议 0 — 三思 SSCK

#### 发送帧格式

- 每个值占用 `m_iDataBits` 字节（Connect 后与数据位相关；Demo 预览用 5/6/8）。  
- 字节 0：`+`(0x2B) 或 `-`(0x2D)。  
- 其后：ASCII `0-9`、`.`；无法映射则 `*`(0x2A)。  
- **整帧末尾**追加一个 `0x0D`（CR）。  
- 总长：`segSize * N + 1`。

组包入口：`FloatToHex` + 写尾 `0D`。

**示例**：`segSize=8`，单值 `1.0`（与预览一致，实际小数位受算法影响）：

```
2B 30 2E ........ 0D
```

多值则顺序拼接各段后再加一个 `0D`。

完整发送：

```cpp
configureSerial(comm, 0);
comm.Connect();
comm.SendData(std::vector<double>{1.0, 2.5}, SERIAL);
```

组包由 `SerialPortComm::SendData` 完成；Demo 不本地预览、不旁路。

#### 接收：控制指令（同缓冲十六进制字符串比较）

当 `input.iTriggerCtrl != 0`：

| 原始帧 HEX | 事件 |
|------------|------|
| `3D0D` | `W_CUSTOM_COMM_STARTCALC` |
| `3E0D` | `W_CUSTOM_COMM_STOPCALC` |
| `6B0D` | `W_CUSTOM_COMM_AUTO_SAVE_IMAGE` |
| `5A0D` | `W_CUSTOM_ZERO_CLEARING` |

另有以 `47` 开头的标距扩展帧（含 `H`/`V` ASCII 量值），会解析水平/垂直标距（详见源码 `onRecvDataSlots`）。

#### 接收：测量

`MsgDataInput` case 0：把十六进制文本按整型解读再换算力（依赖 `dInForceRange`）。联调真实三思 ASCII 帧时，以示波器与 `[预期帧]` 为准。

---

### 7.2 协议 1 — 长春科新

#### 发送

- 固定 **14 字节**缓冲。  
- `FixedDoubleToHex`：格式化为约 `#####.#####` + `#`（`0x23`）规则填入偏移（见源码 `iRowNum*13` 布局）。  
- 只使用 `vData.at(0)`。

```cpp
configureSerial(comm, 1);
comm.Connect();
comm.SendData(std::vector<double>{12.34567}, SERIAL);
```

#### 接收

`MsgDataInput` case 1：`HexToFloat` 解码后 `size=2`（值 + 状态）。

---

### 7.3 协议 2 — 时代新材

`SendData` 中 `case 2: break;` —— **不写串口**。  
CommLab UI 禁发。收侧若存在其它路径以源码为准；Demo 不对齐发送。

---

### 7.4 协议 3 — IEEE

#### 发送

- 要求 `vData.size() >= 5`，否则直接 return。  
- 调用 `SendData2ComPort(d0..d3, 0, v4)`，再 `write(DataPack(), 56)`。  
- 不走普通 `byteArray` 尾写分支（`if (3 != iProtocolType)`）。

语义（参数顺序）：应变1、应变2、位移1、位移2、预留、时间（第 5 个起）。

```cpp
configureSerial(comm, 3);
comm.Connect();
comm.SendData(std::vector<double>{0.1, 0.2, 1.0, 2.0, 0.01}, SERIAL);
```

#### 接收

`MsgDataInput` case 3：从定长 ASCII 十六进制切片解码出力、位移、小变形、大变形、时间等，`size=6`。

---

### 7.5 协议 4 — 冠腾

#### 发送

```text
R{±000.0000}#N{±000.0000}#
```

`formatValue`：符号 + 宽度 8、小数 4、左侧 `0` 填充。

例：力 `12.5`、位移 `-1.25` → 类似：

```text
R+0012.5000#N-0001.2500#
```

```cpp
configureSerial(comm, 4);
comm.Connect();
comm.SendData(std::vector<double>{12.5, -1.25}, SERIAL);
```

---

## 8. 控制指令与事件宏

### 8.1 发送入口（严格按库）

| 通道 | 测量/数值 | 控制/文本 |
|------|-----------|-----------|
| 网口 | `SendData(vector, NETWORK)` | `SendData(QString, NETWORK)` |
| 串口 | `SendData(vector, SERIAL)` | `SendData(QString, SERIAL)` **空实现，Demo 不提供** |

串口三思控制帧（如 `3D0D`）由库在**收侧**解析为事件；本 Demo 不从应用层发送此类帧。

### 8.2 `W_CUSTOM_*` 事件（宿主收到）

定义在 `UIDef.h`（`WM_USER+10` 起）：

| 宏 | 用途 |
|----|------|
| `W_CUSTOM_COMM_STARTCALC` | 开始计算 |
| `W_CUSTOM_COMM_STOPCALC` | 停止 |
| `W_CUSTOM_COMM_EXITPROG` | 退出 |
| `W_CUSTOM_COMM_AUTO_SAVE_IMAGE` | 自动存图 |
| `W_CUSTOM_ZERO_CLEARING` | 清零 |
| `W_CUSTOM_COMM_SINGALTRIIMAGESAVE` | 信号触发存图 |
| `W_CUSTOM_COMM_DATASTREAMING` / `DATAPOLLING` | 流 / 轮询 |
| `W_CUSTOM_COMM_RESET_LVE_LENGTH` / `CALC_LVE_LENGTH` | 标距 |
| 脉冲三类 | `UPDATEPULSECTRL` 等 |

`emitEventMsg(ctrlCmd, viewId, msg)` 中常：`ctrlCmd=ControlSoftStatus`，`msg=上表宏`。

**验收**：对端日志出现 `[收← data]` / `[收← event]` 表示库已解包；仅本端有事件而对端无收，检查协议号与发送 API 是否匹配。

---

## 9. 用 CommLab / MachinePeer 走通主流程

### 流程 A — UDP JSON 双向（推荐入门）

1. 两端 UDP，协议 **0 JSON**，端口 19105↔19106 交叉。  
2. MachinePeer 先打开，CommLab 连接。  
3. CommLab 勾选发送许可 → `SendData(vector)` → Peer `[收← data]`。  
4. Peer `SendData(vector)` 回发 → CommLab `[收← data]`。  
5. CommLab `SendData(QString)` 发 JSON/文本 → Peer 视协议解析为事件或数据。

### 流程 B — UDP 三思仅收

1. CommLab 协议 **3**，连接。  
2. 库无 `SendData(vector)`；仅当对端经库发出可被解析的测量帧时，CommLab 出现 `[收← data]`。  
3. 非法长度不进 `emitNewData`。

### 流程 C — 串口三思数值

1. 虚拟串口 COM4↔COM5：CommLab `iComPort=3`，Peer `iComPort=4`，同波特率 8N1。  
2. 两端协议 **0**，先 Peer 后 Lab 连接。  
3. `SendData(vector)` 双向；串口控制事件仅能通过库收侧解析（本 Demo 不提供串口文本发送）。

### 日志约定

| 标记 | 含义 |
|------|------|
| `[发→ vector]` / `[发→ QString]` | 本端调用库发送 |
| `[收← data]` / `[收← event]` | 库回调解包结果 |
| `[连接]` / `[拒发]` | 连接状态与发送门禁 |

---

## 10. 电压与脉冲通道入口

Demo 无硬件 UI，但产品接入步骤：

```cpp
// 电压 AD
comm.SetCommType(VOLTAGE);
// 通过 SetSysParInfo 写入 VoltageInfo，或逐 key setParameter
comm.Connect(0); // 0=AD,1=DA,2=CI,3=DI
comm.HardTrigger(value, duty);
comm.ReadADData(...);
comm.ReleaseHardTrigger();
comm.Disconnect(0);

// 脉冲
comm.SetCommType(PULSE);
comm.Connect();
comm.SendCaliData();
```

事件：脉冲模块接 `emitEventMsg`（标定完成、刷新按钮等）。

---

## 11. 参数 Key 总表

### 网口（SocketComm setters）

`iTransferType` `iModel` `sLocalIP` `iLocalPort` `sDestIP`（→sPurIP）`iDestPort`（→iPurPort）`iProtoType` `iSymbolType` `bInquireSendFlag` `input` `output` 以及整包 `SetSysParInfo`

### 串口

`iComPort` `iComBaud` `iDataBits` `iParity` `iStopBits` `iProtocolType` `dInForceRange` `bInquireSendFlag` `input` `output` …

未列全的字段以 `initParameters()` 中 `setters[...]` 为准；找不到的 key 会在模板路径失败并可能抛 `runtime_error`。

---

## 12. 如何新增 / 修改协议与指令

1. **只改 Demo、不改 DLL** → 线上行为不变。  
2. 网口新协议：`SocketComm::onRecvDataSlots` + 可选 `SendData` case + 组包函数 + 更新 UIDef 注释 + CommLab 下拉。  
3. 串口新协议：`SerialPortComm` 收发 case 同步。  
4. 新指令字节：DLL 识别 + `UIDef` 宏；Demo 仅经 `emitEventMsg` / `emitEventMsgAndData` 日志展示。  
5. 旧联恒不要复活 UI，除非先恢复库实现。

---

## 13. 废弃项（不对齐 Demo）

| 项 | 状态 |
|----|------|
| 网口旧联恒 `case 0` | 源码注释；现 `0=JSON` |
| 串口旧联恒发送分支 | 注释 |
| UIDef 过时「协议一联恒…协议五三思」 | 以 SocketComm 0–7 为准；Demo 头文件注释已改 |

---

## 14. 故障对照表

| 现象 | 排查 |
|------|------|
| `SendData` 无出站 | `bInquireSendFlag`？协议是否支持发？中机是否正好 2 个数？ |
| 有事件无对端收 | 协议号是否一致？串口 `SendData(QString)` 库为空，须用网口文本或 vector |
| UDP 三思无回调 | 是否恰 24 字节？协议是否为 3？本机端口是否被占？ |
| Connect 失败 | COM 占用、IP/端口冲突、权限 |
| 空回调刷屏 | 库内部空包；CommLab 已节流，不算测量 |
| JSON 发一次后不能再发 | `output.iTransferType==1` 自动关许可，需再置 true |

---

## 附录：源码锚点

| 主题 | 文件 |
|------|------|
| 门面转发 | `CommHandler.cpp` |
| 网口收发 | `CommSdk/SocketComm.cpp` |
| 串口收发 | `CommSdk/SerialPortComm.cpp` |
| 结构体 | `Common/UIDef.h` |
| Demo 安全回调 | `CommLab/src/CommController.cpp` |
| Demo 协议能力表 | `CommLab/src/ProtoGuide.h` |
| Demo 发送封装 | `CommLab/src/DemoComm.h` |

---

*修订时以 SocketComm / SerialPortComm 现行 switch 为准；与过时注释冲突时信源码。*
