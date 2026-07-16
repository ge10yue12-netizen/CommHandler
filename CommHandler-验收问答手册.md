# CommHandler 验收问答手册

> **文档定位**：验收人 / 学习者用 **自问自答** 形式，吃透动态库用法。  
> **与本仓关系**：独立手册，不并入 README；内容与  
> [《CommHandler-动态库使用与数据包详解》](./CommHandler-动态库使用与数据包详解.md) **对齐**（协议号、包格式、API 行为一致）。  
> **更偏字节布局、逐字段结构**时，以《详解》为主文；本册用问答把「怎么验、怎么用、容易错在哪」写透。  
> **真理源**：`7-10\CommHandler` 现行 `SocketComm` / `SerialPortComm`（非过时 UIDef 中文序号）。  
> **废弃**：旧联恒已注释 —— 不对齐 Demo，见 Q37。

---

## 目录（问题列表）

- [Q1–Q8 身份、架构、生命周期](#第一部分身份架构与生命周期)
- [Q9–Q16 数据如何定义、如何进回调](#第二部分数据如何定义如何进回调)
- [Q17–Q24 网口协议与数据包](#第三部分网口协议与数据包)
- [Q25–Q31 串口协议与数据包](#第四部分串口协议与数据包)
- [Q32–Q36 指令与联调流程](#第五部分指令与联调流程)
- [Q37–Q42 扩展、废弃、验收清单](#第六部分扩展废弃与验收清单)

---

# 第一部分：身份、架构与生命周期

## Q1. 验收人：这个 Demo 和动态库是什么关系？各自干什么？

**答：**

| 名字 | 是什么 | 干什么 |
|------|--------|--------|
| `CommHandler.dll` | 通讯动态库 | 打开串口/UDP（及电压/脉冲）、按协议号组包/拆包、回调测量与事件 |
| `CommLab` | 软件侧学习壳 | 链接 DLL：`SetCommType`/`setParameter`/`Connect`/`SendData`/信号 |
| `MachinePeer` | 试验机侧学习壳 | **同样链接 DLL**；收事件/数值，库支持时用 `SendData(vector)` 回测量 |
| 本手册 + 《详解》 | 文档 | 说明现行协议与用法 |

**验收身份**：你站在「会不会用对库」一侧。金标准是：配置对、包格式对、对端日志能印证「线上海真到了」。

---

## Q2. 验收人：库内部架构怎么记？

**答：**

```
CommHandler
 ├─ NETWORK → SocketComm     ← iProtoType 0..7
 ├─ SERIAL  → SerialPortComm ← iProtocolType 0..4
 ├─ VOLTAGE → VoltageComm
 └─ PULSE   → PulseComm
```

构造时四个模块都创建，默认当前是 NETWORK。`SetCommType` 只换「当前指针」，不销毁其它模块。

---

## Q3. 验收人：最小正确用法（骨架示例）必须包含什么？

**答：完整骨架如下（可粘贴为起点）。**

```cpp
#include "CommHandler.h"
#include "UIDef.h"
#include <QObject>
#include <cstring>
#include <vector>

int useSerialSansiOnce()
{
    CommHandler comm;

    // 1) 通道
    comm.SetCommType(SERIAL);

    // 2) 参数（COM4 / 115200 8N1 / 三思）
    comm.setParameter(QStringLiteral("iComPort"), 3);
    comm.setParameter(QStringLiteral("iComBaud"), 5);
    comm.setParameter(QStringLiteral("iDataBits"), 3);
    comm.setParameter(QStringLiteral("iParity"), 0);
    comm.setParameter(QStringLiteral("iStopBits"), 0);
    comm.setParameter(QStringLiteral("iProtocolType"), 0);
    comm.setParameter(QStringLiteral("dInForceRange"), 100.0);
    DataInput in{};
    in.iChannelState = 1;
    in.iTriggerCtrl = 1; // 否则 3D0D 可能不转事件
    comm.setParameter(QStringLiteral("input"), in);
    comm.setParameter(QStringLiteral("bInquireSendFlag"), true);

    // 3) 信号（Connect 前绑定更稳妥）
    QObject::connect(&comm, &CommHandler::emitNewData,
                     [&](void* data, int size, int) {
                         if (!data || size <= 0) return;
                         QVector<double> v(size);
                         std::memcpy(v.data(), data, size * sizeof(double));
                         // 使用 v ...
                     },
                     Qt::DirectConnection);

    QObject::connect(&comm, &CommHandler::emitEventMsg,
                     [&](int, int, int msg) {
                         // msg 对照 W_CUSTOM_* 
                     });

    // 4) 连接
    if (!comm.Connect()) return 1;

    // 5) 发测量
    comm.SendData(std::vector<double>{1.0}, SERIAL);

    // 6) 断开
    comm.Disconnect(0);
    return 0;
}
```

网口把 `SetCommType(NETWORK)` 与 `iTransferType/iModel/sLocalIP/.../iProtoType` 换上即可（见 Q17）。

---

## Q4. 验收人：`SendData` 有几种？分别可靠吗？

**答：**

| API | 网口 SocketComm | 串口 SerialPortComm |
|-----|-----------------|---------------------|
| `SendData(vector<double>, CommType)` | 按 `iProtoType` 组包；受 `bInquireSendFlag` 门闩 | 按 `iProtocolType` 组包；同样有许可逻辑 |
| `SendData(QString, CommType)` | **可靠**：`sendData` 写出 | **不可靠**：**空 `override {}`**，不会写串口 |

因此：

- 串口控制帧 `3D0D`：串口 `SendData(QString)` 在库内为空实现；**Demo 不提供串口文本发送**。收侧由库解析为 `emitEventMsg`。  
- 网口控制文本：使用 `SendData(QString, NETWORK)`（库有实现）。

---

## Q5. 验收人：`bInquireSendFlag` 到底管什么？踩坑点？

**答：**

1. `SendData(vector)` 开头为 false → **直接 return，无包**。  
2. 网口若 `DataOutput.iTransferType == 1`（轮询语义），**成功发送一次后库会把许可置 false**，必须再 `setParameter(true)`。  
3. CommLab：串口连接成功且协议支持发送时会置 `bInquireSendFlag`；UDP 用界面勾选控制。

界面操作：关发送许可 → `SendData` 无包；开许可 → 正常发包。

---

## Q6. 验收人：回调 `emitNewData` 用法验收点是什么？

**答：**

1. `size` = **double 个数**，不是字节数。  
2. `data` 在槽返回后不可再解引用（库可能已 `delete`）。  
3. 必须 `DirectConnection` + 立即 `memcpy`，或等价同步拷贝。  
4. `size<=0` 不是测量（CommLab 记「忽略空回调」）。  

错误示范：把 `void*` 经 `QueuedConnection` 传到 UI 再读 —— **悬空指针**。

---

## Q7. 验收人：Connect / Disconnect 参数？

**答：**

- 网口/串口：`Connect()` / `Disconnect(0)` 关闭当前即可。  
- 电压：`Connect(type)` 中 `type`：`0=AD, 1=DA, 2=CI, 3=DI`（见 `CommBase` 注释）。  
- 门面 `Disconnect(-1)` 意图为关全部模块（见 `CommHandler.cpp` 循环）。

Connect 失败常见：COM 被占、UDP 端口冲突、IP 非法、无权限。

---

## Q8. 验收人：信号还有哪些要接？

**答：**

| 信号 | 验收时看什么 |
|------|----------------|
| `emitEventMsg` | 开始/停止/清零/存图等 `W_CUSTOM_*` |
| `emitEventMsgAndData` | 带 `QVariantMap` 的标距类（纳百川等） |
| `emitNewConn` | iType：0 TCP Server … 3 UDP Client；ip/port |
| `emitClientDisConn` | 对端断开 |

---

# 第二部分：数据如何定义、如何进回调

## Q9. 验收人：「数据怎么定义」——在宿主侧用什么类型表示测量？

**答：统一用 `std::vector<double>`（或 `QVector<double>` 再转）。**

库不关心业务名叫「力还是位移」，只认协议组包规则：

```cpp
std::vector<double> payload = { force, move, strain };
comm.SendData(payload, SERIAL); // 或 NETWORK
```

含义完全由 **当前协议号** 的组包函数解释（见第三、四部分）。

回调侧定义：

```cpp
QVector<double> values(size);
memcpy(values.data(), data, size * sizeof(double));
```

---

## Q10. 验收人：`Data` 结构在线上是什么？

**答（`UIDef.h`）：**

```cpp
struct Data {
    double dForce;   // 力
    double dMove;    // 位移
    double dStrain;  // 应变
}; // sizeof == 24
```

**UDP 三思**：对端应发 24 字节；库校验 `size==sizeof(Data)` 后只把 **`dForce` 作为 length=1 的回调**发出。位移/应变在当前解析里未进入回调。

组包示例：

```cpp
Data d{100.5, 1.0, 0.5};
QByteArray pkt(reinterpret_cast<const char*>(&d), sizeof(Data));
```

---

## Q11. 验收人：`DataInput` / `DataOutput` 验收时最关键哪几项？

**答：**

| 字段 | 影响 |
|------|------|
| `input.iTriggerCtrl` | ≠0 才让串口协议 0 把 `3D0D` 等转成事件；中机/JSON 部分开始逻辑也读它 |
| `input.iChannelSize` | JSON 回包构造标距数组等会用 |
| `output.iTransferType` | 0 流 / 1 轮询；网口发后关许可与此相关 |
| `output.iChannelSize` | 业务通道数 |

CommLab 最小写法见 Q3。

---

## Q12. 验收人：中机二进制结构怎么定义？包长多少？

**答（`#pragma pack(1)`）：**

| 结构 | 长度 | 布局 |
|------|------|------|
| `ControlMessage` | **2** | `uint8_t msgType` + `uint8_t command` |
| `ForceData` | **33** | `msgType` + `double[4]` |
| `ServerData` | **17** | `msgType=0x04` + `valueX` + `valueY` |

控制例：开始 `01 0A`；停止 `01 0B`。

---

## Q13. 验收人：`setParameter("sDestIP")` 和结构体 `sPurIP` 什么关系？

**答：** SocketComm setters 把 **`sDestIP` / `iDestPort` 写入内部 `sPurIP` / `iPurPort`**。  
Demo 与文档统一用 `sDestIP`/`iDestPort` 即可；读结构体文件字段时看到 `sPurIP` 不要慌。

---

## Q14. 验收人：测量从线路到业务的完整链路？

**答：**

```
线上海字节
  → SocketComm/SerialPortComm 收槽
  → switch(协议号) 解析
  → new double[] / 填值
  → emitNewData(ptr, count, type)
  → CommHandler 再 emit
  → 你的槽 memcpy
  → UI / 业务
  →（库 delete 缓冲）
```

反向发送：

```
vector<double>
  → SendData
  → 若许可关：return
  → switch 协议 → CreateXxxPack / FloatToHex ...
  → write / sendData
```

---

## Q15. 验收人：空回调、非法包各是什么表现？

**答：**

| 情况 | 表现 |
|------|------|
| `size<=0` | 无有效值；CommLab `[收] 忽略空数据回调` |
| UDP 三思长度≠24 | **不**进 `emitNewData` |
| 中机长度既不是 2 也不是 33 | 无对应分支处理 |
| JSON 非法 | `fromJson` 失败则无业务副作用（视调用） |

---

## Q16. 验收人：发送许可 + 轮询模式怎样完整演示？

**答：**

```cpp
comm.setParameter(QStringLiteral("bInquireSendFlag"), false);
comm.SendData(v, NETWORK); // 对端不应收到

comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
comm.SendData(v, NETWORK); // 应收到

// 若 output.iTransferType==1，此后许可可能被库清掉，需再次 set true
```

用 `UdpPeerSimulator` 或 MachinePeer 监听目的端口验证。

---

# 第三部分：网口协议与数据包

## Q17. 验收人：网口现行协议表？与 Demo 下拉是否一致？

**答：一致（对齐 SocketComm 现行 switch）。**

| iProtoType | 名称 | 数值发 | Demo |
|------------|------|--------|------|
| 0 | JSON | ✅ | ✅ |
| 1 | 万测 | ✅ | ✅ |
| 2 | 中机 | ✅（恰 2 个数） | ✅ |
| 3 | 三思 | ❌ | ✅ |
| 4 | 触发存图 | ❌ | ✅ |
| 5 | 福建威盛 | ❌ | ✅ |
| 6 | 纳百川全自动 | ✅ | ✅ |
| 7 | 纳百川识别线条 | ✅（恰 4 个数） | ✅ |

旧联恒：**不对齐**。

---

## Q18. 验收人：请给出网口 UDP 配置完整示例（JSON）。

**答：**

```cpp
CommHandler comm;
comm.SetCommType(NETWORK);
comm.setParameter(QStringLiteral("iTransferType"), 1);
comm.setParameter(QStringLiteral("iModel"), 0);
comm.setParameter(QStringLiteral("sLocalIP"), QStringLiteral("127.0.0.1"));
comm.setParameter(QStringLiteral("iLocalPort"), 19105);
comm.setParameter(QStringLiteral("sDestIP"), QStringLiteral("127.0.0.1"));
comm.setParameter(QStringLiteral("iDestPort"), 19106);
comm.setParameter(QStringLiteral("iProtoType"), 0);
comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
if (!comm.Connect()) return;

std::vector<double> v{1.0, 2.0, 3.0};
comm.SendData(v, NETWORK);
// 线上海例：
// {"code":0,"values":[1.0,2.0,3.0],"tn":2}
```

---

## Q19. 验收人：JSON / 万测 / 中机发包各长什么样？

**答：**

**JSON 发：**

```json
{"code":0,"values":[...],"tn":2}
```

**万测发：** 文本拼接，例 `1.1,2.2,3.3`（`iSymbolType`：1 `,` / 2 空格 / 3 `;`）。  
收命令：与配置的 `cCMD[0..2]` 字符串完全相等才触发开始/停止/退出。

**中机发：** 17 字节 `ServerData`（`0x04` + 两个 double）。  
**中机收力：** 33 字节 `ForceData`（`0x02` + 4 个 double）→ 回调 size=4。  
**中机控制：** 2 字节，如 `01 0A` 开始。

---

## Q20. 验收人：三思 UDP 完整验收步骤（含包）？

**答：**

1. `iProtoType=3`，`Connect`。  
2. 对端发送 24 字节 `Data{force,move,strain}`。  
3. 期望：`emitNewData`，`size==1`，值≈`force`。  
4. 再发 10 字节垃圾：不应增加有效测量次数。

对端须经库 `SendData` 发出可被解析的帧；本协议库无 vector 发送时 Peer 按钮灰显。

---

## Q21. 验收人：协议 4/5/6/7 各怎样验？

**答：**

| 协议 | 怎么验 |
|------|--------|
| 4 触发存图 | 对端发含 `\r\n` 的文本 → 事件「信号触发存图」 |
| 5 福建威盛 | 发 `TL123.45E...` 类文本 → 回调力值 |
| 6 纳百川 | 发 `SendData` → JSON name=`LVEComputedValues`；收 `{"name":"alphaStartTest",...}` → 开始事件 |
| 7 纳百川线条 | `SendData` **4** 个数 → `{"hsm":{L1,b1,Le1,bo1,...}}`；收 `calcStart`/`calcEnd` |

完整 JSON 字段见《详解》§6.7–6.8。

---

## Q22. 验收人：网口为什么选了三思却点不了「发送数值」？

**答：** 库对 `iProtoType=3` **没有** `SendData(vector)` 实现。Demo 按 `ProtoGuide::cap` 灰显发送按钮；换 0/1/2/6/7 或仅做接收验收。

---

## Q23. 验收人：中机 `SendData` 写了三个数会怎样？

**答：** `CreateZhongJiShuangZhouFour` 要求 `size==2`，否则返回空串，**最终不调用底层 send**。验收时应故意测：错长度 ↔ 对端零收包。

---

## Q24. 验收人：网口字符串发送完整示例？

**答：**

```cpp
comm.SetCommType(NETWORK);
// ... 配好 IP 端口与 iProtoType ...
comm.Connect();
comm.SendData(QStringLiteral("trigger\r\n"), NETWORK); // 可用于协议 4
```

---

# 第四部分：串口协议与数据包

## Q25. 验收人：串口协议表与发送能力？

**答：**

| iProtocolType | 名称 | 数值发 | 备注 |
|---------------|------|--------|------|
| 0 | 三思 SSCK | ✅ | 指令事件；段长 5/6/8 |
| 1 | 长春科新 | ✅ | 14 字节定长，用 v[0] |
| 2 | 时代新材 | ❌ | `case 2: break`；Demo 禁发 |
| 3 | IEEE | ✅ | 须 ≥5 个数；56 字节 DataPack |
| 4 | 冠腾 | ✅ | `R±000.0000#N±000.0000#` |

---

## Q26. 验收人：三思串口发送数据包如何逐步构造？

**答：**

1. `segSize = m_iDataBits`（与数据位相关；预览常用 8）。  
2. 每个 double → `FloatToHex`：符号 + ASCII 数字/点，空位 `*`。  
3. 所有段拼完后写 **一个** `0x0D`。  
4. `write` 到串口（若许可开）。  

总长 = `segSize * N + 1`。

```cpp
comm.SendData(std::vector<double>{1.0}, SERIAL);
```

算法细节见《详解》§7.1 与 `SerialPortComm::FloatToHex`；出站仅经库 `SendData(vector)`。

---

## Q27. 验收人：三思控制指令完整验收？

**答：帧恒为 2 字节：`命令 + 0D`。**

| HEX | 事件宏 | 含义 |
|-----|--------|------|
| `3D 0D` | STARTCALC | 开始 |
| `3E 0D` | STOPCALC | 停止 |
| `6B 0D` | AUTO_SAVE_IMAGE | 存图 |
| `5A 0D` | ZERO_CLEARING | 清零 |

条件：`iProtocolType==0` 且 `input.iTriggerCtrl!=0`。

库在收侧解析为 `emitEventMsg`。Demo **不提供**串口 `SendData(QString)` 发送；验收收事件时须对端经库发出可被识别的线路数据。

---

## Q28. 验收人：科新 / 冠腾 / IEEE 发包示例？

**答：**

```cpp
// 科新
comm.setParameter(QStringLiteral("iProtocolType"), 1);
comm.SendData(std::vector<double>{12.34567}, SERIAL); // 14 字节

// 冠腾
comm.setParameter(QStringLiteral("iProtocolType"), 4);
comm.SendData(std::vector<double>{12.5, -1.25}, SERIAL);
// 文本类似：R+0012.5000#N-0001.2500#

// IEEE
comm.setParameter(QStringLiteral("iProtocolType"), 3);
comm.SendData(std::vector<double>{0.1, 0.2, 1.0, 2.0, 0.01}, SERIAL); // ≥5
```

中间都必须 `Connect` 且许可为 true（完整参数见 Q3）。

---

## Q29. 验收人：COM 口号怎么对齐物理口？

**答：`iComPort` / Demo `spinComPort` 从 0 计：`0→COM1`，`3→COM4`，`4→COM5`。**  
虚拟串口配对时 CommLab 与 MachinePeer 必须各占一对中的一端，波特率数据位校验停止位一致。

---

## Q30. 验收人：时代新材为什么灰掉发送？

**答：** 库源码发送分支为空。UI 标记「发送未实现」是**对齐库**，不是 Demo 擅自禁用有实现的协议。

---

## Q31. 验收人：三思发送与 DLL 的关系？

**答：** 出站组包由 `SerialPortComm::SendData(vector)` 完成。Demo 不本地预览、不旁路；日志 `[发→ vector]` 表示已调用库 API。

---

# 第五部分：指令与联调流程

## Q32. 验收人：请带我把 UDP 主流程完整走一遍。

**答：逐步验收清单。**

1. 读 `CommLab/netconfig.ini` 与 `MachinePeer/peerconfig.ini`，确认 19105↔19106 交叉。  
2. 建立 COM4↔COM5，先启动 **MachinePeer** → 选 **串口三思**，打开。  
3. 启动 **CommLab** → 串口三思 COM4，连接（含 UDP 指令面）。  
4. CommLab 点 **开始** → Peer `[收← 解包] emitEventMsg`。  
5. Peer 串口 `SendData(vector)` 回发 → CommLab `[收← 解包] emitNewData`。  

纯 UDP JSON：仅可验指令 `tn=1/3`，**不能**验试验机测量回传。

---

## Q33. 验收人：请带我把串口主流程完整走一遍。

**答：**

1. 建立 COM4↔COM5。  
2. CommLab：串口、协议 0、端口序号 3、115200 8N1，连接。  
3. MachinePeer：`port_index=4`，同线格式，打开。  
4. CommLab `SendData(vector)` 发 `1.0` → Peer `[收← data]`。  
5. Peer `SendData(vector)` 回发 → CommLab `[收← data]`。  
6. 串口控制事件：库收侧解析；Demo 不提供串口文本发送按钮。

---

## Q34. 验收人：界面如何自检发送许可？

**答：** CommLab 取消「发送许可」→ `SendData` 调用后线上无包；重新勾选 → 对端应收到。无需 CLI 冒烟命令。

---

## Q35. 验收人：只有本端事件、对端没接收，算通过吗？

**答：不算指令发出验收通过。**  
必须对端或抓包证明线上海存在该帧。事件可能来自本地/回环/误判。

---

## Q36. 验收人：电压 / 脉冲如何口头验收「会用库」？

**答：** 能写出：

```cpp
comm.SetCommType(VOLTAGE);
comm.Connect(0); // AD
// HardTrigger / ReadADData / ReleaseHardTrigger

comm.SetCommType(PULSE);
comm.Connect();
comm.SendCaliData();
```

并说明结构体在 `VoltageInfo`/`PulseInfo`，Demo 无硬件 UI。细节见《详解》§10。

---

# 第六部分：扩展、废弃与验收清单

## Q37. 验收人：联恒协议要不要在 Demo 里对齐？

**答：不要。** 源码中网口/串口旧联恒分支已注释；现 `iProtoType=0` 是 JSON。  
除非 DLL **恢复实现**，否则禁止只加 UI。详见两册「废弃项」。

---

## Q38. 验收人：如何新增一条控制指令（完整清单）？

**答：例 `0xAA 0D`「暂停」——四处对齐：**

1. DLL `SerialPortComm::onRecvDataSlots` 识别 → `emitEventMsg` + `UIDef` 新宏。  
2. CommLab / MachinePeer：`CommController::eventName` + 日志。  
3. 若库有发送 API 则 Demo 暴露对应按钮；无则不做。  
4. 手册与 `ProtoGuide::cap` 更新。

---

## Q39. 验收人：如何新增一个网口协议号？

**答：**

1. `SocketComm::onRecvDataSlots` 加 `case N`。  
2. 需要发则 `SendData(vector)` 加 case + `CreateXxx`。  
3. 更新 UIDef / 手册 / CommLab 下拉与 `ProtoGuide::cap`。

---

## Q40. 验收人：仓库文档怎么读？

**答：**

1. [根 README](./README.md) — 边界、怎么编译。  
2. **本册** — 验收问答与完整示例（当前文）。  
3. [《动态库使用与数据包详解》](./CommHandler-动态库使用与数据包详解.md) — 字段、字节、逐协议圣经。  
4. `CommLab/README.md` / `MachinePeer/README.md` — 工程操作。

---

## Q41. 验收人：交付验收打钩清单？

**答：**

- [ ] 知道四通道与门面 API  
- [ ] 能独立写出 Connect 前后完整参数（串口或网口其一）  
- [ ] 能说明 `vector<double>` → 某协议线上形态  
- [ ] 能说明 `emitNewData` 拷贝规则  
- [ ] 知悉串口 `SendData(QString)` 库为空、Demo 不提供  
- [ ] UDP JSON 双向与许可门闩界面可验  
- [ ] 对端日志 `[收← data]` / `[收← event]` 能对照  
- [ ] 现行 0–7 / 0–4 表能默写大概能力  
- [ ] 废弃联恒不对齐说得出  
- [ ] 知道去哪改 DLL 才能真加协议  

---

## Q42. 验收人：10 道快问快答

1. `iProtoType=3` 能数值发吗？→ **否**  
2. 串口 `SendData(QString)`？→ **空**  
3. 中机数值发几个数？→ **2**  
4. 协议 7 几个数？→ **4**  
5. 三思 UDP 帧长？→ **24**  
6. `iComPort=4`？→ **COM5**  
7. `sDestIP` 写入？→ **sPurIP**  
8. 时代新材能发？→ **否**  
9. 许可关了 vector 发送？→ **无出站**  
10. 对端收到库回调？→ **`[收← data]` / `[收← event]`**

---

## 附录：事件宏速查

见《详解》§8.2 与 `UIDef.h`：`W_CUSTOM_COMM_STARTCALC` … `W_CUSTOM_COMM_AUTO_SAVE_IMAGE` 等。

## 附录：与《详解》章节映射

| 本册 | 《详解》 |
|------|----------|
| Q1–Q8 | §1–§3 |
| Q9–Q16 | §4–§5 |
| Q17–Q24 | §6 |
| Q25–Q31 | §7 |
| Q32–Q36 | §8–§10 |
| Q37–Q42 | §12–§14 |

---

*两册内容对齐现行 SocketComm/SerialPortComm；库变更时请同步改两份文档。*
