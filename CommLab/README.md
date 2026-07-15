# CommLab — 串口 / 网口协议数值联调台（MVP）

位置：`Demo_test/CommLab`  
语言：**C++14** | 工程：**Visual Studio 2019** + Qt VS Tools（`msvc2019_64`，仅 **x64**）  
界面：`MainWindow.ui`（可用 Qt Designer 编辑）

> **学习动态库全文：** 仓库根目录 [`CommHandler-验收问答手册.md`](../CommHandler-验收问答手册.md)（四通道 + 网口 0–7 + 串口 0–4，含示例；废弃联恒仅说明不对齐）。

> 仅维护 **VS 工程**（本目录 `CommLab.sln`）。不要用 Qt Creator / qmake / MinGW 打开。

CommLab 是挂在 `CommHandler` 动态库外面的**联调外壳**：负责选通道、填参数、连接、收发数值、打运行记录、输出串口「预期十六进制帧」，以及无头自检。**协议组包与解析在 DLL 内**；本仓不能改库内协议实现。

本工程**独立维护**：只打开 `CommLab.sln`，不与其它对端模拟工程合并解决方案。

---

## 1. 文档导读

| 你想… | 先读 |
|-------|------|
| **吃透动态库全协议** | [`../CommHandler-验收问答手册.md`](../CommHandler-验收问答手册.md) |
| 建立整体心智模型 | §2 能力边界 → §3 总体逻辑 → §4 模块说明 |
| 串口对照联调 | §5.1 串口主路径 |
| 网口收发 / 自检 | §5.2、§5.3、§7 |
| 在 VS 里链上 CommHandler | §6 工程与依赖 |
| 查参数 / 事件码 | §8、§9 |

---

## 2. 能力边界与前提

| 手上有什么 | 能做什么 |
|------------|----------|
| 仅 `CommHandler.dll` | 运行期黑盒调用（本 Demo 还需要对应 `.lib` + 头文件才能编译） |
| `dll + lib + 头文件` | 完整编译、链接、联调本 Demo |
| 要改协议实现 | 必须有动态库**源码**，不在本项目范围 |

**本项目负责：**

- 调用 `SetCommType` / `setParameter` / `Connect` / `Disconnect` / `SendData`
- 安全承接 DLL 回调（立即拷贝 `void*`）
- 串口三思「预期帧」本地预览（不调用 DLL）
- UDP 本机对端模拟 + CLI / UI 自检

**本项目不负责：** 协议字节定义、串口底层驱动、改 `CommHandler` 内部逻辑。

---

## 3. 总体逻辑与数据流

### 3.1 一句话逻辑

```text
人（界面）或脚本（--smoke*）
    → 写入通道参数
    → CommHandler 连接
    → 收：DLL 回调 → CommController 立即拷贝 → UI / 自检断言
    → 发：SendData →（串口另出预期帧预览）→ 线路 / UDP
```

### 3.2 架构图

```text
                    ┌─────────────────────────────────────┐
                    │  main.cpp                           │
                    │  · 无参      → QApplication + GUI    │
                    │  · --smoke*  → QCoreApplication     │
                    │                + smoke_result.txt   │
                    └──────────────┬──────────────────────┘
                                   │
          ┌────────────────────────┼────────────────────────┐
          ▼                        ▼                        ▼
   MainWindow (+ .ui)         SmokeRunner            SerialFramePreview
   人机联调、日志、计数         无头自检（N1/N2/预览）   本地三思组包预览
          │                        │                        │
          └────────────┬───────────┘                        │
                       ▼                                    │
                CommController                              │
                · Qt::DirectConnection 立即拷贝 void*        │
                · 转发 safeData / event / conn / log        │
                       │                                    │
                       ▼                                    │
                CommHandler（dll + lib + 头文件，VS 手动配置）
                       │
         ┌─────────────┼─────────────┐
         ▼             ▼             ▼
       串口           UDP 网口      事件码
   虚拟串口对      UdpPeerSimulator
   + 串口调试助手   + netconfig.ini
```

### 3.3 回调安全铁律（必读）

`CommHandler::emitNewData(void* data, int size, …)` 的 `void*` **在槽返回后可能失效**。

因此 `CommController` 必须：

1. 使用 `Qt::DirectConnection`（在发射线程立刻执行）
2. 立刻 `memcpy` 到 `QVector<double>`
3. 再 `emit safeDataReceived(...)` 给界面（可排队到 UI 线程）

禁止把原始 `void*` 用 QueuedConnection 丢给 `MainWindow`。

长度防护：`size <= 0` 或 `size >= 10000` 的回调会被丢弃并记警告。

### 3.4 两种启动形态

