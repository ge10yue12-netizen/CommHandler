# MachinePeer — 试验机联调说明（仅本工具；助手侧配置见 COMMUNICATION_ASSISTANT）

## 与通信调试助手对接（默认）

助手 Legacy 默认：**TCP 客户端 → 127.0.0.1:9000**。

本工具默认：**网口 + TCP + 服务端 + 监听 9000**。

1. 启动 `MachinePeer.exe` → 确认「网口 / TCP / 服务端 / 本机 127.0.0.1 / 9000」→ **打开**
2. 日志应出现：`TCP 服务端已监听 …`
3. 助手：兼容动态库 → 网口 → TCP → 客户端 → 远端 `127.0.0.1:9000` → 协议与试验机一致 → **打开连接**
4. 试验机日志：`TCP 对端已连接 …`
5. 试验机发「开始」/「测数」；助手侧应有控制/数值或 HEX

## UDP 模式

`peerconfig.ini` 或界面改为 UDP，交叉端口示例：

| 侧 | 本地端口 | 远端端口 |
|---|---|---|
| 试验机 | 19106 | 19105 |
| 助手 | 19105 | 19106 |

助手本地端口不可为 0。

## 串口

虚拟串口对两端各选一 COM；波特率/数据位/校验/停止位一致（默认 115200 8N1）。`port_index`：0=COM1。

## 协议对齐范围（与 CommHandler）

试验机**不链接** CommHandler DLL，但线帧必须与库一致，用于刺激助手：

| 通道 | 已对齐项 |
|---|---|
| 串口 0–4 | 既有三思/科新/时代/IEEE/冠腾 |
| 串口 1 科新 | 控制 `{QLI[1]}`/`{QLI[2]}`；测数 14B `02 4C…#` |
| 串口 5 联恒 | 控制 type=0x02；测数 SER5 黄金帧 |
| 网口 0–7 | 既有 JSON/万测/中机/三思/威盛/纳百川 |
| 网口 8 联恒 | 刺激发 **RX** 布局；解析软件 **TX** 布局（TX≠RX） |

下拉协议列表含串口「5 联恒光科」、网口「8 联恒光科」。详情见 `SELFTEST.md` 与 `docs/fixtures/comm/`。

## 自检

```text
msbuild MachinePeer.sln /p:Configuration=Debug /p:Platform=x64
msbuild ProtocolTests.vcxproj /p:Configuration=Debug /p:Platform=x64
tests\x64\Debug\ProtocolTests.exe
```

期望：`TOTAL_FAILURES 0`（协议黄金帧 + TCP/UDP 环回）。
