# MachinePeer — 试验机侧（CommHandler 学习壳）

链接 `CommHandler.dll`。**试验机只收指令，只经串口发测量。**

## 主闭环

1. 默认 **串口 三思** COM5（`iComPort=4`）+ **UDP JSON 指令面** 19106
2. 软件 CommLab 点「开始」→ 本端 `[收← 解包] emitEventMsg`
3. 勾选自动回测 → **串口** `SendData(vector)` → 软件 `[收← 解包] emitNewData`

## 界面

- **SendData(vector)**：仅串口测量面
- 不收软件测量（若出现 `emitNewData` 会记为「非预期」）

## 构建

```bat
msbuild MachinePeer.sln /p:Configuration=Debug /p:Platform=x64
```

将 `CommHandler.dll` 放到 exe 旁。