| 启动方式 | 运行环境 | 用途 |
|----------|----------|------|
| 无参数 | `QApplication` + `MainWindow` | 人工联调 |
| `--smoke` / `--smoke-send` / `--smoke-preview` / `--smoke-all` | `QCoreApplication` | 无人值守自检，结果写入当前目录 `smoke_result.txt` |

---

## 4. 模块说明

| 模块 | 文件 | 职责 | 明确不做的事 |
|------|------|------|----------------|
| 入口 | `src/main.cpp` | 参数分流；GUI 或单条/全量 smoke；写 `smoke_result.txt` | 不配置协议细节 |
| 主窗口 | `src/MainWindow.h/.cpp` + `MainWindow.ui` | 通道切换、参数、连接/断开/发送、日志、计数、触发同款自检 | 不直接处理原始 `void*` |
| 安全封装 | `src/CommController.h/.cpp` | 持有 `CommHandler`；危险回调 → 安全 Qt 信号；事件码中文名 | 不做 UI 布局 |
| 网口配置 | `src/NetConfig.h/.cpp` + `netconfig.ini` | 查找并加载 ini；`ApplyNetConfigBase` / `ConfigureLabUdp` | 不管串口线格式 |
| 串口预期帧 | `src/SerialFramePreview.h/.cpp` | 按三思协议 0 本地组包，输出空格分隔大写十六进制 | **不调用 DLL** |
| UDP 对端 | `src/UdpPeerSimulator.h/.cpp` | 本机绑定收包、发原始 UDP；构造 24 字节三思帧 | 不替代 CommHandler |
| 无头自检 | `src/SmokeRunner.h/.cpp` | N1 收合法/非法帧；N2 JSON 发送许可；预览自检 | 预览不依赖真实串口 |
| 外部库 | `CommHandler`（工程外） | 真正连接、组包、解析、事件 | 本仓只黑盒调用 |

### 4.1 通道能力摘要（对齐 `SocketComm` / `SerialPortComm` 现行实现）

| 通道 | 协议号 | 名称 | 收 | 数值发 | 备注 |
|------|--------|------|----|--------|------|
| 串口 | 0 | 三思 SSCK | ✅ | ✅ | 默认；有 `[预期帧]` 预览 |
| 串口 | 1 | 长春科新 | ✅ | ✅ | 14 字节定长 |
| 串口 | 2 | 时代新材 | 视库 | ❌ | UI 禁发（库 `case 2` 空） |
| 串口 | 3 | IEEE | ✅ | ✅ | 须 ≥5 个 double |
| 串口 | 4 | 冠腾 | ✅ | ✅ | `Rxx#Nxx#` ASCII |
| UDP | 0 | JSON | ✅ | ✅ | `{"code":0,"values":[…],"tn":2}` |
| UDP | 1 | 万测 | ✅ | ✅ | 分隔符由 `iSymbolType` |
| UDP | 2 | 中机四通道 | ✅ | ✅ | 发须恰好 2 个 double |
| UDP | 3 | 三思 | ✅ | ❌ | 24 字节，回调多为力值 |
| UDP | 4 | 触发存图 | 事件 | ❌ | 报文含 `\r\n` 触存图事件 |
| UDP | 5 | 福建威盛 | ✅ | ❌ | `T/S` 文本力值 |
| UDP | 6 | 纳百川全自动 | ✅ | ✅ | JSON `LVEComputedValues` |
| UDP | 7 | 纳百川识别线条 | ✅ | ✅ | 发须 4 个 double |

**不对齐 / 仅说明：** 库源码中旧「联恒」网口/串口 `case 0` 已整段注释，本 Demo **不提供**对应选项。完整说明见仓库根目录《CommHandler-验收问答手册》。

发送许可：`bInquireSendFlag`。串口连接成功后自动打开；UDP 由复选框控制。单次发送数值上限 **32**。

---

## 5. 端到端业务流程

### 5.1 串口主路径（推荐先跑通）

**目标：** 本端发一个力值，对端串口助手看到与 `[预期帧]` 一致的十六进制；助手回命令帧，本端显示事件名。

**准备：**

1. 建立虚拟串口对（例：COM4 ↔ COM5）。
2. 串口调试助手打开 **COM5**，模式：**十六进制**收发；线格式与 CommLab 状态栏「线格式」一致（默认 **115200 8N1**）。
3. CommLab 保持默认：**串口 / 三思 / COM 序号 3（显示 COM4）**。

**步骤：**

1. 点击 **连接** → 日志应出现 `[连接] 成功`，并自动启用发送许可。  
2. 点 **开始 3D0D**（或停止/存图/清零）→ 试验机应出现 `[执行] …`；若勾选回测则回传测量。  
3. 发送框填 `1.0`，点 **发送数值** → 运行记录查看 `[预期帧]`（可选）。  
4. 试验机回测量或对端发事件 → 本端 `[接收]` / `[事件]`；若见 `[收] 忽略空数据回调` 表示库空回调，不是有效测量。

