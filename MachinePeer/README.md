# MachinePeer — 试验机对端模拟

位置：`Demo_test/MachinePeer`  
独立工程：`MachinePeer.sln`（VS2019 + Qt `msvc2019_64` x64）

> 软件侧动态库全协议说明：[`../CommHandler-验收问答手册.md`](../CommHandler-验收问答手册.md)  
> 本工程**不链接** CommHandler，只模拟试验机线协议（与三思串口/UDP 测量对齐）。

## 角色

双方皆可收发。本工程作为**试验机**：

| 主路径 | 说明 |
|--------|------|
| 收指令 | 识别 `3D/3E/6B/5A + 0D`，写入「执行」日志 |
| 执行后回测 | 勾选「收到开始后回发测量」时，回传力/位移/应变 |
| 发测量 | 手动「发送测量值」 |
| 收数据 | 非指令帧记为数据帧 |

附加：界面可主动发指令（双向测试），**不是**主路径。

参数全部来自 `peerconfig.ini`（界面可临时改）；无串口时请建虚拟串口对，或改用 UDP。

## 配置 `peerconfig.ini`

查找：当前目录 → exe 目录。

```ini
[network]
bind_ip=127.0.0.1
bind_port=19106
peer_ip=127.0.0.1
peer_port=19105

[serial]
port_index=4
baud=115200
data_bits=8
parity=N
stop_bits=1
seg_size=8
```

## 模块

| 文件 | 职责 |
|------|------|
| `MainWindow.*` | 通道、收指令执行、发测量 |
| `WireCodec.*` | 线协议编解码 |
| `PeerConfig.*` | 读 ini、校验、串口线格式 |

## 构建

```bat
cd Demo_test\MachinePeer
msbuild MachinePeer.sln /p:Configuration=Debug /p:Platform=x64
```
