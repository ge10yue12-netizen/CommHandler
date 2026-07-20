# 代码变更审计报告

## 1. 基本信息

- 变更编号：CA-M2-LEGACY-FIX-001
- 变更标题：Legacy Opening/close 竞态、Mock 文本发送、UI/构建修复
- 基线版本：V1.2.4
- 代码规范版本：V1.0
- 审计规范版本：V1.2
- 所属里程碑：M2
- 审计日期：2026-07-20

## 2. 变更目标

- 修复 Opening 中 close 可能仍触发 connect 的竞态
- Mock 支持透明文本发送（能力表允许时）
- 串口协议标签对齐 ProtoGuide；PostBuild 复制失败不拖垮构建

## 3. 实际修改

| 文件 | 操作 |
|---|---|
| legacy/LegacySession.* | cancelOpen_；迟到 connect 强制 disconnect |
| legacy/ILegacyBackend.cpp | Mock::sendText 成功路径 |
| src/MainWindow.cpp | 串口协议名；Legacy 波特率含 4800 |
| *.vcxproj | PostBuild exit /B 0，文件占用不失败 |
| tests/CoreSelfTest.cpp | cancel / text 用例 |

## 4. 追踪

| 需求 | 结果 | 等级 |
|---|---|---|
| F1 open 中 cancel → Closed 且非 Connected | PASS | E4 |
| F2 Mock 文本 Submitted | PASS | E4 |
| F3 原有 Legacy/DLL smoke 回归 | PASS | E4 |

## 5. 审计结论

- 结论：**PASS**
- 下一步：设备侧端到端协议联调（可选 CA-M2-LEGACY-003）