**指令经路（已审计）：**  
- **UDP**：本软件 `QUdpSocket` 向目的口原始写出 `命令码+0D`（不依赖可能空实现的 `SendData(QString)`）。  
- **串口**：临时 `Disconnect` → `QSerialPort` 旁路写出 → 再 `Connect`（因库占用串口且 `SendData(QString)` 基类默认为空）。  

试验机应出现 `[接收][指令]`；本端发送为 `[发送][指令]`。

### 5.2 UDP 三思接收

1. 确认 `netconfig.ini`（或界面）本机地址端口，默认本机 `127.0.0.1:19105`，对端 `127.0.0.1:19106`。  
2. 通道选 **UDP 网口**，协议选 **3 三思（仅接收）**，连接。  
3. 使用 UI「自检-收」、或 CLI `--smoke`：由 `UdpPeerSimulator` 向本机口发送 24 字节合法帧（力/位移/应变三个 `double`），断言回调力值 ≈ 期望；再发非法长度报文，断言**不应**再触发数据回调。

### 5.3 UDP JSON 发送与许可

1. 协议选 **0 JSON（可收可发）**（或跑 `--smoke-send`）。  
2. 对端监听 **目的端口**（默认 19106）。  
3. `bInquireSendFlag = false` 时 `SendData` **不应**写出 UDP。  
4. 打开许可后应对端收到数据。  
5. **不要用 UDP 三思测发送**——库侧无发送实现；测发送请用 JSON。

### 5.4 主窗内嵌自检

UI 上的「自检」按钮与 CLI 共用 `SmokeRunner`。执行前若当前已连接，会先断开，避免端口占用；自检使用**独立** `CommController` 实例。

---

## 6. 工程与依赖（含手动连接 CommHandler）

### 6.1 工具链

- Visual Studio 2019，平台工具集 **v142**
- Qt **msvc2019_64** + **Qt Visual Studio Tools**
- Qt 模块：`core;gui;widgets;network;serialport`
- 解决方案：`CommLab.sln`（仅 x64 Debug / Release）

### 6.2 CommHandler 手动配置（当前工程已清空内置库路径）

请在 VS：**项目属性**（对 Debug|x64 与 Release|x64 **分别**设置，或注意配置组合一致）：

| 属性页 | 要填的内容 |
|--------|------------|
| **C/C++ → 常规 → 附加包含目录** | 能找到 `CommHandler.h`、`UIDef.h`、`Common` 等相关头文件的目录 |
| **链接器 → 常规 → 附加库目录** | `CommHandler.lib` 所在目录（须与当前 Debug/Release 一致） |
| **链接器 → 输入 → 附加依赖项** | `CommHandler.lib` |
| 运行时 | 将匹配的 `CommHandler.dll`（及厂商依赖 DLL）放到 `OutDir`（例：`x64\Debug\`），或自行增加生成后事件复制 |

配置不一致时的典型现象：编译找不到头文件 → 链接找不到符号 → 运行提示缺少 DLL。

### 6.3 生成与输出

- 输出目录：`CommLab\x64\Debug\` 或 `Release\`
- 生成后事件：仅复制同目录下的 `netconfig.ini` 到输出目录（**不再**自动复制 `CommHandler.dll`）
- 部署 Qt：对输出目录执行 `windeployqt`，或从已部署环境拷入 Qt/插件运行时

### 6.4 MSBuild 示例

```bat
cd Demo_test\CommLab
set QtMsBuild=%LOCALAPPDATA%\QtMsBuild
msbuild CommLab.sln /p:Configuration=Debug /p:Platform=x64
```

---

## 7. CLI 自检

在输出目录（需已有 `CommHandler.dll`、Qt 运行时、`netconfig.ini`）：

```bat
cd Demo_test\CommLab\x64\Debug
CommLab.exe --smoke          rem N1：UDP 三思接收（合法帧 + 非法长度）
CommLab.exe --smoke-send     rem N2：UDP JSON 发送与 bInquireSendFlag
CommLab.exe --smoke-preview  rem 串口三思预期帧自检（无串口）
CommLab.exe --smoke-all      rem 以上三项
```

期望：控制台与 `smoke_result.txt` 均为 `PASS`，进程退出码 `0`。写入结果文件失败且本应通过时，可能返回 `98`。

| 路径 | 操作 | 自动化 |
|------|------|--------|
| 串口发数 + 预期帧 | GUI 发送后对照 `[预期帧]` | 半自动 |
| 串口收命令 | 助手十六进制 `3D0D` 等 | 半自动 |
| N1 / N2 / 预览 | `--smoke*` 或 UI 自检按钮 | 自动 |

---

## 8. 配置项速查

### 8.1 `netconfig.ini`

查找顺序：1）进程当前工作目录；2）exe 所在目录。都找不到则用内置默认值。

```ini
[network]
local_ip=127.0.0.1      ; 本机绑定（UDP 接收）
local_port=19105
dest_ip=127.0.0.1       ; 对端（UDP 发送目的）
dest_port=19106
transfer_type=1         ; → iTransferType（1=UDP）
model=0                 ; → iModel

