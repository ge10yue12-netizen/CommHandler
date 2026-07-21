# 自检自测报告



- 变更编号：CA-M2-DEBUG-ASSIST-001（未解析 + 断帧 + 上限）

- 日期：2026-07-21

- 基线：V1.2.5



## 目的对齐



Legacy 动态库仍只出业务回调；调试助手要回答：**来数据了，当前协议解不出**。  

联恒还要区分：**断帧等待** vs **完整坏帧可观测**。



## 逻辑自检



- [x] DLL `emitUnparsedRx`：default；联恒完整坏帧；断帧不报

- [x] `probeCompleteFrameSize`：0=断帧，-1=非法同步，>0=完整长度

- [x] `processLhgkRxStream`：串口 case5 / 网口 case8

- [x] 助手：HEX 展示 + `[边界] …解析失败`；单包 4KiB 截断；100ms 限速

- [x] 不在助手内造协议帧

- [x] runtime / 助手 Debug 旁 DLL 与 Release.lib 同源



## 构建



| 目标 | 结果 |

|---|---|

| CommHandler Release + Debug | PASS（均含 `emitUnparsedRx`） |

| CommunicationAssistant Debug | PASS（须关占用中的 exe） |

| CoreSelfTest | DLL smoke PASS；串口环回 2 FAIL（环境，非本票） |

| 运行 DLL | 用 Release（与导入库同源，避免 Debug CRT 混用崩溃） |



## 手工



1. Legacy 联恒：对端慢发半帧 → 不应刷「解析失败」；拼满后应出业务或一次坏帧

2. Legacy 联恒：故意坏 CRC 完整帧 → 数据窗 HEX + `解析失败`

3. Legacy 未知协议 index → default 未解析仍可见

4. 高频噪声 → 日志出现「另抑制N」，UI 不卡死



## 续：乱码 / 侧栏 / 文档（2026-07-21）



### 逻辑自检

- [x] 联恒/中机乱码判定为二进制线帧 + 对端文本模式（非助手 CSV 直发）
- [x] 中机发送改二进制 `sendData(char*,len)`，避免 QString 损坏
- [x] 网口三思：能力表标明 DLL 无发送分支；串口三思 ASCII 提示
- [x] 具名协议解析失败补 `emitUnparsedRx`（中机/三思/JSON/万测/威盛/串口抽样）
- [x] 侧栏固定高度；transport 常驻；客户端区占位
- [x] 专业文档：`docs/LEGACY_PROTOCOL_DEV_GUIDE.md`、`docs/LEGACY_FULL_FLOW_TEST_MANUAL.md`

### 构建（本续）

| 目标 | 结果 |
|---|---|
| CommHandler Release\|x64 | PASS → runtime + x64\Debug 已覆盖 |
| CommunicationAssistant Debug\|x64 | PASS |
| CoreSelfTest | 2 FAIL：串口环回 `loop open RX` / `loop RX matches`（环境 COM4/5，非本票逻辑）；能力表/DLL smoke/其余 **PASS** |

## 续：人机交互文案专业化 + 交付文档（2026-07-21）

### 范围

- 界面标题、占位符、协议能力提示、运行日志、边界/异常文案专业化
- 对外术语：原生通道 / 兼容动态库 / 模拟后端
- 文档：`docs/COMM_TEST_CASE_MANUAL.md`、`docs/COMMUNICATION_ASSISTANT_DEV_GUIDE.md`、重写 `docs/LEGACY_PROTOCOL_DEV_GUIDE.md`

### 自检

- [x] 去掉口语化提示（「不要填」「点发送」「在跑」等）
- [x] 能力区改「支持/不支持」书面表述
- [x] 测试手册覆盖原生四传输 + 网口 0–8 + 串口 0–5 收发矩阵
- [x] 开发说明含架构、数据流、连接真值、文案规范、构建移植

### 兼容串口参数补齐（2026-07-21）

- [x] `formLegacySerial` 增加数据位/校验/停止位，与原生选项对齐
- [x] `buildConfig` 读取三控件，不再硬编码 8/none/1
- [x] `dataBitsIndexFromValue` 补 `case 7 → 2`
- [x] 测试手册增加 N-S-05、L-S-PARAM-01～05

### Designer 手改永久护栏（2026-07-21）

- [x] 去掉 `pageLegacy` 的 `colspan`，改为 `layLegacyPage` 平铺（可 Designer 保存）
- [x] `mainPanel`/`sendWorkbenchPane` 改为可选 `findChild`，拆外壳不再 C2039
- [x] `tools/ui_contract.txt` + `check_ui_contract.ps1` + `msbuild/UiContract.targets` 编译前校验


