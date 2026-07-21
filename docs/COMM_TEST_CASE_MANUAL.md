# 通信调试助手 — 通讯测试用例手册（可执行黄金数据）

| 项 | 内容 |
|---|---|
| 文档标识 | CA-TEST-COMM-001 |
| 适用范围 | CommunicationAssistant（原生通道 / 兼容动态库）及 CommHandler 动态库协议 |
| 基线版本 | V1.2.5 |
| 读者 | 测试、联调、移植与验收人员 |
| 关联文档 | `docs/COMMUNICATION_ASSISTANT_DEV_GUIDE.md`、`docs/LEGACY_PROTOCOL_DEV_GUIDE.md`、`docs/BASELINE.md` |
| 黄金数据目录 | `docs/fixtures/comm/`（与本手册用例一一对应，可直接粘贴到对端工具） |

---

## 1. 目的与范围

本手册提供**可直接执行**的通讯测试用例：每条用例均包含前置条件、可粘贴入参 / 对端载荷、期望线帧（全长 HEX）与判定结果。

覆盖：

1. **原生通道**：串口、TCP 客户端、TCP 服务端、UDP 的连接真值与透明收发。
2. **兼容动态库（CommHandler）**：网口协议索引 0–8、串口协议索引 0–5 的发送入参、线帧、接收解析与失败可观测性。

**不在范围**：电压/脉冲类未开放通讯类型；厂商现场工艺验收。

---

## 2. 如何使用本手册（测试方法）

### 2.1 工具

| 角色 | 建议工具 |
|---|---|
| 被测 | CommunicationAssistant（x64），运行目录部署 **Release** `CommHandler.dll` |
| 对端 | 任意串口/网络调试助手（须支持**十六进制发送与显示**） |
| 载荷 | 打开 `docs/fixtures/comm/*.hex` 或 `*.txt`，整段复制到对端发送区 |

### 2.2 约定

| 项 | 说明 |
|---|---|
| 业务入参 | 助手「发送工作台」填写的 CSV（如 `12.3,45.6`），**不是**线帧 |
| 线帧 | 链路上真实字节；对端必须以 HEX 核对二进制协议 |
| 黄金数据来源 | 按当前 CommHandler 源码编解码推算并经 IEEE754 / CRC16-XMODEM 核算 |
| 重构纪律 | DLL 编码/CRC/类型号变更时，必须同步本手册与 `docs/fixtures/comm/` |
| JSON 字段序 | Qt `QJsonObject` 键序可能变化；判定看语义字段，不强制字节序与示例 HEX 逐字节相同 |

### 2.3 判定代号

| 代号 | 含义 |
|---|---|
| P-CONN | 状态文案与真实连通一致 |
| P-TX-IN | 数据显示区有「入参」发送记录（兼容通道标明由动态库编码） |
| P-PEER-HEX | 对端 HEX 与用例期望线帧**逐字节一致**（文本协议允许字段序差异） |
| P-PEER-TXT | 对端文本模式可读；二进制乱码属预期 |
| P-RX-VAL | 助手出现解析后的数值记录 |
| P-RX-CTL | 助手出现控制事件记录 |
| P-RX-UNP | 出现未解析接收（HEX 展示 + 运行日志），禁止静默 |
| P-BOUND | 能力外操作被拒绝并写日志，对端无误发包 |
| P-UI | 模式/协议切换后侧栏无重叠错位 |

### 2.4 记录模板

```text
用例 ID：
日期：
构建版本 / DLL 路径：
结果：PASS / FAIL / BLOCKED
对端 HEX 摘录：
数据显示摘录：
运行日志摘录：
备注：
```

---

## 3. 环境与前置条件

| 项 | 要求 |
|---|---|
| 被测程序 | CommunicationAssistant，x64 |
| 动态库 | CommHandler **Release**，与助手同目录（禁止 Debug/Release CRT 混用） |
| 串口 | 虚拟串口对或真实串口；两端波特率/数据位/校验/停止位一致 |
| 网口 | 本机回环或同网段；对端开 HEX 显示 |
| 覆盖 DLL 前 | 先关闭正在运行的助手进程 |

