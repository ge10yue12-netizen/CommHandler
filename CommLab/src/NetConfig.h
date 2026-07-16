#pragma once

#include <QString>

// 软件侧网口默认参数（可被 netconfig.ini 覆盖）；model=1 客户端。
struct NetConfig {
    QString localIp = QStringLiteral("127.0.0.1");
    int localPort = 19105;
    QString destIp = QStringLiteral("127.0.0.1");
    int destPort = 19106;
    int transferType = 1;
    int model = 1;
};

// 加载 netconfig.ini，找不到则用默认值。
NetConfig LoadNetConfig();
