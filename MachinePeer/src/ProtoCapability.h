// ProtoCapability.h — MachinePeer：与库对齐的协议能力（有则实现，无则提示）
#pragma once

#include <QString>
#include <QtGlobal>

namespace ProtoCapability {

// 网口：库能否 emitNewData（测数入站）
// proto：iProtoType
inline bool netCanReceiveMeasure(int proto)
{
    switch (proto) {
    case 2:
    case 3:
    case 5:
        return true;
    default:
        return false;
    }
}

// 网口：库能否解析指令
// proto：iProtoType；cmd：0开始 1停止 2存图 3清零
inline bool netCanReceiveCmd(int proto, int cmd)
{
    switch (proto) {
    case 0:
    case 1:
    case 2:
    case 6:
    case 7:
        return cmd == 0 || cmd == 1;
    case 4:
        return cmd == 2;
    default:
        return false;
    }
}

// 串口：库能否解析测数入站
// proto：iProtocolType
// 3 IEEE：库有协议位，但 onRecv 仅指令后 return，无测数帧解析 → Demo 不发测数
// 4 冠腾：库有协议位与出站 R#N#，无入站测数/指令 case → Demo 不发测数
inline bool serialCanReceiveMeasure(int proto)
{
    switch (proto) {
    case 0:
    case 1:
    case 2:
        return true;
    default:
        return false;
    }
}

// 串口：库能否解析指令
// proto：iProtocolType；cmd：0开始 1停止 2存图 3清零
inline bool serialCanReceiveCmd(int proto, int cmd)
{
    switch (proto) {
    case 0:
        return cmd >= 0 && cmd <= 3;
    case 3: // IEEE：仅开始/停止 HEX，库有实现
        return cmd == 0 || cmd == 1;
    default:
        return false;
    }
}

// 软件回发是否仅含力（科新 FixedDoubleToHex 只用一通道）
// proto：iProtocolType
inline bool serialSoftReplyForceOnly(int proto)
{
    return proto == 1;
}

// 网口测数拒绝说明
inline QString netMeasureDenyReason(int proto)
{
    switch (proto) {
    case 0:
        return QStringLiteral("JSON 库不解析 values/tn=2，发了也会被丢弃");
    case 1:
        return QStringLiteral("万测库无测数入站");
    case 4:
        return QStringLiteral("触发存图无测数帧");
    case 6:
    case 7:
        return QStringLiteral("纳百川无常规力温入站");
    default:
        return QStringLiteral("本协议库无测数入站");
    }
}

// 串口测数拒绝说明（写明库“有协议、无入站解析”）
inline QString serialMeasureDenyReason(int proto)
{
    switch (proto) {
    case 3:
        return QStringLiteral("IEEE：库有协议，收路径仅开始/停止，无测数解析");
    case 4:
        return QStringLiteral("冠腾：库有协议与出站 R#N#，无入站测数解析");
    default:
        return QStringLiteral("本协议库无测数入站");
    }
}

// 网口指令拒绝说明
inline QString netCmdDenyReason(int proto, int cmd)
{
    Q_UNUSED(cmd);
    switch (proto) {
    case 3:
        return QStringLiteral("三思网口库无开始/停止指令解析");
    case 5:
        return QStringLiteral("福建威盛库无指令事件");
    case 4:
        return QStringLiteral("触发存图仅支持存图类载荷");
    default:
        return QStringLiteral("本协议库无此指令");
    }
}

// 串口指令拒绝说明
inline QString serialCmdDenyReason(int proto, int /*cmd*/)
{
    switch (proto) {
    case 1:
        return QStringLiteral("科新库收路径无开始/停止事件");
    case 2:
        return QStringLiteral("时代新材库无指令事件");
    case 4:
        return QStringLiteral("冠腾库无指令入站（仅软件可 SendData 出站）");
    default:
        return QStringLiteral("本协议库无此指令");
    }
}

} // namespace ProtoCapability
