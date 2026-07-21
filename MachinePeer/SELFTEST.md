# MachinePeer 自检自测

- 日期：2026-07-21
- 范围：仅 MachinePeer（试验机）；未改 CommunicationAssistant / CommHandler

## 根因

助手 Legacy 默认 **TCP 客户端 → :9000**；旧试验机仅 **UDP + 19105/19106**，且默认串口，导致「连不上网口」。

## 变更

- `PeerChannelManager`：TCP 服务端/客户端 + 原有 UDP/串口
- 默认：网口、TCP、服务端、监听 **9000**（`peerconfig.ini` 可改）
- 串口：补数据位 **7**；打开映射与 UIDef 一致
- `ProtocolTests`：增加 TCP/UDP 环回用例

## 自检

| 项 | 结果 |
|---|---|
| MachinePeer Debug 构建 | PASS |
| ProtocolTests Debug 构建 | PASS |
| ProtocolTests 运行 `TOTAL_FAILURES 0` | 见运行输出 |

## 联调步骤（人工）

见 `MachinePeer/README.md`：先开试验机 TCP 服务端 9000，再开助手 TCP 客户端连 9000。
