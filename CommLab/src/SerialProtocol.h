// SerialProtocol.h — 软件侧串口配置与业务回发，严格通过 CommHandler
#pragma once

#include "CommHandler.h"
#include "UIDef.h"

#include <QString>
#include <vector>

namespace SerialProtocol {

// 配置 SerialPortComm 所需的最小参数集
inline void configure(CommHandler* comm, int comPort, int baud, int dataBits, int parity, int stopBits,
                      int proto)
{
    DataInput input{};
    input.iChannelState = 1;
    input.iChannelSize = 1;
    input.iTriggerCtrl = 1;
    input.iTransferType = 0;

    DataOutput output{};
    output.iChannelState = 1;
    output.iChannelSize = 1;
    output.iTransferType = 0;

    comm->SetCommType(SERIAL);
    comm->setParameter(QStringLiteral("input"), input);
    comm->setParameter(QStringLiteral("output"), output);
    comm->setParameter(QStringLiteral("iComPort"), comPort);
    comm->setParameter(QStringLiteral("iComBaud"), baud);
    comm->setParameter(QStringLiteral("iDataBits"), dataBits);
    comm->setParameter(QStringLiteral("iParity"), parity);
    comm->setParameter(QStringLiteral("iStopBits"), stopBits);
    comm->setParameter(QStringLiteral("iProtocolType"), proto);
    comm->setParameter(QStringLiteral("dInForceRange"), 100.0);
    comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
}

// 判断串口入站第二个 double 是否为状态位而非温度
inline bool secondValueIsStatus(int proto)
{
    return proto == 0 || proto == 1;
}

// 判断当前串口协议能否形成“收测数→处理→回发”闭环
inline bool canReplyToMeasurement(int proto)
{
    return proto == 0 || proto == 1;
}

// 返回串口自动业务回发不可用原因
inline QString replyUnavailableReason(int proto)
{
    if (proto == 2)
        return QStringLiteral("时代新材库 SendData 为空实现");
    if (proto == 3)
        return QStringLiteral("IEEE 库无测数入站，不能触发业务回发");
    if (proto == 4)
        return QStringLiteral("冠腾库无测数入站，不能触发业务回发");
    return QStringLiteral("该串口协议无业务闭环");
}

// 经 SerialPortComm::SendData 回发处理后的真实测量结果
inline bool sendProcessed(CommHandler* comm, int proto, double force, bool hasTemp, double temp)
{
    if (!comm || !canReplyToMeasurement(proto))
        return false;
    comm->SetCommType(SERIAL);
    comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
    if (proto == 1 || !hasTemp)
        comm->SendData(std::vector<double>{force}, SERIAL);
    else
        comm->SendData(std::vector<double>{force, temp}, SERIAL);
    return true;
}

} // namespace SerialProtocol
