# CommunicationAssistant

Qt 桌面通信调试助手：Native（串口/TCP/UDP）+ Legacy（CommHandler.dll）。

## CommHandler 接入布局

| 目录 | 内容 |
|------|------|
| `include/` | `CommHandler.h`、`CommBase.h`、`Any.h`、`UIDef.h` |
| `lib/` | `CommHandler.lib`（导入库） |
| `runtime/` | `CommHandler.dll` / `.pdb` 及依赖 DLL（构建后复制到 exe 旁） |

工程通过 `CA_LINK_COMMHANDLER=1` 静态链接导入库；PostBuild 拷贝 `runtime/*`、`CommHandler.dll`，并调用仓库通用工具：

`../QtExeTool.bat <exe> fix nopause`

该 bat 在**仓库根目录**（与工程无关）：任意 Qt+VS 的 exe 都可用。发给别人选菜单 **3 ALL**，zip 在 `QtExePack\`（与 bat 同级，按 `_debug`/`_release` 分包）。  
说明见：`../QtExeTool.README.md`

更新动态库时：同步替换 `include/`、`lib/CommHandler.lib`、`runtime/CommHandler.dll(.pdb)`。

## 工具链

- VS2019 v142、Qt 5.15.2 msvc2019_64、C++17
- Debug 配置与 Release CommHandler ABI 对齐（`/MD` + Release Qt）
- 可双击运行目录：`x64\Debug\` 或 `x64\Release\`（构建后应含 `Qt5Network.dll` 等）
