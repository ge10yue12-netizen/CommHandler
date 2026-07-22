# 变更对齐单

- 变更编号：CA-M2-UX-BASELINE-UI-001
- 基线版本：V1.2.5（docs/BASELINE.md）
- 审计规范版本：V1.2（docs/AUDIT_SPEC.md）
- 所属里程碑：M2
- 需求来源：用户确认修改——产品基线（改动能在正式软件跑）+ 助手 UI 分区/能力说明区/发送列表交互 + 下拉三角可见
- 问题描述：侧栏分区与清除位置不清晰；能力说明偏简；发送列表交互与文案混乱；下拉框三角标识被样式隐藏；需相对 CommHandler_1 保证正式宿主可用
- 预期结果：
  1. 侧栏内收；兼容模式分区：连接区 → 通信配置区 → 接收设置区 → 发送设置区 → 能力说明区
  2. 数据清除 / 日志清除分别在数据窗、日志窗右下角
  3. 发送列表：勾选删除、全部清空、右键菜单骨架；去掉「发送启用行」与延时列；「启动调度」改为「发送」；模式文案调整；间隔/次数按模式显隐
  4. 能力说明区按协议对齐 DLL（边界、日志要点）
  5. 全部 QComboBox 显示下拉三角
  6. DLL：相对 CommHandler_1 做正式路径兼容核查说明（本变更默认不改 DLL 源码，除非发现破坏性差异需最小修复）
- 明确不做：Voltage/Pulse；正式宿主业务改造；大幅重构 Session/Transport
- 已读取文件：BASELINE.md、AUDIT_SPEC.md、COMMHANDLER_DLL_DELTA.md、MainWindow.ui/cpp/h、LegacyCapability.*、SendScheduler.h、CommHandler.h / CommHandler_1
- 计划修改文件：
  - `CommunicationAssistant/ui/MainWindow.ui`
  - `CommunicationAssistant/src/MainWindow.cpp`
  - `CommunicationAssistant/src/MainWindow.h`
  - `CommunicationAssistant/legacy/LegacyCapability.cpp`（能力说明文案增强，必要时）
  - `docs/reports/CA-M2-UX-BASELINE-UI-001*.md`（对齐/自检/审计）
  - 可选：下拉箭头资源与 vcxproj
- 计划新增文件：对齐/自检/审计报告；可选 `resources/combo_down.svg` + `.qrc`
- 计划删除文件：无
- 外部接口变化：无（助手 UI/文案/交互；DLL 默认冻结）
- 持久化格式变化：发送列表 CSV 兼容去掉 interval 列（导入仍可读旧 5 列）
- 线程模型变化：无
- 使用的假设：单次模式对多勾选行按勾选顺序发完一轮后停止（不再截断为首行）
- 阻塞项：无
- 验收标准：Debug|x64 可编译；侧栏分区与清除位正确；发送列表交互符合对齐；下拉三角可见；正式路径兼容说明完成
- 计划执行的构建命令：MSBuild CommunicationAssistant Debug|x64（环境可用时）
- 计划执行的测试：CoreSelfTest（若可构建）；手工 UI 核对清单
- 回滚方式：git checkout 相关文件
- 是否已获「确认修改」：是