**推荐默认联调参数**

| 通道 | 参数 |
|---|---|
| 串口 | 115200 / 8 / 无 / 1 |
| TCP | 本机 `127.0.0.1`，端口双方约定（例 `9000`） |
| UDP | 本机绑定端口与对端目标端口交叉配置 |

---

## 4. 原生通道测试用例

### 4.1 串口

| ID | 前置 | 操作（刺激） | 期望 |
|---|---|---|---|
| N-S-01 | 原生 + 串口；对端同参已开 | 打开本侧端口 | P-CONN「已连接」 |
| N-S-02 | 已连接；文本模式 | 发送 `ABC` | 对端收到 ASCII `ABC`（`41 42 43`） |
| N-S-03 | 已连接；勾选十六进制发送 | 发送 `41 42 43` | 对端收到 `ABC` |
| N-S-04 | 已连接 | 对端 HEX 发 `48 65 6C 6C 6F`（`Hello`） | 接收计数增加；数据显示可见 |
| N-S-05 | 两端一致改参 | 数据位 7、偶校验、停止位 2 打开并收发 `31 32 33` | 可打开；载荷一致 |
| N-S-06 | 故意不一致 | 本侧偶校验、对端无校验 | 打开或收发异常可观测（不得假「正常收发」） |

### 4.2 TCP 客户端

| ID | 前置 | 操作 | 期望 |
|---|---|---|---|
| N-TC-01 | 对端先监听 | 打开客户端 | P-CONN「已连接」 |
| N-TC-02 | 对端未监听 | 打开客户端 | 打开失败，状态非「已连接」 |
| N-TC-03 | 已连接 | 文本发 `ping`；再 HEX 发 `70 6F 6E 67` | 对端分别见 `ping` / `pong` |

### 4.3 TCP 服务端

| ID | 前置 | 操作 | 期望 |
|---|---|---|---|
| N-TS-01 | 原生 + TCP 服务端 | 打开 | P-CONN「已监听」类语义 |
| N-TS-02 | 已监听 | 对端连入后互发 `31 32 33` | 载荷一致 |

### 4.4 UDP

| ID | 前置 | 操作 | 期望 |
|---|---|---|---|
| N-U-01 | 原生 + UDP | 打开绑定本地端口 | P-CONN「已绑定」类语义 |
| N-U-02 | 已绑定；目标填对端 | 发送 `55 44 50`；对端回 `4F 4B` | 双向载荷与地址端口符合配置 |

---

## 5. 兼容动态库 — 网口协议（iProtoType）

**通用前置**：工作模式「兼容动态库」→ 连接方式「网口」→ 选协议 → TCP 客户端 → 对端 TCP 服务端同端口 → 打开连接。

### 5.0 网口协议速查

| 索引 | 名称 | 能否发数值 | 用例节 |
|---|---|---|---|
| 0 | JSON | 能 | §5.1 |
| 1 | 万测 | 能 | §5.2 |
| 2 | 中机 | 能（恰好 2） | §5.3 |
| 3 | 三思 | **不能** | §5.4 |
| 4 | 触发存图 | **不能** | §5.5 |
| 5 | 福建威盛 | **不能** | §5.6 |
| 6 | 纳百川自动化 | 能 | §5.7 |
| 7 | 纳百川线条 | 文本/4 值 | §5.8 |
| 8 | 联恒光科 | 能（≥2：力、温） | §5.9 |

---

### 5.1 TC-NET-0 JSON

#### TC-NET-0-TX-01 发送数值包

| 项 | 内容 |
|---|---|
| 前置 | 协议 0；已连接 |
| 入参 | `12.3,45.6` |
| 期望文本（语义） | `{"code":0,"values":[12.3,45.6],"tn":2}` |
| 期望 HEX（字段插入序参考） | `7B 22 63 6F 64 65 22 3A 30 2C 22 76 61 6C 75 65 73 22 3A 5B 31 32 2E 33 2C 34 35 2E 36 5D 2C 22 74 6E 22 3A 32 7D` |
| Fixture | `docs/fixtures/comm/NET0_json_TX_expect.txt` |
| 判定 | P-TX-IN；P-PEER-TXT（对象含 `code/values/tn`）；键序可变 |

