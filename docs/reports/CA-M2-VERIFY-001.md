# 验证结论报告

## 1. 基本信息

- 变更编号：CA-M2-VERIFY-001
- 日期：2026-07-20
- CoreSelfTest：=== done: 0 failed ===

## 2. 主窗修复

- 默认尺寸改为约 900×580，最小 760×480
- 左侧参数区改为可滚动，避免控件把窗口撑得过大

## 3. 覆盖结论（诚实边界）

### 已由自测覆盖（PASS）

| 类别 | 内容 |
|---|---|
| Native 通讯 | 串口失败路径；虚拟串口环回（若本机有 COM4/5）；TCP Client 环回；TCP Server 双客户端；UDP 互通/冲突/空包 |
| Native 周边 | SendScheduler；Capture JSONL |
| Legacy 能力表 | 网口协议 0–7、串口 0–4 全矩阵；Raw 恒关；Voltage/Pulse configure 拒绝 |
| Legacy 会话 | 单会话互斥；Watchdog；Mock open/send/close；能力拒绝；open 中 cancel；Mock 文本发送 |
| CommHandler DLL | 网口 configure/open 冒烟；串口 configure/open 冒烟（无端口可失败但不崩溃） |

### 未宣称完成（需你设备联调）

| 类别 | 说明 |
|---|---|
| Legacy 各协议真实对端收发 | JSON/万测/中机/三思等需真实设备或对端软件；自测不替代现场联调 |
| Voltage / Pulse | 基线 [BLOCKED]，刻意不开放 |
| M3/M4 | Simulator、Codec、回放未做 |

## 4. 设计是否「都做好了」

- **Native 四种通讯方式**：工程与自测已具备，可认为 M1 验收级完成
- **动态库（Legacy）架构**：Worker 线程创建/销毁、Direct 复制、能力表驱动 UI、单会话、Watchdog、已链接你的 CommHandler.dll —— **架构已落地**
- **「所有协议都联调无问题」**：**不能**在无对端设备时保证；当前保证的是能力门控正确 + DLL 可加载/可 configure + Mock 路径正确

## 5. 建议你本地再点验

1. 重启应用（用最新 `CommunicationAssistant.exe`）看窗口是否变小
2. Native：TCP/UDP/串口各开一次
3. Legacy：取消 Mock，选一个你有的对端协议做一次真连