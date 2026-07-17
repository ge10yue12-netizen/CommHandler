// NetConfig.h — CommLab 网口默认参数（可由 netconfig.ini 覆盖）
#pragma once

#include <QString>

// 软件侧网口连接参数；model=1 表示客户端
struct NetConfig {
    // 本机绑定 IP
    QString localIp = QStringLiteral("127.0.0.1");
    // 本机绑定端口（库 iLocalPort）
    int localPort = 19105;
    // 对端（试验机）IP
    QString destIp = QStringLiteral("127.0.0.1");
    // 对端端口（库 iDestPort）
    int destPort = 19106;
    // 传输类型（库 iTransferType，1=UDP 等）
    int transferType = 1;
    // 角色模型（库 iModel，1=客户端）
    int model = 1;
};

// 按搜索路径加载 netconfig.ini；不存在则返回结构体内缺省值
NetConfig LoadNetConfig();