#### TC-NET-0-RX-01 控制开始

| 项 | 内容 |
|---|---|
| 对端注入（文本） | `{"cmd":1}` |
| 对端注入 HEX | `7B 22 63 6D 64 22 3A 31 7D` |
| Fixture | `NET0_json_RX_ctrl_start.txt` |
| 判定 | P-RX-CTL（开始类控制） |

#### TC-NET-0-RX-02 非法 JSON → 未解析

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `6E 6F 74 2D 6A 73 6F 6E`（`not-json`） |
| 判定 | P-RX-UNP |

---

### 5.2 TC-NET-1 万测

#### TC-NET-1-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6`（分隔符默认逗号） |
| 期望线帧 ASCII | `12.3,45.6` |
| 期望 HEX | `31 32 2E 33 2C 34 35 2E 36` |
| Fixture | `NET1_wance_TX_expect.txt` |
| 判定 | P-TX-IN；P-PEER-HEX |

#### TC-NET-1-RX-01 未匹配控制字

| 项 | 内容 |
|---|---|
| 说明 | 接收仅在开启触发控制且整包等于配置 `cCMD` 时成立；默认配置下任意非匹配包应未解析 |
| 对端注入 | `41 42 43`（`ABC`） |
| 判定 | P-RX-UNP（或与现场 `cCMD` 全等时为 P-RX-CTL——以实际配置为准并在记录中注明） |

---

### 5.3 TC-NET-2 中机（二进制，必须 HEX 核对）

结构（`#pragma pack(1)`）：

- 发送 `ServerData`：`uint8=0x04` + `double LE`×2，共 **17** 字节
- 接收力值 `ForceData`：`uint8=0x02` + `double LE`×4，共 **33** 字节
- 接收控制 `ControlMessage`：`uint8 msgType` + `uint8 command`，共 **2** 字节（开始命令 `0x0A`）

#### TC-NET-2-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6`（必须恰好 2 个） |
| 期望线帧 HEX（17B） | `04 9A 99 99 99 99 99 28 40 CD CC CC CC CC CC 46 40` |
| Fixture | `NET2_zhongji_TX_12p3_45p6.hex` |
| 判定 | P-TX-IN；**P-PEER-HEX 逐字节**；文本模式乱码属预期 |

#### TC-NET-2-TX-02 个数非法

| 项 | 内容 |
|---|---|
| 入参 | `12.3` 或 `1,2,3` |
| 判定 | P-BOUND 或无写出；对端无新完整 17 字节包 |

#### TC-NET-2-RX-01 力值四通道

| 项 | 内容 |
|---|---|
| 对端注入 HEX（33B，值 1/2/3/4） | `02 00 00 00 00 00 00 F0 3F 00 00 00 00 00 00 00 40 00 00 00 00 00 00 08 40 00 00 00 00 00 00 10 40` |
| Fixture | `NET2_zhongji_RX_force_1_2_3_4.hex` |
| 判定 | P-RX-VAL（四通道约 1、2、3、4） |

#### TC-NET-2-RX-02 控制开始

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `01 0A` |
| Fixture | `NET2_zhongji_RX_ctrl_start.hex` |
| 判定 | P-RX-CTL |

#### TC-NET-2-RX-03 错误长度

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `04 00` |
| Fixture | `NET2_zhongji_RX_BAD_len.hex` |
| 判定 | P-RX-UNP |

---

### 5.4 TC-NET-3 三思（仅接收）

`Data` 未 pack：`double`×3 = **24** 字节；实现取第一路为力值。

