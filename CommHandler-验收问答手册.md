# CommHandler 动态库学习与验收手册

> **目的**：快速看懂并上手使用 `CommHandler.dll`（以本机源码 `g:\My_work\2026-7\7-10\CommHandler` 为真理源）。  
> **Demo**：`Demo_test/CommLab`（链接 DLL）+ `MachinePeer`（无 DLL 的线协议对端）。  
> **对齐原则**：只对齐库里**现行可运行**的协议与 API；源码里**已注释废弃**的分支在文档「废弃项」说明，**不接进 Demo UI**。

---

## 0. 30 秒心智模型

```
你的程序
  └─ CommHandler（门面）
        ├─ NETWORK → SocketComm     ← iProtoType = 0..7
        ├─ SERIAL  → SerialPortComm ← iProtocolType = 0..4
        ├─ VOLTAGE → VoltageComm    ← Connect(type=AD/DA/CI/DI)
        └─ PULSE   → PulseComm      ← 脉冲卡 / 标定
```

统一用法三步：

```cpp
comm.SetCommType(NETWORK);           // 或 SERIAL / VOLTAGE / PULSE
comm.setParameter("key", value);     // 见各通道参数表
comm.Connect();
// 收：监听 emitNewData / emitEventMsg
// 发：SendData(vector, type) 或（仅网口）SendData(QString, NETWORK)
comm.Disconnect(0);
```

---

## 目录

