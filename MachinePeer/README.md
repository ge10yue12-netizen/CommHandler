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

## 自检

```text
msbuild MachinePeer.sln /p:Configuration=Debug /p:Platform=x64
msbuild ProtocolTests.vcxproj /p:Configuration=Debug /p:Platform=x64
tests\x64\Debug\ProtocolTests.exe
```

期望：`TOTAL_FAILURES 0`（含 TCP/UDP 环回）。