#### TC-NET-3-TX-01 禁止发送

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6` 并点发送 |
| 判定 | P-BOUND；对端无新字节 |

#### TC-NET-3-RX-01 定长成功

| 项 | 内容 |
|---|---|
| 对端注入 HEX（24B，力=12.3，其余 0） | `9A 99 99 99 99 99 28 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00` |
| Fixture | `NET3_sansi_RX_force_12p3.hex` |
| 判定 | P-RX-VAL（力约 12.3） |

#### TC-NET-3-RX-02 长度错误

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `9A 99 99 99 99 99 28 40` |
| Fixture | `NET3_sansi_RX_BAD_len.hex` |
| 判定 | P-RX-UNP |

---

### 5.5 TC-NET-4 触发存图（仅接收）

#### TC-NET-4-TX-01 禁止发送

| 项 | 内容 |
|---|---|
| 操作 | 发起数值发送 |
| 判定 | P-BOUND |

#### TC-NET-4-RX-01 含 CRLF

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `54 52 49 47 0D 0A`（`TRIG\r\n`） |
| Fixture | `NET4_trig_RX_ok.hex` |
| 判定 | P-RX-CTL（触发存图类事件） |

#### TC-NET-4-RX-02 无 CRLF

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `54 52 49 47` |
| Fixture | `NET4_trig_RX_BAD_no_crlf.hex` |
| 判定 | P-RX-UNP |

---

### 5.6 TC-NET-5 福建威盛（仅接收）

规则：首字符 `T`/`S`；`L`…`E` 之间为 load。

#### TC-NET-5-RX-01

| 项 | 内容 |
|---|---|
| 对端注入 ASCII | `TL12.3E0` |
| 对端注入 HEX | `54 4C 31 32 2E 33 45 30` |
| Fixture | `NET5_weisheng_RX_TL12p3E0.hex` |
| 判定 | P-RX-VAL（约 12.3） |

#### TC-NET-5-RX-02 非法头

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `58 4C 31 32 2E 33 45 30`（`XL12.3E0`） |
| Fixture | `NET5_weisheng_RX_BAD_head.hex` |
| 判定 | P-RX-UNP |

---

### 5.7 TC-NET-6 纳百川自动化

#### TC-NET-6-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6` |
| 期望文本（语义） | `{"values":[12.3,45.6],"name":"LVEComputedValues"}` |
| 期望 HEX（参考插入序） | `7B 22 76 61 6C 75 65 73 22 3A 5B 31 32 2E 33 2C 34 35 2E 36 5D 2C 22 6E 61 6D 65 22 3A 22 4C 56 45 43 6F 6D 70 75 74 65 64 56 61 6C 75 65 73 22 7D` |
| Fixture | `NET6_nbc_TX_expect.txt` |
| 判定 | P-TX-IN；P-PEER-TXT（字段语义一致） |

#### TC-NET-6-RX-01 开始测试

| 项 | 内容 |
|---|---|
| 对端注入 | `{"name":"alphaStartTest"}` |
| Fixture | `NET6_nbc_RX_start.txt` |
| 判定 | P-RX-CTL / 参数事件 |

---

### 5.8 TC-NET-7 纳百川线条

#### TC-NET-7-TX-01（4 值）

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6,1,2` |
| 期望 | 对端 JSON 含 `hsm.L1/b1/Le1/bo1` 对应上述值；其余字段为 `"0"` |
| 判定 | P-TX-IN；P-PEER-TXT |

#### TC-NET-7-RX-01

| 项 | 内容 |
|---|---|
| 对端注入 ASCII | `calcStart` |
| 对端注入 HEX（等价） | `63 61 6C 63 53 74 61 72 74` |
| Fixture | `NET7_nbc_line_RX_calcStart.txt` |
| 判定 | P-RX-CTL；文本与 HEX 字节等价均应成功；重复投递仍有控制事件（不得静默） |
| 负例 | 任意非协议载荷 → P-RX-UNP（禁止无日志丢弃） |

---

### 5.9 TC-NET-8 联恒光科（网口）

**重要（重构必读）**：当前实现中，**发送编码帧**与**接收解析期望帧**布局不一致。

| 方向 | 长度字段含义 | 力/温类型号 | 段计数字节 |
|---|---|---|---|
| TX（`buildExternalDataFrame`） | payload 字节数 `0x0015`=21 | 力 `0x0B`、温 `0x0C` | 有前导 `0x02` |
| RX（`checkAndParseFrame` + `getExternalData`） | **段数** `0x0002` | 力 `0x0C`、温 `0x0D` | **无**前导计数 |

因此：**不能把 TX 黄金帧环回当 RX 用例**。TX、RX 必须使用下列两套不同 HEX。

CRC：CRC16-XMODEM（poly `0x1021`，初值 0），网口 CRC **大端**，帧尾 `0D 0A`。

#### TC-NET-8-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6` |
| 期望线帧 HEX（30B） | `AA 55 01 00 15 02 06 0B 40 28 99 99 99 99 99 9A 06 0C 40 46 CC CC CC CC CC CD 6F 2D 0D 0A` |
| Fixture | `NET8_lhgk_TX_12p3_45p6.hex` |
| 判定 | P-TX-IN；P-PEER-HEX 逐字节（含 `AA 55` 与尾 `0D 0A`、CRC `6F 2D`） |

