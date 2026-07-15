# CommHandler 联调 Demo 仓库

本仓库放置 **CommHandler 动态库的联调 Demo 与学习文档**（非动态库完整业务源码）。  
用于：理解门面 API、现行协议编号、参数、控制指令，并在本机用两个 EXE 做串口 / UDP 双向联调。

| 内容 | 路径 |
|------|------|
| 学习 / 验收手册（优先阅读） | [`CommHandler-验收问答手册.md`](./CommHandler-验收问答手册.md) |
| 软件侧联调台（链接 DLL） | [`CommLab/`](./CommLab/) |
| 试验机对端模拟（不链接 DLL） | [`MachinePeer/`](./MachinePeer/) |

远程仓库：<https://github.com/ge10yue12-netizen/CommHandler>

---

## 1. 仓库边界（很重要）

| 本仓库包含 | 本仓库不包含 |
|------------|--------------|
| CommLab / MachinePeer 源码与 VS 工程 | `CommHandler.dll` 完整 SDK 业务实现（在本地其它目录维护） |
| 头文件副本 `CommLab/include/` | 运行时必须的 `CommHandler.dll`（须自行放到 exe 旁） |
| 导入库 `CommLab/lib/CommHandler.lib`（便于链接） | 电压 / 脉冲通道的完整硬件 Demo UI |
| 协议学习手册、README | Git 历史中的本地构建产物（`x64/` 等已忽略） |

协议编号与收发能力以动态库源码 `SocketComm` / `SerialPortComm` 的现行 `switch` 为准，详见手册。  
**旧「联恒」分支已在库中注释废弃，本 Demo 不对齐、不提供对应选项。**

---

## 2. 快速开始

### 2.1 环境

- Windows x64  
- Visual Studio 2019（v142）+ Qt VS Tools（`msvc2019_64`）  
- Qt 模块：`core;gui;widgets;network;serialport`

### 2.2 CommLab（软件侧）

1. 打开 `CommLab/CommLab.sln`  
2. 工程属性中确认：  
   - 附加包含目录 → `CommLab/include`  
   - 附加库目录 → `CommLab/lib`，链接 `CommHandler.lib`  
3. 将 **`CommHandler.dll`** 放到生成目录（与 `CommLab.exe` 同级）  
4. 按需改 `CommLab/netconfig.ini`（UDP 本机 / 对端端口）  
5. 生成并运行；或无无头自检：

```bat
CommLab.exe --smoke
CommLab.exe --smoke-send
CommLab.exe --smoke-preview
CommLab.exe --smoke-all
```

更细的联调步骤见 [`CommLab/README.md`](./CommLab/README.md)。

### 2.3 MachinePeer（试验机侧）

1. 打开 `MachinePeer/MachinePeer.sln`（**独立**解决方案，不要与 CommLab 合并）  
2. 编辑 `MachinePeer/peerconfig.ini`（默认 UDP `19106` ↔ CommLab `19105`）  
3. 串口联调请用虚拟串口对（如 COM4↔COM5），两端线格式一致  

详见 [`MachinePeer/README.md`](./MachinePeer/README.md)。

### 2.4 推荐联调顺序

1. 读手册 §0–§3（心智模型 + 网口/串口协议表）  
2. CommLab `--smoke` / `--smoke-preview`  
3. UDP：先开 MachinePeer，再开 CommLab，发 `3D0D`，对端应出现 `[接收][指令]`  
4. 串口：虚拟 COM 配对后同样验证指令与三思测量帧  

---

## 3. 协议速览（现行可用）

### 网口 `iProtoType`（SocketComm）

| 值 | 名称 | 数值发送 |
|----|------|----------|
| 0 | JSON | ✅ |
| 1 | 万测 | ✅ |
| 2 | 中机四通道 | ✅（须 2 个数） |
| 3 | 三思 | ❌（仅收，24 字节） |
| 4 | 触发存图 | ❌（事件） |
| 5 | 福建威盛 | ❌（仅收） |
| 6 | 纳百川全自动 | ✅ |
| 7 | 纳百川识别线条 | ✅（须 4 个数） |

### 串口 `iProtocolType`（SerialPortComm）

| 值 | 名称 | 数值发送 |
|----|------|----------|
| 0 | 三思 SSCK | ✅（可识别 3D/3E/6B/5A0D） |
| 1 | 长春科新 | ✅ |
| 2 | 时代新材 | ❌（库内空实现） |
| 3 | IEEE | ✅（≥5 个数） |
| 4 | 冠腾 | ✅ |

完整示例、参数 Key、如何改协议 / 指令：见 [`CommHandler-验收问答手册.md`](./CommHandler-验收问答手册.md)。

---

## 4. 目录结构

```
.
├── README.md                          ← 本说明
├── CommHandler-验收问答手册.md         ← 动态库学习与验收
├── CommLab/                           ← 软件联调台
│   ├── CommLab.sln
│   ├── include/                       ← CommHandler 头文件
│   ├── lib/CommHandler.lib
│   ├── netconfig.ini
│   ├── README.md
│   └── src/
└── MachinePeer/                       ← 试验机模拟
    ├── MachinePeer.sln
    ├── peerconfig.ini
    ├── README.md
    └── src/
```

---

## 5. 许可与注意

- 请勿将真实设备地址、账号、密钥提交进仓库。  
- `CommHandler.dll` 请自行从公司构建产物获取，勿依赖本仓提供运行时 DLL。  
- 提交流程：本地改完后按团队规范提交；本仓默认分支建议为 `main`。
