// DemoComm.h — MachinePeer 试验机侧：只收指令、只发测量（仅调库 API）
#pragma once

#include "CommHandler.h"
#include "ProtoGuide.h"
#include "UIDef.h"

#include <QString>
#include <vector>

// 试验机侧 IO：parseCtrl=true 解析软件指令；不发指令
inline void applyIoFlags(CommHandler* c, bool onlineCollect, bool parseCtrl = true)
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

// 试验机回发测量（禁止发 JSON tn=1/3 等指令）
inline bool sendValues(CommHandler* c, bool udpDataPlane, int proto, const QVector<double>& v, QString* fail)
{
    if (v.isEmpty()) {
        if (fail) *fail = QStringLiteral("请填写测量值，如 1.0");
        return false;
    }
    if (udpDataPlane) {
        if (proto == 3 || proto == 5) {
            if (fail) *fail = QStringLiteral("库网口协议 %1 仅收测量").arg(proto);
            return false;
        }
        c->SetCommType(NETWORK);
        c->setParameter(QStringLiteral("bInquireSendFlag"), true);
        c->SendData(std::vector<double>(v.begin(), v.end()), NETWORK);
        return true;
    }
    if (!ProtoGuide::canSendValues(false, proto)) {
        if (fail) *fail = QStringLiteral("库无 SendData(vector) 串口能力");
        return false;
    }
    c->SetCommType(SERIAL);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);
    c->SendData(std::vector<double>(v.begin(), v.end()), SERIAL);
    return true;
}
