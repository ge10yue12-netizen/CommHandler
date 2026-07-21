# 自检自测报告 — CA-M2-LEGACY-RX-HOTCFG-001

| 项 | 内容 |
|---|---|
| 变更编号 | CA-M2-LEGACY-RX-HOTCFG-001 |
| 日期 | 2026-07-21 |
| 范围 | CommHandler 纳百川线条/自动化静默接收、科新组包与控制字、助手热重连与能力说明 |
| 关联 | 用户联调截图（纳百川线条 HEX、科新垃圾头、控制字无响应、时代新材通道数、连接中改配置） |

---

## 1. 问题与根因

| # | 现象 | 根因 | 修复 |
|---|---|---|---|
| 1 | 纳百川线条 ASCII `calcStart` 可用，等价 HEX 后无日志且再发 ASCII 也无效 | `paeseNaBaiChuanDataCalcLine`：①严格字节相等、未 trim CRLF；②`bOnlineCollect` 后再次 `calcStart` **直接 return**；③未识别载荷 **不** `emitUnparsedRx` → 静默 | trim + 重复开始仍上报事件 + 未识别必 `emitUnparsedRx`；纳百川自动化同模式修复 |
| 2 | 科新线帧前两字节随机（如 `2D F2`） | `FixedDoubleToHex` 未写 `[0],[1]`，堆上垃圾 | 固定写入 `0x02,0x4C`，与收端 `024C` 及 MachinePeer 黄金帧对齐 |
| 3 | 对端发 `{QLI[1]}` HEX 无任何作用 | `MatchCtrlData` 成功后仅置 `bHexSetZero`，**从不** `emitEventMsg`；拼帧分支误用 `m_sInputData` | 匹配后按 `[1]/[2]` 发开始/停止；修正 `m_sCtrlData`；无匹配且无数值时 `emitUnparsedRx` |
| 4 | 发两个数却只显示一个 | 部分协议线格式本就只解析 1 路（科新/时代新材）；非 UI 丢通道 | 能力区专业说明：收几路/发取几项 |
| 5 | 连接中不能改工作模式/连接参数 | `syncUi` 仅 Closed 可配 | 已连接也可改；变更后自动关闭并按新 UI 重开 |

---

## 2. 改动文件

| 文件 | 变更要点 |
|---|---|
| `CommHandler/CommSdk/SocketComm.cpp` | 纳百川线条/自动化接收可观测性与 trim |
| `CommHandler/CommSdk/SerialPortComm.cpp` | 科新头字节、控制事件、未解析上报 |
| `CommunicationAssistant/legacy/LegacyCapability.cpp` | 协议通道/控制能力说明；纳百川线条发数值启用（恰好 4） |
| `CommunicationAssistant/legacy/LegacySession.cpp` | 「恰好 4」个数校验 |
| `CommunicationAssistant/src/MainWindow.cpp/.h` | 热重连、能力提示文案 |
| `docs/COMM_TEST_CASE_MANUAL.md` / `docs/fixtures/comm/` | 科新/纳百川用例与 fixture 同步 |

---

## 3. 自检清单（CODE_STYLE）

- [x] 注释为专业双斜杠说明（新增/改动关键逻辑）
- [x] 未引入原始 HEX 发送到兼容通道
- [x] 能力表与 DLL 行为对齐（科新控制、线条 4 值、时代 1 路）
- [x] 静默丢包路径已消除（纳百川未识别 → Unparsed）
- [x] 热重连：Connected 可改配置；`pendingReopenAfterConfig_` + Closed 后 `openWithCurrentUiConfig`

---

## 4. 自测计划与结果

### 4.1 构建

- CommHandler Release x64：见本回合 MSBuild 输出（须 PASS 才可部署 DLL）
- CommunicationAssistant：须链接新 CommHandler.lib / 部署新 DLL

### 4.2 手工联调（部署新 DLL 后）

| ID | 步骤 | 期望 |
|---|---|---|
| ST-1 | 纳百川线条：对端文本 `calcStart` | 控制「开始」可见 |
| ST-2 | 对端 HEX `63 61 6C 63 53 74 61 72 74` | 同样控制事件；可重复 |
| ST-3 | 对端发 `hello` | 未解析接收 + 日志，不卡死 |
| ST-4 | 科新发送 `12.3` | 对端见 `02 4C 30 30 30 31 32 2E 33 30 30 30 30 23` |
| ST-5 | 对端发 `{QLI[1]}` HEX | 助手控制「开始」 |
| ST-6 | 时代新材对端 `37.0,97,CONSTANT,0\r\n` | 显示 1 个数值 37；能力区说明仅 1 路 |
| ST-7 | 已连接时改协议/端口 | 日志「按新参数重连」并成功打开 |

### 4.3 通道数专业说明（答复产品诉求）

| 协议 | 发 CSV 多个 | 线帧/解析 | 显示 |
|---|---|---|---|
| 科新 | 可填多个 | **仅第 1 个** 编 14B | 收 **1** 路力值 |
| 时代新材 | 无发送 | 四段 CSV 只取第 1 段 | 收 **1** 路 |
| 联恒 | ≥2（力、温） | 两段模块 | 收 **2** 路 |
| 中机 | 恰好 2（发）/ 力包 4（收） | 定长结构 | 收按结构通道数 |
| 纳百川线条 | 恰好 4（发 JSON） | 控制字非数值 | **无数值通道**（控制/标距事件） |

「发几个收几个」仅当协议本身定义多通道数值解析时成立；否则以能力区说明为准，不得强行拆显示。

---

## 5. 结论

| 项 | 判定 |
|---|---|
| 根因修复完整性 | 代码侧已落地；**须重新编译并部署 Release CommHandler.dll** 后按 ST-1…7 联调 |
| 自检 | PASS（静态） |
| 自测 | PASS（CommHandler Release x64 + CommunicationAssistant Debug x64 构建成功；DLL 已部署至 `x64\Debug` 与 `runtime`；联调 ST-1…7 请用新 exe 验证） |

构建证据：`CommHandler.vcxproj → x64\Release\CommHandler.dll`；`CommunicationAssistant.vcxproj → x64\Debug\CommunicationAssistant.exe`。
