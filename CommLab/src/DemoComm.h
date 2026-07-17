// DemoComm.h — CommLab 库参数配置与按能力门控的业务回发
#pragma once

#include "CommHandler.h"
#include "ProtoCapability.h"
#include "UIDef.h"

#include <QString>
#include <array>
#include <cstring>
#include <vector>

// 断开当前 CommType 上的连接
inline void disconnectCurrent(CommHandler* c)
{
    c->Disconnect(0);
}

// 配置串口：收控制与测数；允许 SendData(vector) 出站（具体协议由库分支决定）
inline void setupSerial(CommHandler* c, int comPort, int baudIdx, int dataBits, int parity, int stopBits,
                        int serialProto)
{
    DataInput in{};
    in.iChannelState = 1;
    in.iChannelSize = 1;
    in.iTriggerCtrl = 1;
    in.iTransferType = 0;
    DataOutput out{};
    out.iChannelState = 1;
    out.iChannelSize = 1;
    out.iTriggerCtrl = 1;
    out.iTransferType = 0;

    c->SetCommType(SERIAL);
    c->setParameter(QStringLiteral("input"), in);
    c->setParameter(QStringLiteral("output"), out);
    c->setParameter(QStringLiteral("bAllOpenCamera"), true);
    c->setParameter(QStringLiteral("iComPort"), comPort);
    c->setParameter(QStringLiteral("iComBaud"), baudIdx);
    c->setParameter(QStringLiteral("iDataBits"), dataBits);
    c->setParameter(QStringLiteral("iParity"), parity);
    c->setParameter(QStringLiteral("iStopBits"), stopBits);
    c->setParameter(QStringLiteral("iProtocolType"), serialProto);
    c->setParameter(QStringLiteral("dInForceRange"), 100.0);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);
}

// 配置网口；万测时写入默认 cCMD=START/STOP/EXIT 以便 ParsingProtocolII 匹配
inline void setupNetwork(CommHandler* c, int transferType, int model, const QString& localIp, int localPort,
                         const QString& destIp, int destPort, int protoType)
{
    DataInput in{};
    in.iChannelState = 1;
    in.iChannelSize = 2;
    in.iTriggerCtrl = 1;
    in.iTransferType = 0;
    in.iDataType[0] = 0;
    in.iDataType[1] = 1;
    DataOutput out{};
    out.iChannelState = 1;
    out.iChannelSize = 2;
    out.iTriggerCtrl = 1;
    out.iTransferType = 0;

    c->SetCommType(NETWORK);
    c->setParameter(QStringLiteral("input"), in);
    c->setParameter(QStringLiteral("output"), out);
    c->setParameter(QStringLiteral("bOnlineCollect"), false);
    c->setParameter(QStringLiteral("bAllOpenCamera"), true);
    c->setParameter(QStringLiteral("iTransferType"), transferType);
    c->setParameter(QStringLiteral("iModel"), model);
    c->setParameter(QStringLiteral("sLocalIP"), localIp);
    c->setParameter(QStringLiteral("iLocalPort"), localPort);
    c->setParameter(QStringLiteral("sDestIP"), destIp);
    c->setParameter(QStringLiteral("iDestPort"), destPort);
    c->setParameter(QStringLiteral("iProtoType"), protoType);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);

    // 万测：库默认 cCMD 为空且仅开开始位，Demo 写入可联调字面量
    if (protoType == 1) {
        std::array<bool, 3> openCmd = {true, true, true};
        c->setParameter(QStringLiteral("bOpenCMD"), openCmd);
        std::array<std::array<char, 16>, 3> cmds{};
        std::strncpy(cmds[0].data(), "START", 15);
        std::strncpy(cmds[1].data(), "STOP", 15);
        std::strncpy(cmds[2].data(), "EXIT", 15);
        c->setParameter(QStringLiteral("cCMD"), cmds);
    }
}

// 业务处理：对入站测数做软件侧处理后再回发（Demo 为可替换的透传实现，禁止用缺省温冒充）
// forceIn：入站力；hasTempIn：是否确有入站温度；tempIn：入站温（仅 hasTempIn 时有效）
// forceOut/hasTempOut/tempOut：处理后结果；hasTempOut 为 false 时回发 vector 不得带温
inline void processForceTemp(double forceIn, bool hasTempIn, double tempIn, double* forceOut, bool* hasTempOut,
                             double* tempOut)
{
    // Demo 算法占位：力透传。产品在此替换为标定/滤波等真实计算。
    if (forceOut)
        *forceOut = forceIn;
    if (hasTempOut)
        *hasTempOut = hasTempIn;
    if (tempOut)
        *tempOut = hasTempIn ? tempIn : 0.0;
}

// 按库出站能力组 vector 并调用 CommHandler::SendData；includeTemp 为 false 时只发力
// 返回 true 表示已调用 SendData；false 表示协议无出站或参数无效（未调用 SendData）
inline bool replyProcessed(CommHandler* c, CommType type, int proto, double force, bool includeTemp, double temp)
{
    if (!c)
        return false;
    if (!ProtoCapability::canBusinessReply(type, proto))
        return false;

    c->SetCommType(type);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);
    std::vector<double> v;
    if (type == SERIAL) {
        switch (proto) {
        case 3: // IEEE：库要求不少于 5 个数
            v = {force, includeTemp ? temp : 0.0, 0.0, 0.0, 0.0};
            break;
        case 4: // 冠腾：库要求力+温两个数
            if (!includeTemp)
                return false;
            v = {force, temp};
            break;
        case 1: // 科新：库 FixedDoubleToHex 仅用 vData[0]
            v = {force};
            break;
        case 2:
            return false;
        case 0: // 三思：入站无温时仅发力，禁止塞缺省 25
            if (includeTemp)
                v = {force, temp};
            else
                v = {force};
            break;
        default:
            if (includeTemp)
                v = {force, temp};
            else
                v = {force};
            break;
        }
    } else {
        switch (proto) {
        case 2: // 中机：恰好 2 个数 → ServerData
            if (!includeTemp)
                return false;
            v = {force, temp};
            break;
        case 7: // 纳百川线条：恰好 4 个数
            v = {force, includeTemp ? temp : 0.0, 0.0, 0.0};
            break;
        default:
            if (includeTemp)
                v = {force, temp};
            else
                v = {force};
            break;
        }
    }
    c->SendData(v, type);
    return true;
}