#### TC-NET-8-RX-01 可解析完整帧

| 项 | 内容 |
|---|---|
| 对端注入 HEX（29B） | `AA 55 01 00 02 06 0C 40 28 99 99 99 99 99 9A 06 0D 40 46 CC CC CC CC CC CD C6 0D 0D 0A` |
| Fixture | `NET8_lhgk_RX_OK_12p3_45p6.hex` |
| 判定 | P-RX-VAL（力≈12.3，温≈45.6） |

#### TC-NET-8-RX-02 坏 CRC

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `AA 55 01 00 02 06 0C 40 28 99 99 99 99 99 9A 06 0D 40 46 CC CC CC CC CC CD 00 00 0D 0A` |
| Fixture | `NET8_lhgk_RX_BAD_crc.hex` |
| 判定 | P-RX-UNP |

#### TC-NET-8-RX-03 半帧（断帧）

| 项 | 内容 |
|---|---|
| 对端注入 HEX（半包） | `AA 55 01 00 02 06 0C 40 28 99 99 99 99 99 9A` |
| Fixture | `NET8_lhgk_RX_HALF_frame.hex` |
| 判定 | **不得立即**刷解析失败；补发剩余字节 `06 0D 40 46 CC CC CC CC CC CD C6 0D 0D 0A` 后应变为 P-RX-VAL 或一次完整坏帧处理 |

#### TC-NET-8-RX-04 禁止把 TX 帧当 RX

| 项 | 内容 |
|---|---|
| 对端注入 | 直接发送 `NET8_lhgk_TX_12p3_45p6.hex` |
| 判定 | 按当前解析器应 **P-RX-UNP**（用于锁定「TX≠RX」语义；若未来统一编解码，须改期望并同步 fixture） |

---

## 6. 兼容动态库 — 串口协议（iProtocolType）

**通用前置**：兼容动态库 → 串口 → 选协议 → COM 与对端成对、参数一致 → 打开。

### 6.0 串口参数侧栏

| ID | 操作 | 期望 |
|---|---|---|
| L-S-PARAM-01 | 查看侧栏 | 可见端口、波特率、数据位、校验、停止位 |
| L-S-PARAM-02 | 默认 115200/8/无/1 打开 | P-CONN |
| L-S-PARAM-03 | 改为 7/偶/2（对端同参） | 可打开收发 |
| L-S-PARAM-04 | 对端故意不同校验 | 异常/乱码可观测 |

### 6.1 串口协议速查

| 索引 | 名称 | 发数值 | 用例节 |
|---|---|---|---|
| 0 | 三思 | 能 | §6.2 |
| 1 | 科新 | 能 | §6.3 |
| 2 | 时代新材 | **不能** | §6.4 |
| 3 | IEEE | 能（≥5） | §6.5 |
| 4 | 冠腾 | 能（≥2） | §6.6 |
| 5 | 联恒光科 | 能（≥2） | §6.7 |

---

### 6.2 TC-SER-0 三思串口

数据位取打开串口时的 `DataBits`（常用 **8**）。每值：符号 +（段长−1）位 ASCII，末尾总加 `0D`。

