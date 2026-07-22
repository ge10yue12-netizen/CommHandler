# CA-M2-PACK-PORTABLE-001

| 项 | 内容 |
|---|---|
| 日期 | 2026-07-22 |
| 口令 | 确认修改 |
| 目标 | 通用「补齐依赖」+「干净打包」；助手 PostBuild 复用 |

## 新增

| 路径 | 用途 |
|---|---|
| `tools/Deploy-QtExe.ps1` | 任意 exe：探测 Qt、`windeployqt`、拷 Extra DLL（解决双击缺库） |
| `tools/Pack-Portable.ps1` | 任意 exe：打干净便携目录/zip（排除 obj/tlog） |
| `tools/README.md` | 用法说明 |
| `CommunicationAssistant/tools/pack_ca.ps1` | 助手一键打包封装 |
| `CommunicationAssistant/tools/deploy_runtime.ps1` | 改为转调通用 Deploy-QtExe |

## 验证

- `Deploy-QtExe` 对 Release exe：PASS（Qt bin=`C:\Qt\5.15.2\msvc2019_64\bin`）
- `Pack-Portable -Zip`：PASS；输出 `CommunicationAssistant\portable\CommunicationAssistant\` 与 `.zip`；无 `.obj`/`.tlog`
