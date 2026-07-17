// PeerConfig.h — MachinePeer 默认网络/串口参数（可由 peerconfig.ini 覆盖）
#pragma once

#include <QString>

// 试验机测试工具默认参数
struct PeerConfig {
    // 本机 UDP 绑定 IP
    QString localIp = QStringLiteral("127.0.0.1");
    // 本机 UDP 绑定端口（对应软件 destPort）
    int localPort = 19106;
    // 软件侧 IP
    QString destIp = QStringLiteral("127.0.0.1");
    // 软件侧端口（对应软件 localPort）
    int destPort = 19105;
    // 串口索引（0→COM1，与库 iComPort 一致）
    int comPort = 4;
};

// 按搜索路径加载 peerconfig.ini；不存在则返回结构体内缺省值
PeerConfig LoadPeerConfig();
