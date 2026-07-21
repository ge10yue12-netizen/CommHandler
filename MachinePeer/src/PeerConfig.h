// PeerConfig.h — MachinePeer 默认网络/串口参数（可由 peerconfig.ini 覆盖）
#pragma once

#include <QString>

// 试验机测试工具默认参数（缺省对齐助手 Legacy：TCP 客户端 → 本工具 TCP 服务端 :9000）
struct PeerConfig {
    // 本机绑定/监听 IP
    QString localIp = QStringLiteral("127.0.0.1");
    // 本机端口（TCP 服务端监听端口 / UDP 绑定端口）
    int localPort = 9000;
    // 软件侧 IP
    QString destIp = QStringLiteral("127.0.0.1");
    // 软件侧端口（TCP 客户端目标 / UDP 发送目标）
    int destPort = 9000;
    // 0=TCP 1=UDP（对齐 iTransferType）
    int transferType = 0;
    // 0=服务端 1=客户端（对齐 iModel；仅 TCP）
    int model = 0;
    // 启动默认通道：0=网口 1=串口
    int defaultCommType = 0;
    // 串口索引（0→COM1，与库 iComPort 一致）
    int comPort = 4;
    // 波特率数值（用于匹配下拉文本）
    int baudRate = 115200;
};

// 按搜索路径加载 peerconfig.ini；不存在则返回结构体内缺省值
PeerConfig LoadPeerConfig();
