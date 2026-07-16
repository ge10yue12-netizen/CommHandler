// DemoComm.h — CommLab 软件侧：只发指令、只收测量（仅调库 API）
#pragma once

#include "CommHandler.h"
#include "ProtoGuide.h"
#include "UIDef.h"

#include <QString>

// 软件侧 IO：parseCtrl=false，不把入站 JSON/串口控制帧解析为 emitEventMsg
inline void applyIoFlags(CommHandler* c, bool onlineCollect, bool parseCtrl = false)
{
    DataInput in{};
    in.iChannelState = 1;
    in.iChannelSize = 1;
    in.iTriggerCtrl = parseCtrl ? 1 : 0;
    in.iTransferType = 0;
    DataOutput out{};
    out.iChannelState = 1;
    out.iChannelSize = 1;
    out.iTriggerCtrl = 1;
    out.iTransferType = 0;
    c->setParameter(QStringLiteral("input"), in);
    c->setParameter(QStringLiteral("output"), out);
    c->setParameter(QStringLiteral("bOnlineCollect"), onlineCollect);
    c->setParameter(QStringLiteral("bAllOpenCamera"), true);
}

// 断开串口与网口
inline void disconnectAll(CommHandler* c)
{
    c->SetCommType(SERIAL);
    c->Disconnect(0);
    c->SetCommType(NETWORK);
    c->Disconnect(0);
}

// 读取网口 bInquireSendFlag
inline bool queryNetSendFlag(CommHandler* c)
{
    try {
        c->SetCommType(NETWORK);
        return c->getParameter<bool>(QStringLiteral("bInquireSendFlag"));
    } catch (...) {
        return false;
    }
}

// 发 Start/Stop 前同步库内 JSON 状态，避免回包干扰
inline void syncNetCollectingForCmd(CommHandler* c, ProtoGuide::CommCmd cmd)
{
    if (cmd != ProtoGuide::CommCmd::Start && cmd != ProtoGuide::CommCmd::Stop)
        return;
    c->SetCommType(NETWORK);
    c->setParameter(QStringLiteral("bOnlineCollect"), cmd == ProtoGuide::CommCmd::Start);
}

// 软件经串口发三思控制帧（仅出站，试验机收）
inline bool sendSerialSansiCmd(CommHandler* c, ProtoGuide::CommCmd cmd, QString* fail)
{
    const QByteArray hex = ProtoGuide::sansiHexPayload(cmd);
    if (hex.isEmpty()) {
        if (fail) *fail = QStringLiteral("指令未定义");
        return false;
    }
    c->SetCommType(SERIAL);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);
    c->SendData(QString::fromLatin1(hex), SERIAL);
    return true;
}

// 软件经 UDP JSON 发开始/停止（试验机收 emitEventMsg）
inline bool sendNetJsonCmd(CommHandler* c, ProtoGuide::CommCmd cmd, QString* fail)
{
    const QString text = ProtoGuide::jsonCmdText(cmd);
    if (text.isEmpty()) {
        if (fail) *fail = QStringLiteral("指令未定义");
        return false;
    }
    c->SetCommType(NETWORK);
    if (!queryNetSendFlag(c)) {
        if (fail) *fail = QStringLiteral("bInquireSendFlag=false");
        return false;
    }
    syncNetCollectingForCmd(c, cmd);
    c->SendData(text, NETWORK);
    return true;
}

// 软件发四指令（禁止 SendData(vector) 测量）
inline bool sendCommand(CommHandler* c, bool serialDataPlane, int netProto, ProtoGuide::CommCmd cmd,
                        QString* fail)
{
    if (serialDataPlane) {
        if (cmd == ProtoGuide::CommCmd::Start || cmd == ProtoGuide::CommCmd::Stop)
            return sendNetJsonCmd(c, cmd, fail);
        return sendSerialSansiCmd(c, cmd, fail);
    }
    if (netProto == 0)
        return sendNetJsonCmd(c, cmd, fail);
    if (fail) *fail = QStringLiteral("网口协议 %1 请用 SendData(QString) 自填指令").arg(netProto);
    return false;
}
