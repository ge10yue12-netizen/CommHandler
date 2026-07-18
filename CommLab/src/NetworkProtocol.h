// NetworkProtocol.h — 软件侧网口配置与动态库能力边界
#pragma once

#include "CommHandler.h"
#include "UIDef.h"

#include <QString>
#include <array>
#include <cstring>

namespace NetworkProtocol {

// 配置 SocketComm 所需的最小参数集
inline void configure(CommHandler* comm, int transferType, int model, const QString& localIp,
                      int localPort, const QString& destIp, int destPort, int proto)
{
    DataInput input{};
    input.iChannelState = 1;
    input.iChannelSize = 2;
    input.iTriggerCtrl = 1;
    input.iTransferType = 0;
    input.iDataType[0] = 0;
    input.iDataType[1] = 1;

    DataOutput output{};
    output.iChannelState = 1;
    output.iChannelSize = 2;
    output.iTransferType = 0;

    comm->SetCommType(NETWORK);
    comm->setParameter(QStringLiteral("input"), input);
    comm->setParameter(QStringLiteral("output"), output);
    comm->setParameter(QStringLiteral("bOnlineCollect"), false);
    comm->setParameter(QStringLiteral("bAllOpenCamera"), true);
    comm->setParameter(QStringLiteral("iTransferType"), transferType);
    comm->setParameter(QStringLiteral("iModel"), model);
    comm->setParameter(QStringLiteral("sLocalIP"), localIp);
    comm->setParameter(QStringLiteral("iLocalPort"), localPort);
    comm->setParameter(QStringLiteral("sDestIP"), destIp);
    comm->setParameter(QStringLiteral("iDestPort"), destPort);
    comm->setParameter(QStringLiteral("iProtoType"), proto);
    comm->setParameter(QStringLiteral("bInquireSendFlag"), true);

    // 万测依赖可配置字面量；库默认命令数组不能完成 START/STOP 联调。
    if (proto == 1) {
        comm->setParameter(QStringLiteral("bOpenCMD"), std::array<bool, 3>{{true, true, true}});
        std::array<std::array<char, 16>, 3> commands{};
        std::strncpy(commands[0].data(), "START", 15);
        std::strncpy(commands[1].data(), "STOP", 15);
        std::strncpy(commands[2].data(), "EXIT", 15);
        comm->setParameter(QStringLiteral("cCMD"), commands);
    }
}

// 当前动态库没有可用的网口自动业务闭环
inline bool canReplyToMeasurement(int /*proto*/)
{
    return false;
}

// 返回网口自动业务回发不可用原因
inline QString replyUnavailableReason(int proto)
{
    if (proto == 2)
        return QStringLiteral("中机库ServerData疑问");
        //return QStringLiteral("中机库将 17 字节 ServerData 转为 QString，实际仅发 04，数值不可恢复");
    if (proto == 3)
        return QStringLiteral("三思网口库无 SendData(vector) 分支");
    if (proto == 5)
        return QStringLiteral("福建威盛库无 SendData(vector) 分支");
    return QStringLiteral("该网口协议无“测数入站→业务回发”闭环");
}

} // namespace NetworkProtocol