[smoke]
sansi_proto=3           ; 自检三思协议号
json_proto=0            ; 自检 JSON 协议号
```

### 8.2 串口 → `setParameter`（连接前写入）

| 界面 | 参数名 | 备注 |
|------|--------|------|
| COM 序号 | `iComPort` | 从 0 起；显示名 COM = 序号+1 |
| 波特率下拉 | `iComBaud` | 存的是索引，不是波特率数值 |
| 数据位 | `iDataBits` | 0=Data5，1=Data6，3=Data8（无 7） |
| 校验 | `iParity` | 0N / 1E / 2O / 3S / 4M |
| 停止位 | `iStopBits` | 0=1，1=1.5，2=2 |
| 串口协议 | `iProtocolType` | 0=三思 SSCK 等 |
| — | `dInForceRange` | 固定写入 `100.0` |
| — | `input` | `DataInput`（通道/触发等） |
| 允许发送 | `bInquireSendFlag` | bool |

### 8.3 网口 → `setParameter`

| 来源 | 参数名 |
|------|--------|
| 通道类型 | `SetCommType(NETWORK)` |
| ini / 界面 | `iTransferType`、`iModel`、`sLocalIP`、`iLocalPort`、`sDestIP`、`iDestPort` |
| 协议下拉 | `iProtoType` |
| 允许发送 | `bInquireSendFlag` |

### 8.4 串口命令事件对照

| 助手发送（十六进制） | 界面含义 |
|----------------------|----------|
| `3D 0D`（或本软件「开始」按钮） | 开始计算 |
| `3E 0D` | 停止计算 |
| `6B 0D` | 自动存图 |
| `5A 0D` | 清零 |

另有退出程序、信号触发存图等事件码，未知码以十六进制显示。

### 8.5 预期帧规则（串口三思协议 0）

- 每个数值占 `segSize` 字节（5 / 6 / 8），符号等为 ASCII 风格编码，帧末追加 `0x0D`
- 仅用于与串口助手**对照**；真正发送仍由 DLL `SendData` 完成
- `--smoke-preview` / `SerialFramePreview::selfCheckOrEmpty` 校验段长 8/6/5 的组包自洽性

---

## 9. 常见问题

| 现象 | 排查 |
|------|------|
| 找不到 `CommHandler.h` / 链接 `CommHandler.lib` | §6.2 手动包含目录与库目录是否配在当前配置（Debug/Release）上 |
| 运行缺 `CommHandler.dll` | 拷到 exe 旁；本工程不再自动复制 |
| 运行缺 Qt DLL | `windeployqt` 或补齐平台插件 |
| 串口连上但助手无数据 | COM 号是否配对；线格式是否一致；是否已连接且发送许可打开 |
| `[预期帧]` 与助手不一致 | 是否协议 0；数据位是否与助手一致；对照的是发送后的预览而非收包 |
| `[收] 忽略空数据回调` | 库空回调，非有效测量；有效测量应有 `[接收] …数值=` |
| 点指令试验机无反应 | 是否已连接；试验机是否打开配对 COM/UDP；`SendData(QString)` 是否被当前协议支持 |
| smoke 失败「连接失败」 | 端口占用、ini 地址错误、DLL 未加载 |

---

## 10. 目录与源码索引

```text
CommLab/
  CommLab.sln              VS 解决方案
  CommLab.vcxproj          工程（不含 CommHandler 内置路径）
  MainWindow.ui            界面布局
  netconfig.ini            网口 / smoke 默认配置
  README.md                本文件
  src/
    main.cpp               入口与 CLI
    MainWindow.*           主窗逻辑
    CommController.*       DLL 安全封装
    NetConfig.*            网口配置加载与应用
    SerialFramePreview.*   三思预期帧
    UdpPeerSimulator.*     UDP 模拟对端
    SmokeRunner.*          无头自检
  x64/Debug|Release/       构建输出
```

| 我要… | 优先打开 |
|-------|----------|
| 改界面布局 | `MainWindow.ui` |
| 改连接/发送流程 | `MainWindow.cpp` |
| 改回调安全策略 | `CommController.cpp` |
| 改网口默认地址 | `netconfig.ini` |
| 改预期十六进制算法 | `SerialFramePreview.cpp` |
| 改自检用例 | `SmokeRunner.cpp` + `main.cpp` 参数分支 |