#### TC-SER-0-TX-01（DataBits=8）

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6` |
| 期望 ASCII | `+12.3000+45.6000\r` |
| 期望 HEX | `2B 31 32 2E 33 30 30 30 2B 34 35 2E 36 30 30 30 0D` |
| Fixture | `SER0_sansi_TX_Data8_12p3_45p6.hex` |
| 判定 | P-TX-IN；P-PEER-HEX（若 MSVC `to_string` 尾数位差 1，以实机抓包修正 fixture 并记录） |

#### TC-SER-0-RX-01 未匹配 → 未解析

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `41 42 43 0D` |
| 判定 | 拼帧完成后 P-RX-UNP（控制字匹配场景依赖触发配置，记录中注明） |

---

### 6.3 TC-SER-1 科新

发送 `FixedDoubleToHex`：固定 14 字节，前缀 `02 4C` + `00012.30000#`（仅编码 CSV 第 1 个数值）。

#### TC-SER-1-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3`（仅第 1 个数值编码；多余 CSV 项忽略） |
| 期望线帧 HEX（14B） | `02 4C 30 30 30 31 32 2E 33 30 30 30 30 23` |
| Fixture | `SER1_kexin_TX_12p3.hex` |
| 判定 | P-TX-IN；P-PEER-HEX 逐字节（前两字节必须为 `02 4C`，禁止随机垃圾头） |

#### TC-SER-1-RX-01（数据）

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `02 4C 30 30 30 31 32 2E 33 30 30 30 30 23` |
| Fixture | `SER1_kexin_RX_with_prefix_12p3.hex` |
| 判定 | P-RX-VAL（约 12.3，**仅 1 路**） |

#### TC-SER-1-RX-02 控制字开始

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `7B 51 4C 49 5B 31 5D 7D`（`{QLI[1]}`） |
| Fixture | `SER1_kexin_RX_ctrl_QLI1.hex` |
| 判定 | P-RX-CTL（开始）；运行日志/数据显示可见控制事件 |

---

### 6.4 TC-SER-2 时代新材（仅接收）

缓冲至 `\r\n`；逗号分 4 段；`parts[2]` 为 `CONSTANT` 或 `RUN` 时取 `parts[0]`。

#### TC-SER-2-TX-01

| 项 | 内容 |
|---|---|
| 操作 | 发送数值 |
| 判定 | P-BOUND；对端无写出 |

#### TC-SER-2-RX-01

| 项 | 内容 |
|---|---|
| 对端注入 ASCII | `37.0,97,CONSTANT,0\r\n` |
| 对端注入 HEX | `33 37 2E 30 2C 39 37 2C 43 4F 4E 53 54 41 4E 54 2C 30 0D 0A` |
| Fixture | `SER2_sdxc_RX_ok.hex` |
| 判定 | P-RX-VAL（约 37.0） |

#### TC-SER-2-RX-02 段数不足

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `33 37 2E 30 2C 39 37 0D 0A` |
| Fixture | `SER2_sdxc_RX_BAD_parts.hex` |
| 判定 | P-RX-UNP |

---

### 6.5 TC-SER-3 IEEE（56 字节）

入参 ≥5：`v0..v3` 为应变/位移，`v4` 为时间；第 5 路扩展在发送路径固定为 `0`。

帧：`24 01` + 52 字节 ASCII（浮点 LE 字节倒序的 hex 字符映射）+ `0D 0A`。

#### TC-SER-3-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6,1,2,3` |
| 期望线帧 HEX（56B） | `24 01 34 31 34 34 43 43 43 44 34 32 33 36 36 36 36 36 33 46 38 30 30 30 30 30 34 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 34 30 34 30 30 30 30 30 32 30 30 30 0D 0A` |
| Fixture | `SER3_ieee_TX_12p3_45p6_1_2_3.hex` |
| 判定 | P-TX-IN；P-PEER-HEX 逐字节 |

#### TC-SER-3-TX-02 少于 5 个

| 项 | 内容 |
|---|---|
| 入参 | `1,2,3,4` |
| 判定 | P-BOUND；对端无 56 字节包 |

#### TC-SER-3-RX-01 / 02 控制

