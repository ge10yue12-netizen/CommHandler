# CommHandler 兼容协议说明

| 项 | 内容 |
|---|---|
| 文档标识 | CA-DEV-LEGACY-PROTO-001 |
| 基线 | V1.2.5 |
| 适用范围 | 兼容动态库（CommHandler）串口 / 网口协议编解码语义 |
| 关联 | `docs/COMMUNICATION_ASSISTANT_DEV_GUIDE.md`、`docs/COMM_TEST_CASE_MANUAL.md` |

---

## 1. 关键语义

1. 助手「发送工作台」中的数值字符串（如 `12.3,45.6`）为**业务入参**，不是线帧。  
2. 线帧由 CommHandler 按协议索引编码后写出；库接口不向助手回传已发送原始字节。  
3. 对端以文本模式查看二进制线帧出现乱码，属预期现象；应以十六进制核对。  
4. 串口协议索引与网口协议索引**相互独立**（例如串口 0「三思」≠ 网口 3「三思」）。

---

## 2. 发送数据流

```text
业务入参（CSV / 文本）
  → LegacySession 能力校验
  → CommHandler::SendData(std::vector<double>) 或文本接口
  → SerialPortComm / SocketComm 协议分支
  → 串口 write / NetworkBox::sendData
```

无发送能力时，助手拒绝发送并记录边界日志，不得向链路写出。

---

## 3. 接收数据流

```text
链路字节
  → 协议解析
  → 成功：数值 / 控制事件
  → 失败：emitUnparsedRx → 助手展示十六进制线帧与「协议解析失败」日志
```

---

## 4. 网口协议（iProtoType）

| 索引 | 名称 | 发数值 | 发送线帧 | 收数值 | 不匹配接收 |
|---|---|---|---|---|---|
| 0 | JSON | 是 | UTF-8 JSON | 否（控制为主） | 非 JSON 对象 → 未解析 |
| 1 | 万测 | 是 | 分隔符数值文本 | 否（控制为主） | 未匹配控制字 → 未解析 |
| 2 | 中机 | 是（恰好 2） | 二进制 `ServerData`（pack 1）：`uint8=0x04` + `double`×2，共 17 字节；`sendData(char*,len)` 直发 | 是（定长结构） | 长度不匹配 → 未解析 |
| 3 | 三思 | **否** | 动态库无 case 3，不写出 | 是（`sizeof(Data)`） | 长度不匹配 → 未解析 |
| 4 | 触发存图 | 否 | — | 否 | 无 `\r\n` → 未解析 |
| 5 | 福建威盛 | 否 | — | 是（`T/S…L…`） | 解析失败 → 未解析 |
| 6 | 纳百川自动化 | 是 | JSON | 控制/参数 | 依实现 |
| 7 | 纳百川线条 | 文本为主 | JSON | 控制 | 依实现 |
| 8 | 联恒光科 | 是（≥2：力、温） | `AA 55 … CRC 0D 0A`（`NetLhgkProtocol`，BE） | 是 | 流式拼帧；断帧等待；完整坏帧 → 未解析 |

### 4.1 中机

- 入参：水平、垂向各一个 double。  
- 线帧：`04` + IEEE754 小端 `valueX` + `valueY`。  
- 实现约束：禁止经 `QString`/`toLatin1` 转发二进制。

### 4.2 联恒光科（网口）

- 入参：至少力、温两路。  
- 线帧：帧头 `AA 55`，类型 `0x01`，双段模块数据，CRC16-XMODEM（大端），尾 `0D 0A`。  
- 对端十六进制应可见 `AA 55`。  
- **TX/RX 分叉（当前实现）**：发送 `buildExternalDataFrame` 的 length 为 payload 字节数、力/温类型为 `0x0B/0x0C` 且带段计数字节；接收 `checkAndParseFrame` 将 length 当段数、`getExternalData` 认 `0x0C/0x0D` 且无段计数字节。黄金用例见 `docs/COMM_TEST_CASE_MANUAL.md` §5.9 与 `docs/fixtures/comm/NET8_*`。  
- 不得将 TX 黄金帧直接环回作为 RX 正例。

### 4.3 三思（网口）

- 发送：能力关闭。若业务需要发送，须先在 CommHandler 增加编码分支并更新能力表。  
- 接收：仅定长 `Data` 结构解析。

---

## 5. 串口协议（iProtocolType）

| 索引 | 名称 | 发数值 | 发送线帧 | 收数值 | 不匹配接收 |
|---|---|---|---|---|---|
| 0 | 三思 | 是 | 定宽 ASCII（`FloatToHex`）+ `0x0D` | 是 | 拼帧完成仍失败 → 未解析 |
| 1 | 科新 | 是 | 定长编码段 | 是 | 依拼帧 |
| 2 | 时代新材 | 否 | — | 是（`,` + `\r\n`） | 段不符 → 未解析 |
| 3 | IEEE | 是（≥5） | 专用打包写出 | 控制为主 | 非控制 → 未解析 |
| 4 | 冠腾 | 是（≥2） | ASCII `R…#N…#` | 否 | 默认未解析 |
| 5 | 联恒光科 | 是（≥2） | `AA 55 … 0D 0A`（`SerLhgkProtocol`，LE） | 是 | 同网口联恒流式规则 |

---

## 6. 能力表

实现文件：`CommunicationAssistant/legacy/LegacyCapability.cpp`。

界面「协议能力」区展示支持项与限制说明。二进制协议须提示对端以十六进制核对。

---

## 7. 变更纪律

协议索引、线帧布局或能力开关变更时，须同步：

1. `docs/BASELINE.md`  
2. 本说明  
3. `docs/COMM_TEST_CASE_MANUAL.md`  
4. `docs/fixtures/comm/` 黄金 HEX/TXT  
5. `LegacyCapability.cpp`  
6. 相关审计报告  

---

## 8. 修订记录

| 日期 | 说明 |
|---|---|
| 2026-07-21 | 专业表述重写；与测试手册、开发说明对齐 |
| 2026-07-21 | 补充联恒网口 TX/RX 分叉说明；变更纪律纳入 fixtures |
