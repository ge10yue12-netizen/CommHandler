#pragma once

#include <QString>

class CommHandler;

// 网口参数（来自 netconfig.ini；找不到文件时用内置默认值）。
struct NetConfig {
    QString localIp = QStringLiteral("127.0.0.1"); // 本机绑定 IP
    int localPort = 19105;                          // 本机绑定端口
    QString destIp = QStringLiteral("127.0.0.1");  // 目的 IP（setParameter 名 sDestIP）
    int destPort = 19106;                           // 目的端口
    int transferType = 1;                           // iTransferType：1=UDP
    int model = 0;                                  // iModel：0=服务端
    int smokeSansiProto = 3;                        // 自检用三思协议号
    int smokeJsonProto = 0;                         // 自检用 JSON 协议号
    QString sourcePath; // 实际加载路径；空表示内置默认
    bool fromFile = false;
};

// 查找并加载 netconfig.ini（当前目录 → exe 目录）。始终返回可用配置。
NetConfig LoadNetConfig();

// 将网络段写入 CommHandler（不含协议号与发送许可）。
void ApplyNetConfigBase(CommHandler* comm, const NetConfig& cfg);

// ApplyNetConfigBase + iProtoType。
void ConfigureLabUdp(CommHandler* comm, int protoType, const NetConfig& cfg);