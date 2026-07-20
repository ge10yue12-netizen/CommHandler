# CommunicationAssistant

Qt 桌面通信调试助手：Native（串口/TCP/UDP）+ Legacy（CommHandler.dll）。

## CommHandler 接入布局

| 目录 | 内容 |
|------|------|
| `include/` | `CommHandler.h`、`CommBase.h`、`Any.h`、`UIDef.h` |
| `lib/` | `CommHandler.lib`（导入库） |
| `runtime/` | `CommHandler.dll` / `.pdb` 及依赖 DLL（构建后复制到 exe 旁） |

工程通过 `CA_LINK_COMMHANDLER=1` 静态链接导入库；PostBuild 将 `runtime/*` 拷到 `x64\Debug` 或 `x64\Release`。

更新动态库时：同步替换 `include/`、`lib/CommHandler.lib`、`runtime/CommHandler.dll(.pdb)`。

## 工具链

- VS2019 v142、Qt 5.15.2 msvc2019_64、C++17
- Debug 配置与 Release CommHandler ABI 对齐（`/MD` + Release Qt）