1. [门面 API 怎么用](#1-门面-api-怎么用)
2. [网口全部现行协议（0–7）](#2-网口全部现行协议07)
3. [串口全部现行协议（0–4）](#3-串口全部现行协议04)
4. [电压 / 脉冲通道](#4-电压--脉冲通道)
5. [控制指令与事件宏](#5-控制指令与事件宏)
6. [参数 Key 速查](#6-参数-key-速查)
7. [如何新增 / 修改协议与指令](#7-如何新增--修改协议与指令)
8. [CommLab / MachinePeer 对应哪里](#8-commlab--machinepeer-对应哪里)
9. [废弃项（不对齐 Demo）](#9-废弃项不对齐-demo)
10. [联调与冒烟](#10-联调与冒烟)
11. [验收自测题](#11-验收自测题)

---

## 1. 门面 API 怎么用

### 1.1 类关系

| 类 | 文件 | 职责 |
|----|------|------|
| `CommHandler` | `CommHandler.h/.cpp` | 选通道、转发 `setParameter` / `Connect` / `SendData`、汇总信号 |
| `CommBase` | `CommSdk/CommBase.h` | 抽象基类；`SendData(QString)` **默认空实现** |
| `SocketComm` | `CommSdk/SocketComm.*` | TCP/UDP + `iProtoType` |
| `SerialPortComm` | `CommSdk/SerialPortComm.*` | 串口 + `iProtocolType` |
| `VoltageComm` / `PulseComm` | 同目录 | 采集卡 / 脉冲 |

### 1.2 必须掌握的信号

| 信号 | 含义 | 注意 |
|------|------|------|
| `emitNewData(void*, int size, int type)` | `size` = **double 个数** | `void*` 仅在槽返回前有效；CommLab 用 `DirectConnection` + `memcpy` |
| `emitEventMsg(ctrlCmd, viewId, msg)` | 控制/状态事件 | `msg` 常为 `W_CUSTOM_*` |
| `emitNewConn(iType, ip, port)` | 新连接 | iType：0 TCP Server … 3 UDP Client |
| `emitClientDisConn()` | 对端断开 | |

### 1.3 发送 API 差异（必读）

| API | 网口 SocketComm | 串口 SerialPortComm |
|-----|-----------------|---------------------|
| `SendData(vector<double>, CommType)` | 按 `iProtoType` 组包（见下表「数值发」列） | 按 `iProtocolType` 组包 |
| `SendData(QString, CommType)` | **有效**：`m_controlBox.sendData(data)` | **空**：`override {}`，写不上线 |

因此 CommLab：

- **网口控制字节**：可直接 `writeDatagram`，或理论可用 `SendData(QString, NETWORK)`；
- **串口控制字节**：必须旁路 `QSerialPort`（先 `Disconnect` 释放占用）。

### 1.4 最小示例（网口 JSON 发三值）

```cpp
CommHandler comm;
comm.SetCommType(NETWORK);
comm.setParameter(QStringLiteral("iTransferType"), 1);      // UDP
comm.setParameter(QStringLiteral("iModel"), 0);              // 服务端
comm.setParameter(QStringLiteral("sLocalIP"), QStringLiteral("127.0.0.1"));
comm.setParameter(QStringLiteral("iLocalPort"), 19105);
comm.setParameter(QStringLiteral("sDestIP"), QStringLiteral("127.0.0.1")); // 内部写入 sPurIP
comm.setParameter(QStringLiteral("iDestPort"), 19106);
comm.setParameter(QStringLiteral("iProtoType"), 0);          // JSON
comm.setParameter(QStringLiteral("bInquireSendFlag"), true);
if (!comm.Connect()) return;
std::vector<double> v{1.0, 2.0, 3.0};
comm.SendData(v, NETWORK);
// 线上海报文类似：{"code":0,"values":[1.0,2.0,3.0],"tn":2}
```

### 1.5 最小示例（串口三思收事件）

连接后若对端发来 `3D 0D`，且 `input.iTriggerCtrl != 0`：

```text
emitEventMsg(..., W_CUSTOM_COMM_STARTCALC)
```

CommLab 日志：`[接收][事件] 开始计算`。

---

## 2. 网口全部现行协议（0–7）

真理源：`SocketComm::onRecvDataSlots` / `SocketComm::SendData`。

| `iProtoType` | 名称 | 收 | 数值发 | 发包形态（简述） |
|--------------|------|----|--------|------------------|
| **0** | JSON | ✅ | ✅ | 发：`CreateJsonDataPack` → `{"code":0,"values":[…],"tn":2}` |
| **1** | 万测 | ✅ | ✅ | 发：数值用 `,` / 空格 / `;` 拼接（`iSymbolType` 1/2/3） |
| **2** | 中机四通道 | ✅ | ✅ | 发：恰 **2** 个 double → `ServerData` 二进制；收：力包 4 通道 double |
| **3** | 三思试验 | ✅ | ❌ | 收：恰 `sizeof(Data)==24` 字节，回调 `size=1`（力） |
| **4** | 触发存图 | 事件 | ❌ | 报文含 `\r\n` → `W_CUSTOM_COMM_SINGALTRIIMAGESAVE` |
| **5** | 福建威盛 | ✅ | ❌ | 文本以 `T`/`S` 开头，解析载荷力值 |
| **6** | 纳百川全自动 | ✅ | ✅ | 发：`{"values":[…],"name":"LVEComputedValues"}` |
| **7** | 纳百川识别线条 | ✅ | ✅ | 发：须 **4** 个 double → 专用 JSON |

### 2.1 协议 0 JSON — 怎么用

```cpp
comm.setParameter("iProtoType", 0);
comm.setParameter("bInquireSendFlag", true);
comm.SendData(std::vector<double>{10.5, 0.1}, NETWORK);
```

对端用文本工具即可看见紧凑 JSON。CommLab：`--smoke-send` 验证许可开关。

### 2.2 协议 1 万测 — 怎么用

```cpp
comm.setParameter("iProtoType", 1);
comm.setParameter("iSymbolType", 1); // 1=,  2=空格  3=;
comm.SendData(std::vector<double>{1.1, 2.2, 3.3}, NETWORK);
// 例如线上海：1.1,2.2,3.3
```

### 2.3 协议 2 中机 — 怎么用

结构定义在 `UIDef.h` → `SocketProtol`（与南航 DIC 控制字同源）：

- 控制：`msgType=0x01` + `command`（如 `0x0A` 开始）
- 力值：`msgType=0x02` + `double[4]`
- 软件发测量：`msgType=0x04` + `valueX/valueY`（**SendData 要求 vector 长度 = 2**）

```cpp
comm.setParameter("iProtoType", 2);
comm.SendData(std::vector<double>{12.0, 34.0}, NETWORK); // 恰好 2 个，否则库返回空串不发
```

### 2.4 协议 3 三思 — 怎么用（仅收）

```cpp
comm.setParameter("iProtoType", 3);
comm.Connect();
// 对端发 24 字节：double force, move, strain
// 本端 emitNewData：通常 size=1，值为 force
```

CommLab / MachinePeer：`packSansiUdp` / `UdpPeerSimulator::makeSansiPacket`。  
`--smoke`：合法 24B 有回调；非法 10B 无回调。

### 2.5 协议 4 触发存图 — 怎么用

对端发任意含 `\r\n` 的文本 → 事件「信号触发存图」。无数值 `SendData`。

### 2.6 协议 5 福建威盛 — 怎么用

文本示例形态：`TL123.45E…`（以 `T`/`S` 开头）。解析成功后 `emitNewData` 力值。Demo 未做专用模拟器，联调需真实设备或按格式手搓。

### 2.7 协议 6 / 7 纳百川 — 怎么用

```cpp
// 6
comm.setParameter("iProtoType", 6);
comm.SendData(std::vector<double>{1,2,3}, NETWORK);

// 7 — 必须 4 个数
comm.setParameter("iProtoType", 7);
comm.SendData(std::vector<double>{L1, b1, Le1, bo1}, NETWORK);
```

### 2.8 网口通用连接参数

| Key | 含义 | 示例 |
|-----|------|------|
| `iTransferType` | 0 TCP / 1 UDP | `1` |
| `iModel` | 0 服务端 / 1 客户端 | `0` |
| `sLocalIP` / `iLocalPort` | 本机绑定 | `127.0.0.1` / `19105` |
| `sDestIP` / `iDestPort` | 目的（setter 写入内部 `sPurIP`/`iPurPort`） | `19106` |
| `iProtoType` | 上表 | `0`…`7` |
| `bInquireSendFlag` | 允许数值发送 | `true` |
| `iSymbolType` | 万测分隔符 | `1` |

---

## 3. 串口全部现行协议（0–4）

真理源：`SerialPortComm::SendData(vector)` / `onRecvDataSlots`。

| `iProtocolType` | 名称 | 数值发 | 说明 |
|-----------------|------|--------|------|
| **0** | 三思 SSCK | ✅ | 每值 `segSize`(5/6/8) ASCII + 末 `0D`；可识别 `3D0D` 等指令事件 |
| **1** | 长春科新 | ✅ | 14 字节定长浮点编码 |
| **2** | 时代新材 | ❌ | `case 2: break;` 空实现 — Demo UI 禁发 |
| **3** | IEEE | ✅ | 要求 `vData.size()>=5`，走 `SendData2ComPort` + 56 字节包 |
| **4** | 冠腾 | ✅ | ASCII：`R{力}#N{位移}#` |

### 3.1 协议 0 三思 — 帧示例

`segSize=8`，值 `1.0`：

```text
2B 30 2E 30 30 30 2A 2A 0D
 +  0  .  0  0  0  *  *  CR
```

连接参数示例：

```cpp
comm.SetCommType(SERIAL);
comm.setParameter("iComPort", 3);        // COM4
comm.setParameter("iComBaud", 5);      // 115200
comm.setParameter("iDataBits", 3);     // 8
comm.setParameter("iParity", 0);
comm.setParameter("iStopBits", 0);
comm.setParameter("iProtocolType", 0);
comm.setParameter("dInForceRange", 100.0);
DataInput in{}; in.iChannelState = 1; in.iTriggerCtrl = 1;
comm.setParameter("input", in);
comm.setParameter("bInquireSendFlag", true);
comm.Connect();
comm.SendData(std::vector<double>{1.0}, SERIAL);
```

### 3.2 协议 1 / 3 / 4 提示

- **科新**：`SendData` 取 `vData[0]` 打 14 字节。  
- **IEEE**：至少 5 个数（应变×2、位移×2、时间等，按库实现）。  
- **冠腾**：至少 2 个数组成 `R…#N…#`。

### 3.3 串口 `SendData(QString)` 

库内 `SerialPortComm::SendData(const QString&) override {}` —— **故意空**。控制指令不要调用它。CommLab 旁路：`Disconnect` → 临时口写 `cmd+0D` → `Connect`。

---

## 4. 电压 / 脉冲通道

Demo **未做 UI**（无采集卡环境），但学习库必须知道入口。

### 4.1 VOLTAGE

```cpp
comm.SetCommType(VOLTAGE);
// Connect(type)：0=AD 1=DA 2=CI 3=DI（见 CommBase 注释）
comm.Connect(0);
bool HardTrigger(double value, double dutyCycle);
bool ReleaseHardTrigger();
bool ReadADData(double* pdData, double* pGainData, double* truthData, bool bGetVolt=false);
bool ReadCIFreq();
```

参数结构见 `UIDef.h` → `VoltageInfo`（AD 16 通道、DA 2 通道、标定数组等）。用 `setParameter` / `SetSysParInfo` 注入。

### 4.2 PULSE

```cpp
comm.SetCommType(PULSE);
comm.Connect();
comm.SendCaliData();   // 门面快捷：发标定
```

参数见 `PulseInfo`（卡类型、精度、帧率、USB/NET 速度等）。事件含 `W_CUSTOM_COMM_UPDATEPULSECTRL`、`PULSECALIDONE` 等。

---

## 5. 控制指令与事件宏

### 5.1 串口线指令（协议 0 识别）

| 帧 HEX | 事件宏 | 含义 |
|--------|--------|------|
| `3D 0D` | `W_CUSTOM_COMM_STARTCALC` | 开始计算 |
| `3E 0D` | `W_CUSTOM_COMM_STOPCALC` | 停止计算 |
| `6B 0D` | `W_CUSTOM_COMM_AUTO_SAVE_IMAGE` | 自动存图 |
| `5A 0D` | `W_CUSTOM_ZERO_CLEARING` | 清零 |

另有标距等以 `47…` 开头的扩展帧（见 `SerialPortComm::onRecvDataSlots`）。

### 5.2 常用 `W_CUSTOM_*`（`UIDef.h`）

| 宏 | 用途 |
|----|------|
| `W_CUSTOM_COMM_EXITPROG` | 退出程序 |
| `W_CUSTOM_COMM_DATASTREAMING` | 数据流开/关 |
| `W_CUSTOM_COMM_DATAPOLLING` | 轮询 |
| `W_CUSTOM_COMM_SINGALTRIIMAGESAVE` | 信号触发存图（网口协议 4） |
| `W_CUSTOM_COMM_RESET_LVE_LENGTH` / `CALC_LVE_LENGTH` | 标距 |
| 脉冲相关 | `UPDATEPULSECTRL` / `PULSECALIDONE` / `PULSEBUTTON` |

### 5.3 中机控制字（`SocketProtol`）

| 值 | 含义 |
|----|------|
| `0x0A` | START_CALC |
| `0x0B` | STOP_CALC |
| `0x0C` | DATA_COLL_POLLING |
| `0x0D` | DATA_COLL_FLOW |
| `0x0E` | ACQ_STATE |
| `0x64`… | 返回状态（已连接 / 相机未开 / …） |

---

## 6. 参数 Key 速查

串口常用：`iComPort` `iComBaud` `iDataBits` `iParity` `iStopBits` `iProtocolType` `dInForceRange` `bInquireSendFlag` `input` `output`

网口常用：`iTransferType` `iModel` `sLocalIP` `iLocalPort` `sDestIP` `iDestPort` `iProtoType` `iSymbolType` `bInquireSendFlag` `input` `output` `bOpenCMD` / `cCMD`（自定义命令串，视模块使用）

`setParameter` 失败：先当前模块，再遍历四通道；全失败抛 `runtime_error`。

---

## 7. 如何新增 / 修改协议与指令

### 7.1 新增网口协议（必须改 DLL）

1. `SocketComm::onRecvDataSlots` 增加 `case N:` + 解析函数  
2. 若需发：`SendData(vector)` 增加 `case N:` + `CreateXxxPack`  
3. 更新 `UIDef.h` 注释与产品文档  
4. **然后** CommLab `fillCombos` 增加一项（`UserRole+1` 标记可否发）  
5. 需要时给 MachinePeer / 冒烟补对端模拟  

只改 Demo 不改 DLL → 界面有选项但库 `default: break`，无效。

### 7.2 新增串口协议

同理改 `SerialPortComm` 的收/发 `switch (iProtocolType)`，再暴露 UI。

### 7.3 新增控制指令（例：`0xAA 0D` 暂停）

| 层 | 改动 |
|----|------|
| DLL | `SerialPortComm::onRecvDataSlots` 识别并 `emitEventMsg`；可选 `UIDef` 新宏 |
| CommLab | `sendControlCommand(0xAA, "暂停")` + 按钮；`eventName` |
| MachinePeer | `WireCodec::classifySerialCommand` |

### 7.4 修改已有指令码

软件、DLL、试验机、文档 **四处同步**，只改一处必联调失败。

---

## 8. CommLab / MachinePeer 对应哪里

| 学习目标 | 改 / 看哪里 |
|----------|-------------|
| 调 DLL 连接参数 | `MainWindow::applyUdpParams` / `applySerialParams`，`NetConfig.cpp` |
| 看回调安全拷贝 | `CommController.cpp` |
| 数值发送 | `MainWindow::onSendClicked` → `SendData` |
| 串口指令旁路 | `sendControlCommandViaSerialBypass` |
| 网口指令原始发 | `sendControlCommandViaUdpRaw`（串口字符串发送在库为空；网口库可用但仍旁路一致） |
| 三思帧对照 | `SerialFramePreview`（对齐 `SerialPortComm` 协议 0，不调用 DLL） |
| 试验机线协议 | `MachinePeer/WireCodec` |

---

## 9. 废弃项（不对齐 Demo）

| 项 | 源码状态 | Demo 策略 |
|----|----------|-----------|
| 网口旧「联恒」`case 0` | `SocketComm` 中整段注释；现 `case 0` 已改为 JSON | **不提供**「联恒」选项；文档标明历史 |
| 串口旧「联恒」发送 | `SerialPortComm::SendData` 中注释 | 同上 |
| `UIDef.h` 旧注释「协议一联恒…协议五三思」 | 与现行 0–7 不符 | Demo 副本已改为现行列表说明 |

若产品仍要联恒，须在 DLL **恢复实现**后再开 Demo 项，禁止只改 UI。

---

## 10. 联调与冒烟

### UDP 角色

| 程序 | 绑定 | 发往 |
|------|------|------|
| CommLab | 19105 | 19106 |
| MachinePeer | 19106 | 19105 |

### CLI

```bat
CommLab.exe --smoke          rem 协议 3 收 24B
CommLab.exe --smoke-send     rem 协议 0 发 + 许可门闩
CommLab.exe --smoke-preview  rem 串口三思预览自检
CommLab.exe --smoke-all
```

### 金标准日志

- 发指令一侧：`[发送][指令] …`  
- **对端**必须出现 `[接收][指令]`；仅本端 `[接收][事件]` 不能证明对端收到。

---

## 11. 验收自测题

1. `iProtoType=3` 能否 `SendData(vector)`？→ **否**  
2. 串口 `SendData(QString)`？→ **空实现**  
3. 中机数值发送几个数？→ **2**  
4. 纳百川协议 7 几个数？→ **4**  
5. 三思 UDP 帧长？→ **24**  
6. 旧联恒要不要对齐 Demo？→ **不要**（已注释）  
7. `sDestIP` 实际写入？→ **`sPurIP`**  
8. 时代新材 `iProtocolType=2` 数值发？→ **否**  
9. 电压 `Connect` 的 type？→ **0 AD / 1 DA / 2 CI / 3 DI**  
10. 协议编号以谁为准？→ **`SocketComm`/`SerialPortComm` switch，不是过时 UIDef 中文序号**

---

## 附录：真理源路径

| 内容 | 路径 |
|------|------|
| 网口收发分支 | `7-10\CommHandler\CommSdk\SocketComm.cpp` |
| 串口收发分支 | `7-10\CommHandler\CommSdk\SerialPortComm.cpp` |
| 结构体 / 事件 | `7-10\CommHandler\Common\UIDef.h`（Demo 副本：`CommLab\include\UIDef.h`） |
| 官方场景样例 | `7-10\CommHandler\test\CommHandlerDemo\scenarios\` |

---

*文档随 DLL 现行 switch 维护；新增协议先改库再改 Demo。*
