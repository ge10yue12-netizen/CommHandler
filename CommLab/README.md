# CommLab — 软件侧（CommHandler 学习壳）

链接 `CommHandler.dll`。**软件只发指令，只收试验机测量。**

## 主闭环（推荐）

1. 通道选 **串口**，协议 **0 三思**，`iComPort=3`（COM4）
2. 同时自动连接 **UDP JSON 指令面**（19105→19106）
3. 点 **开始** → `SendData(QString,NETWORK)` 发 `{"tn":1}`
4. MachinePeer 收 `emitEventMsg`，经 **串口** `SendData(vector)` 回测量
5. CommLab 串口侧 `[收← 解包] emitNewData`

虚拟串口：COM4↔COM5；试验机 `iComPort=4`（COM5）。

## 界面

- **四指令**：开始 / 停止 / 存图 / 清零
  - 开始、停止：走 UDP JSON 指令面
  - 存图、清零：调 `SendData(QString,SERIAL)` 发 HEX（库发侧当前可能空实现）
- **不发** `SendData(vector)` 测量
- 日志：`[发→ 组包]` / `[收← 解包]`

## 构建

```bat
msbuild CommLab.sln /p:Configuration=Debug /p:Platform=x64
```

将 `CommHandler.dll` 放到 exe 旁。
