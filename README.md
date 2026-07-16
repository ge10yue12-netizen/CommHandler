# CommHandler 联调 Demo 仓库

本仓库放置 **CommHandler 动态库的联调 Demo 与学习文档**（非动态库完整业务源码）。  
用于：理解门面 API、现行协议怎么用、怎么发/收；**CommLab 与 MachinePeer 两端都链接 CommHandler.dll**（不自写线协议）。

远程仓库：<https://github.com/ge10yue12-netizen/CommHandler>

---

## 文档阅读顺序（两本独立手册，请都读）

| 顺序 | 文档 | 用途 |
|------|------|------|
| ① | [`CommHandler-验收问答手册.md`](./CommHandler-验收问答手册.md) | **独立验收问答**；完整示例、逐步联调流程、对/错判据（Q1–Q42） |
| ② | [`CommHandler-动态库使用与数据包详解.md`](./CommHandler-动态库使用与数据包详解.md) | **独立详解**；字段定义、逐协议数据包、组包/拆包、API 深度说明 |
| ③ | [`CommLab/README.md`](./CommLab/README.md) | 软件侧工程与界面操作 |
| ④ | [`MachinePeer/README.md`](./MachinePeer/README.md) | 试验机对端工程 |

两手册内容**协议编号与行为对齐**（均以 `SocketComm` / `SerialPortComm` 现行源码为准）。旧「联恒」已废弃，Demo 不对齐。

---

## 1. 仓库边界（很重要）

| 本仓库包含 | 本仓库不包含 |
|------------|--------------|
| CommLab / MachinePeer 源码与 VS 工程 | `CommHandler.dll` 完整 SDK 业务实现（在本地其它目录维护） |
| 头文件副本 `CommLab/include/`、`MachinePeer/include/` | 运行时必须的 `CommHandler.dll`（须自行放到 exe 旁） |
| 导入库 `*/lib/CommHandler.lib` | 电压 / 脉冲通道的完整硬件 Demo UI |
| **两本**协议学习手册、本 README | 本地构建产物（`x64/` 等已忽略） |

协议编号与收发能力以动态库源码现行 `switch` 为准。  
**旧「联恒」分支已在库中注释废弃，本 Demo 不提供对应选项。**

---

## 2. 快速开始

### 2.1 环境

- Windows x64  
- Visual Studio 2019（v142）+ Qt VS Tools（`msvc2019_64`）  
- Qt 模块：`core;gui;widgets;network;serialport`

### 2.2 CommLab（软件侧）与 MachinePeer（试验机侧）

两端工程各自打开解决方案，均链接 `CommHandler.lib`，exe 旁放 `CommHandler.dll`。

**推荐主闭环（串口测量 + UDP 指令）：**

1. 虚拟串口 COM4↔COM5；两端选 **串口 + 协议 0 三思**（Lab `iComPort=3`，Peer `iComPort=4`）
2. **MachinePeer 先打开**（自动：串口 + UDP 19106 指令面）
3. **CommLab 再连接**（串口 COM4 + UDP 19105→19106）
4. CommLab 点 **开始** → Peer `[收← 解包] emitEventMsg · 开始`
5. Peer 勾选自动回测 → 串口 `[发→ 组包] SendData(vector,SERIAL)`
6. CommLab `[收← 解包] emitNewData` 收到测量

纯 UDP JSON **不能**收试验机 `tn=2` 测量（库收侧不回调）。库无发送能力的协议按钮灰显，**不自造发送、不旁路**。

### 2.3 推荐学习顺序

1. 《验收问答》Q1–Q8 → 《详解》§2–§3  
2. 《详解》§6 / §7（逐协议）→ 两 EXE 按协议号对照联调  
3. 两 EXE 按协议号对照联调；能力以界面状态栏与 `ProtoGuide.h` 中 `cap` 表为准

---

## 3. 协议速览（现行可用）

**网口 `iProtoType`：** 0 JSON发 / 1 万测发 / 2 中机发(2值) / 3 三思仅收(24B) / 4 触发存图 / 5 福建威盛仅收 / 6 纳百川发 / 7 纳百川线条发(4值)

**串口 `iProtocolType`：** 0 三思 / 1 科新 / 2 时代新材(无发送) / 3 IEEE(≥5值) / 4 冠腾

报文与完整示例见两本手册。

---

## 4. 目录结构

```
.
├── README.md
├── CommHandler-验收问答手册.md
├── CommHandler-动态库使用与数据包详解.md
├── CommLab/
└── MachinePeer/
```

---

## 5. 注意

- 勿提交密钥与真实设备敏感配置。  
- 运行时 `CommHandler.dll` 请自行获取。  
- 默认分支：`main`。