| ID | 对端注入 HEX | Fixture | 判定 |
|---|---|---|---|
| TC-SER-3-RX-01 | `24 00 0D 0A` | `SER3_ieee_RX_ctrl_start.hex` | P-RX-CTL（开始） |
| TC-SER-3-RX-02 | `24 FF 0D 0A` | `SER3_ieee_RX_ctrl_end.hex` | P-RX-CTL（结束） |

---

### 6.6 TC-SER-4 冠腾

格式：`R±########.#N±########.#`（宽 8、小数 4、左补 0）。

#### TC-SER-4-TX-01

| 项 | 内容 |
|---|---|
| 入参 | `12.3,45.6` |
| 期望 ASCII | `R+0012.3000#N+0045.6000#` |
| 期望 HEX | `52 2B 30 30 31 32 2E 33 30 30 30 23 4E 2B 30 30 34 35 2E 36 30 30 30 23` |
| Fixture | `SER4_guanteng_TX_12p3_45p6.hex` |
| 判定 | P-TX-IN；P-PEER-HEX |

#### TC-SER-4-RX-01 无专用解析

| 项 | 内容 |
|---|---|
| 对端注入 | 任意回显上述 TX HEX |
| 判定 | P-RX-UNP（当前无 case 4 接收分支） |

---

### 6.7 TC-SER-5 联恒光科（串口，LE）

帧：`AA 55 | type | len_lo len_hi (LE=payload 字节数) | payload | CRC16-XMODEM (LE) | 0D 0A`

发送与接收类型一致：力 `0x0C`、温 `0x0D`；length=`0x0014`（20）。**TX 帧可环回作 RX。**

#### TC-SER-5-TX-01 / RX-01（同一帧）

| 项 | 内容 |
|---|---|
| 入参（TX） | `12.3,45.6` |
| 线帧 HEX（29B） | `AA 55 01 14 00 06 0C 9A 99 99 99 99 99 28 40 06 0D CD CC CC CC CC CC 46 40 02 67 0D 0A` |
| Fixture | `SER5_lhgk_TXRX_12p3_45p6.hex` |
| TX 判定 | P-TX-IN；P-PEER-HEX（CRC `02 67` 小端） |
| RX 判定 | 对端注入同一 HEX → P-RX-VAL（力≈12.3，温≈45.6） |

#### TC-SER-5-RX-02 坏 CRC

| 项 | 内容 |
|---|---|
| 对端注入 HEX | `AA 55 01 14 00 06 0C 9A 99 99 99 99 99 28 40 06 0D CD CC CC CC CC CC 46 40 00 00 0D 0A` |
| Fixture | `SER5_lhgk_RX_BAD_crc.hex` |
| 判定 | P-RX-UNP |

---

## 7. 横切用例

### 7.1 连接真值

| ID | 场景 | 期望 |
|---|---|---|
| X-CONN-01 | TCP 客户端对端拒绝 | 不得显示「已连接」 |
| X-CONN-02 | UDP 绑定失败 | 不得显示「已绑定」 |
| X-CONN-03 | 串口打开失败 | 状态关闭/失败 |

### 7.2 能力边界

| ID | 场景 | 期望 |
|---|---|---|
| X-CAP-01 | 兼容通道勾选十六进制发送 | 拒绝发送并记日志；对端无包 |
| X-CAP-02 | 无发送能力协议点发送 | P-BOUND |

### 7.3 界面稳定性

| ID | 场景 | 期望 |
|---|---|---|
| X-UI-01 | 原生↔兼容、网口↔串口、协议切换 | P-UI |
| X-UI-02 | 发送列表页签切换 | 主区分栏高度不跳变 |

### 7.4 自动化与部署

| ID | 动作 | 期望 |
|---|---|---|
| X-AUTO-01 | 运行 CoreSelfTest | 与环境无关用例通过；串口环回失败须标注环境 |
| X-AUTO-02 | Debug 助手 + Release DLL | 可启动，无 CRT 混用崩溃 |

---

## 8. 推荐最小回归集（重构 CommHandler 必跑）

按优先级，每次改动态库协议后至少执行：

