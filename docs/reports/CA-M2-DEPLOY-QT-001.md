# 变更说明 — CA-M2-DEPLOY-QT-001

| 项 | 内容 |
|---|---|
| 日期 | 2026-07-22 |
| 口令 | 确认修改 |
| 问题 | PostBuild 写死 `E:\Qt\...`，本机为 `C:\Qt\...`，双击 exe 缺 `Qt5Network.dll` |
| 处理 | 新增 `tools/deploy_runtime.ps1`；Debug/Release PostBuild 共用；环境探测 Qt |

## 探测顺序

1. `-QtBinHint`（`$(QTDIR)\bin`，可空）
2. 环境变量 `QTDIR` / `QT_DIR` / `Qt5_DIR` / `CMAKE_PREFIX_PATH`
3. PATH 中的 `qmake` / `windeployqt`
4. 各固定盘符下 `\Qt\<version>\msvc2019_64\bin`
5. `qmake -query QT_INSTALL_BINS`

工程相对路径继续拷贝：`runtime\*`、`..\CommHandler\x64\Release\CommHandler.dll`

## 验证

| 配置 | 结果 |
|---|---|
| Debug\|x64 | PASS；部署日志 `Qt bin = C:\Qt\5.15.2\msvc2019_64\bin`；存在 `Qt5Network.dll` |
| Release\|x64 | PASS；同上 |

可双击：

- `CommunicationAssistant\x64\Debug\CommunicationAssistant.exe`
- `CommunicationAssistant\x64\Release\CommunicationAssistant.exe`
