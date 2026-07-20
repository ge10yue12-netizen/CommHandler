# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-SIDEBAR-LAYOUT-001
- 标题：左栏分区重排（连接 / 接收 / 发送 / 其他）
- 基线版本：V1.2.5
- 审计规范版本：V1.2
- 里程碑：M2
- 日期：2026-07-21
- 范围：CommunicationAssistant 侧栏信息架构与样式；不改协议语义

## 2. 变更目标

- 客户路径：先连 → 再设收 → 再设发 → 偶尔清理
- 侧栏分区清晰；发送载荷仍在下方工作台
- 去掉参数与操作之间的无意义空洞；「其他」沉底

## 3. 实际修改

| 文件 | 动作 |
|---|---|
| `ui/MainWindow.ui` | 侧栏重排；发送格式迁回侧栏；单条页仅编辑框+发送钮 |
| `src/MainWindow.cpp` | 分区 QSS（`SideSection` / `FieldLabel`）；`sendSideHint`；`syncUi` 显隐 |

## 4. 侧栏结构

```text
标题
连接 → 模式/方式/参数/打开/状态/客户端
接收 → 显示格式 / 抓包
发送 → Native 格式 或 Legacy 模式 + 工作台提示
（弹性空白）
其他 → 清空数据/日志
```

## 5. 验证

| 项 | 结果 |
|---|---|
| Debug\|x64 构建 | PASS |
| 目视分区与交互 | 待用户确认 |

## 6. 结论

**CONDITIONAL PASS**

---

## 续：间距收紧（同编号）

- **问题**：`QStackedWidget` 默认纵向撑满，参数区与「打开连接」之间出现大块空白。
- **处理（优先写在 `.ui`，便于 Designer 调）**：
  - `paramStack`：`vsizetype=Maximum`
  - `sideLay`：spacing 4、margin 12；表单 `verticalSpacing=4`
  - `sideStretch`：`Expanding`（只在「其他」之上吃剩余高度）
  - 布局 `stretch` 写入 `.ui`；控件 padding/字体在 QSS 略收
- **验证**：Debug\|x64 PASS

## 续：紧凑侧栏 + 格式勾选（同编号）

- 侧栏宽 **290**；`sideLay` spacing **3** / margin **8**；表单行距 **2**
- 接收/发送格式改回 **☑ 十六进制显示 / 十六进制发送**（去掉格式下拉）
- 自检脚本阈值：宽≤300、spacing≤3、margin≤8、verticalSpacing≤2、无 rx/txFormatCombo、`paramStack` Maximum → **SELFCHECK PASS**
- Debug\|x64 构建 **PASS**

## 续：参考密度强压（同编号）

- 宽 **268**；spacing **2**；margin **6**；表单行距 **1**
- 工作模式/连接方式并入 `formConnect`（标签左、控件右，不再上下叠）
- 分区标题「接收设置：/发送设置：」；格式仍为勾选
- SidePanel 专用 QSS：12px 字、控件高约 20–22、主按钮高约 26–28
- 自检 **SELFCHECK PASS** + Debug 构建 **PASS**
