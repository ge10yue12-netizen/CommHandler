# MachinePeer 自检自测

- 日期：2026-07-22
- 范围：MachinePeer 与 CommHandler 线帧全量对齐（串口 PT0–5 / 网口 PT0–8）

## 对齐要点

| 协议 | 刺激方向（机→库） | 软件回包解析 |
|---|---|---|
| 串口 1 科新 | `{QLI[1/2]}` 控制；`FixedDoubleToHex` 14B（`02 4C…#`）测数 | 14B 定长切帧 |
| 串口 5 联恒 | AA55 小端 CRC；力 0x0C / 温 0x0D | 同布局 |
| 网口 8 联恒 | **RX 布局**（`len`=段数，力 0x0C / 温 0x0D） | **TX 布局**（`len`=payload 字节，力 0x0B / 温 0x0C） |

黄金帧与 `docs/fixtures/comm/`（SER1 / SER5 / NET8）一致。

## 自检

```text
msbuild MachinePeer.sln /p:Configuration=Debug /p:Platform=x64
msbuild ProtocolTests.vcxproj /p:Configuration=Debug /p:Platform=x64
tests\x64\Debug\ProtocolTests.exe
```

| 项 | 结果 |
|---|---|
| MachinePeer Debug 构建 | PASS |
| ProtocolTests Debug 构建 | PASS |
| ProtocolTests 运行 | `TOTAL_FAILURES 0` |

## 联调步骤（人工）

见 `MachinePeer/README.md`：先开试验机 TCP 服务端 9000，再开助手 TCP 客户端连 9000；协议索引与助手一致。
