# QtExeTool.bat — 使用说明

文件位置：**仓库根目录** `Demo_test\QtExeTool.bat`

## 目的

一个可单独复制的 bat，给任意 **Qt + Visual Studio** Windows 程序：

- 只有 Debug、只有 Release、或两者都有 —— **按你拖入的那个 exe** 自动处理
- 判定依据是 **exe 导入表**（不是文件夹名 `Debug`/`Release`）

| 操作 | 作用 |
|---|---|
| **1 FIX** | 补齐正确变体的 Qt 运行库到 exe 旁（本机双击） |
| **2 PACK** | 打干净便携包 |
| **3 ALL** | 先 FIX 再 PACK（发给别人用这个） |

## 用法

```bat
QtExeTool.bat
QtExeTool.bat "D:\any\YourApp.exe" all
QtExeTool.bat "D:\any\YourApp.exe" fix nopause
```

也可把 exe 拖到 bat 上。

同一软件若 Debug / Release 都能编：对两个 exe 各跑一次 ALL，会生成两套互不覆盖的包。

## 输出位置

| 操作 | 位置 |
|---|---|
| FIX | exe 同目录 |
| PACK | **`QtExePack\`**（与 bat 同级），例如： |

```
Demo_test\
  QtExeTool.bat
  QtExePack\
    CommLab_Debug\          + CommLab_Debug.zip
    CommLab_Release\        + CommLab_Release.zip
    CommunicationAssistant_Debug\   + ...zip
    CommunicationAssistant_Release\ + ...zip
```

目录名优先取 exe 所在的 `Debug` / `Release` 文件夹，避免两套配置互相覆盖；内部 Qt DLL 仍按 exe 导入表部署。

## 对方环境

解压整个文件夹后双击 exe；一般不必装 Qt。缺 `VCRUNTIME140.dll` 时安装 VC++ 2015–2022 x64。

## 修订

| 日期 | 说明 |
|---|---|
| 2026-07-22 | 输出集中到 `QtExePack\`；Debug/Release 分包命名；插件错变体清理 |
