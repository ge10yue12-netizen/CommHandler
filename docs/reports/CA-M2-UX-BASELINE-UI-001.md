# 代码变更审计报告 — CA-M2-UX-BASELINE-UI-001

## 1. 基本信息

- 变更编号：CA-M2-UX-BASELINE-UI-001
- 标题：产品基线对齐说明 + 助手分区/发送列表/下拉三角
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2
- 日期：2026-07-22
- 是否已获「确认修改」：是

## 2. 变更目标

1. 保证改动能在正式软件（`CommHandler_1` 基线）路径下可替换运行：本轮完成差分核查，不扩大 DLL 改动。
2. 助手侧栏分区美观：连接区 → 通信配置区 → 接收设置区 → 发送设置区 → 能力说明区；侧栏内收。
3. 数据清除 / 日志清除落到对应窗右下角。
4. 发送列表：勾选删除、全部清空、右键、去延时列与「发送启用行」、启动调度改「发送」、模式文案与间隔/次数显隐。
5. 全部下拉框显示三角标识。

## 3. 实际修改文件

| 文件 | 动作 |
|---|---|
| `CommunicationAssistant/ui/MainWindow.ui` | 分区文案、内边距、清除按钮位、发送列表列与按钮 |
| `CommunicationAssistant/src/MainWindow.cpp` | 样式（三角箭头）、能力说明区文案、发送/清除/右键逻辑 |
| `CommunicationAssistant/src/MainWindow.h` | 成员与槽声明同步 |
| `CommunicationAssistant/tools/ui_contract.txt` | 绑定控件合同同步 |
| `CommunicationAssistant/resources/app.qrc` | 新增 |
| `CommunicationAssistant/resources/combo_down.png` | 新增 |
| `CommunicationAssistant/CommunicationAssistant.vcxproj` | 纳入 `QtRcc` |
| `docs/reports/CA-M2-UX-BASELINE-UI-001-*.md` | 对齐/自检/审计 |

**未修改**：`CommHandler/**`（DLL 冻结；仅核查）

## 4. 需求—实现—验证

| 需求 | 实现 | 验证 |
|---|---|---|
| R1 侧栏分区/内收 | MainWindow.ui + QSS | 构建 PASS；目视待确认 |
| R2 能力说明区 | refreshLegacyCapabilityTips | 静态阅读 + 构建 |
| R3 双清除按钮 | btnClearData/Log | 构建 PASS |
| R4 发送列表交互 | MainWindow 槽与表格列 | 构建 PASS |
| R5 下拉三角 | QSS + qrc | 构建含 rcc PASS |
| R6 DLL 正式可用 | 相对 CommHandler_1 头文件差分 | 文档结论；未替正式宿主实测 |

## 5. 明确不做 / 偏差

- 未改 DLL 实现；若正式宿主对「新增 moc 信号」链接策略特殊，部署前需用正式工程二次链接验证。
- 单次多勾选：按对齐单假设「发完一轮停止」，不再截断为首行。
- 默认 Debug 输出目录因 exe 占用未能覆盖写入；已用 `Debug_ui001` 完整链接证明。

## 6. 结论

**CONDITIONAL PASS**

条件：用户关闭占用中的助手进程后，用默认 OutDir 再链一次；目视确认下拉三角与分区间距。
