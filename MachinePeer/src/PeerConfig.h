#pragma once

#include <QString>

// 试验机侧默认参数（可被 peerconfig.ini 覆盖）。
struct PeerConfig {
    QString localIp = QStringLiteral("127.0.0.1");
    int localPort = 19106;
    QString destIp = QStringLiteral("127.0.0.1");
    int destPort = 19105;
    int transferType = 1;
    int model = 0;
    int comPort = 4;
};

// 加载 peerconfig.ini，找不到则用默认值。
PeerConfig LoadPeerConfig();
