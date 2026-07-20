# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-UI-DESIGNER-001
- 标题：MainWindow 迁到 Qt Designer `.ui`（外观等价）
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2
- 日期：2026-07-20
- 范围：CommunicationAssistant 主窗布局壳层；不改协议/会话语义

## 2. 变更目标

- 原问题：主窗布局全在 `buildUi()` / `buildSendWorkbench()` 手写，难以用 Qt Designer 调整外观。
- 预期：静态壳层进入 `ui/MainWindow.ui`；运行时仍填充端口/协议/列表行并连接信号；外观与交互等价。
- 明确不做：不改 Native/Legacy 收发语义；不改 DLL；不重构业务到 `ui.` 全量替换成员指针。

## 3. 工作区状态

- Git 工程：是
- 用户已有大量未跟踪构建产物；本变更聚焦源码与工程文件
- 审计方法：文件清单 + Debug 构建证据

## 4. 实际修改

| 文件 | 动作 | 原因 |
|---|---|---|
| `CommunicationAssistant/ui/MainWindow.ui` | 新增 | Designer 壳层：侧栏、参数栈、数据/日志分栏、发送工作台、页脚 |
| `CommunicationAssistant/src/MainWindow.h` | 修改 | 引入 `ui_MainWindow.h`、`Ui::MainWindow ui_`；`bindUiWidgets` / `populateDynamicUi` |
| `CommunicationAssistant/src/MainWindow.cpp` | 修改 | `setupUi` 替代手写布局；删除 `buildSendWorkbench` |
| `CommunicationAssistant/CommunicationAssistant.vcxproj` | 修改 | `<QtUic Include="ui\MainWindow.ui" />` |
| `CommunicationAssistant/tools/gen_mainwindow_ui.py` | 新增 | 可从结构再生 `.ui`（维护辅助，非运行时依赖） |

## 5. 设计说明

- 控件 `name` 供 uic 生成成员；部分 `objectName` 覆盖为样式表选择器（如 `SideScroll`、`PrimaryButton`）。
- 动态内容仍在代码：串口枚举、协议列表、Mock 默认、示例发送行、信号连接、Splitter 拉伸。
- 业务逻辑继续使用既有 `*_` 成员指针（由 `bindUiWidgets` 绑定），避免一次大范围改写。

## 6. 验证

| 项 | 结果 |
|---|---|
| `uic ui\MainWindow.ui` | PASS |
| Debug\|x64 编译 | PASS |
| 链接 `CommunicationAssistant.exe` | PASS（曾因进程占用 LNK1168，结束进程后重链成功） |
| PostBuild windeployqt | 警告：本机 Anaconda Qt 路径干扰依赖探测；与本次布局迁移无关 |
| Designer 打开 `.ui` 后拖拽再编译 | 待用户确认 |
| 外观/交互目视等价 | 待用户确认 |

## 6.1 后续修复（同编号续）

- **问题**：`.ui` 将 QSS 用名（`PrimaryButton` 等）写成控件 `name`，uic 成员与 `bindUiWidgets` 不一致 → C2039；工程无 `.filters` → 解决方案资源管理器平铺。
- **处理**：控件 `name` 改回稳定业务名；在 `populateDynamicUi` 中 `setObjectName` 供 QSS；补 `mainPanel` 包装；新增 `CommunicationAssistant.vcxproj.filters`（按 src/core/transport/session/services/capture/legacy/include/Form Files 分组）。
- **复验**：Debug\|x64 Rebuild PASS（2026-07-21）。
- **续**：新增 `tests/CoreSelfTest.vcxproj.filters`（tests/transport/session/services/capture/legacy + Header 分组），仅整理资源管理器显示。

## 7. 偏差与风险

- 表单项间距等细微差异可能来自 Designer 默认 FormLayout 参数；若目视有偏差可在 `.ui` 中微调。
- 未在本轮跑 CoreSelfTest（本变更不触及协议能力表逻辑）。
- Designer 中请勿把多个控件的 `name` 改成相同样式名；样式请改 `objectName` 或继续在代码里 `setObjectName`。

## 8. 结论

**CONDITIONAL PASS**（构建通过；外观等价与 Designer 可编辑性待用户确认；筛选器需在 VS 中重新加载工程后查看）