| 顺序 | 用例 ID | 目的 |
|---|---|---|
| 1 | TC-NET-2-TX-01 / RX-01 / RX-03 | 中机 pack 与定长解析 |
| 2 | TC-NET-8-TX-01 / RX-01 / RX-02 / RX-04 | 联恒网口 TX/RX 分叉与 CRC |
| 3 | TC-SER-5-TX-01 / RX-01 / RX-02 | 联恒串口环回与 CRC |
| 4 | TC-NET-3-TX-01 / RX-01 | 仅收协议边界 |
| 5 | TC-SER-3-TX-01 / RX-01 | IEEE 56B 打包 |
| 6 | TC-SER-4-TX-01 | 冠腾 ASCII 格式 |
| 7 | N-S-02 / N-TC-01 / N-U-01 | 原生通道未回归 |

完整矩阵仍以第 5、6 章全量为准。

---

## 9. Fixture 文件索引

目录：`docs/fixtures/comm/`

| 文件 | 对应用例 |
|---|---|
| `NET0_json_TX_expect.txt` | TC-NET-0-TX-01 |
| `NET0_json_RX_ctrl_start.txt` | TC-NET-0-RX-01 |
| `NET1_wance_TX_expect.txt` | TC-NET-1-TX-01 |
| `NET2_zhongji_TX_12p3_45p6.hex` | TC-NET-2-TX-01 |
| `NET2_zhongji_RX_force_1_2_3_4.hex` | TC-NET-2-RX-01 |
| `NET2_zhongji_RX_ctrl_start.hex` | TC-NET-2-RX-02 |
| `NET2_zhongji_RX_BAD_len.hex` | TC-NET-2-RX-03 |
| `NET3_sansi_RX_force_12p3.hex` | TC-NET-3-RX-01 |
| `NET3_sansi_RX_BAD_len.hex` | TC-NET-3-RX-02 |
| `NET4_trig_RX_ok.hex` / `NET4_trig_RX_BAD_no_crlf.hex` | TC-NET-4-RX-* |
| `NET5_weisheng_RX_TL12p3E0.hex` / `NET5_weisheng_RX_BAD_head.hex` | TC-NET-5-RX-* |
| `NET6_nbc_TX_expect.txt` / `NET6_nbc_RX_start.txt` | TC-NET-6-* |
| `NET7_nbc_line_RX_calcStart.txt` | TC-NET-7-RX-01 |
| `NET8_lhgk_TX_12p3_45p6.hex` | TC-NET-8-TX-01 |
| `NET8_lhgk_RX_OK_12p3_45p6.hex` | TC-NET-8-RX-01 |
| `NET8_lhgk_RX_BAD_crc.hex` | TC-NET-8-RX-02 |
| `NET8_lhgk_RX_HALF_frame.hex` | TC-NET-8-RX-03 |
| `SER0_sansi_TX_Data8_12p3_45p6.hex` | TC-SER-0-TX-01 |
| `SER1_kexin_RX_with_prefix_12p3.hex` | TC-SER-1-RX-01 |
| `SER2_sdxc_RX_ok.hex` / `SER2_sdxc_RX_BAD_parts.hex` | TC-SER-2-RX-* |
| `SER3_ieee_TX_12p3_45p6_1_2_3.hex` | TC-SER-3-TX-01 |
| `SER3_ieee_RX_ctrl_start.hex` / `SER3_ieee_RX_ctrl_end.hex` | TC-SER-3-RX-* |
| `SER4_guanteng_TX_12p3_45p6.hex` | TC-SER-4-TX-01 |
| `SER5_lhgk_TXRX_12p3_45p6.hex` | TC-SER-5-TX/RX-01 |
| `SER5_lhgk_RX_BAD_crc.hex` | TC-SER-5-RX-02 |

---

## 10. 修订记录

| 日期 | 说明 |
|---|---|
| 2026-07-21 | 初版：操作清单 |
| 2026-07-21 | **可执行黄金数据版**：网口 0–8 / 串口 0–5 全长 HEX + `docs/fixtures/comm/`；标明联恒网口 TX/RX 分叉 |
