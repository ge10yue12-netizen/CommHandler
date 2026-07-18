// ProtocolResult.h — 试验机解析软件回包的通用结果
#pragma once

#include <QString>

// 统一表达协议 ACK、业务数值与解析失败
struct ProtocolResult {
    bool ok = false;
    bool isAck = false;
    bool hasValues = false;
    bool hasTemp = false;
    double force = 0.0;
    double temp = 0.0;
    QString detail;
};
