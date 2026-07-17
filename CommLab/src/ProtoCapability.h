// ProtoCapability.h — CommLab：按动态库审计的协议门控（有则跟，无则不做）
#pragma once

#include "UIDef.h"

#include <QString>

namespace ProtoCapability {

// 网口：库是否将试验机测数解析为 emitNewData
// proto：库 iProtoType
inline bool netCanReceiveMeasure(int proto)
{
    switch (proto) {
    case 2: // 中机 ForceData
    case 3: // 三思定长 Data
    case 5: // 福建威盛文本力
        return true;
    default:
        // 0 JSON / 1 万测 / 4 触发存图 / 6·7 纳百川：库无常规力温入站
        return false;
    }
}

// 网口：库 SendData(vector) 是否存在出站组包分支
// proto：库 iProtoType
inline bool netCanBusinessReply(int proto)
{
    switch (proto) {
    case 0: // CreateJsonDataPack → tn=2
    case 1: // CreateWanceSimbolDataPack
    case 2: // CreateZhongJiShuangZhouFour
    case 6: // CreateHNBCDataPack
    case 7: // GenJsonDocument（须 4 个数）
        return true;
    default:
        // 3/4/5：库 switch 无对应 case
        return false;
    }
}

// 网口：库是否将指令解析为 emitEventMsg
// proto：库 iProtoType；cmd：0开始 1停止 2存图 3清零
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

// 串口：库是否解析测数入站为 emitNewData
// proto：库 iProtocolType
// 说明：3 IEEE 收路径仅指令后 return，不调 MsgDataInput；4 冠腾无专用 case
inline bool serialCanReceiveMeasure(int proto)
{
    switch (proto) {
    case 0: // 三思 E2
    case 1: // 科新 024C+ASCII
    case 2: // 时代新材 CSV
        return true;
    default:
        return false;
    }
}

// 串口：库 SendData(vector) 是否有出站
// proto：库 iProtocolType
// 说明：3 IEEE / 4 冠腾 有出站但无入站测数，业务回发须另有测数来源
inline bool serialCanBusinessReply(int proto)
{
    switch (proto) {
    case 0: // FloatToHex + 0D
    case 1: // FixedDoubleToHex（仅力）
    case 3: // IEEE 定长 ASC（≥5 个数）
    case 4: // R力#N温#
        return true;
    case 2: // 时代新材：库 case 空实现
        return false;
    default:
        return false;
    }
}

// 串口：库是否解析指令为 emitEventMsg
// proto：库 iProtocolType；cmd：0开始 1停止 2存图 3清零
inline bool serialCanReceiveCmd(int proto, int cmd)
{
    switch (proto) {
    case 0: // 3D/3E/6B/5A
        return cmd >= 0 && cmd <= 3;
    case 3: // 24000D0A / 24FF0D0A
        return cmd == 0 || cmd == 1;
    default:
        // 1 科新 / 2 时代新材 / 4 冠腾：库无开始停止事件（科新 7B 仅置清零标志）
        return false;
    }
}

// 三思/科新 MsgDataInput：values[1] 为 COMDATA_STATUS_ENABLE，不是温度
// proto：库 iProtocolType
inline bool serialSecondIsStatusNotTemp(int proto)
{
    return proto == 0 || proto == 1;
}

// 入站测数帧本身不带温度（三思 E2 / 科新 024C 仅力）
// proto：库 iProtocolType
inline bool serialInboundForceOnly(int proto)
{
    return proto == 0 || proto == 1;
}

// 出站 vector 是否只应含力：科新库只用 v[0]；三思入站无温时不得用缺省温冒充回发
// proto：库 iProtocolType；hasMeasuredTemp：本轮是否收到真实温度通道
inline bool serialReplyForceOnly(int proto, bool hasMeasuredTemp)
{
    if (proto == 1)
        return true;
    if (proto == 0 && !hasMeasuredTemp)
        return true;
    return false;
}

// 兼容旧调用：仅按协议判断（科新恒为仅力）
inline bool serialReplyForceOnly(int proto)
{
    return serialReplyForceOnly(proto, false);
}

// 当前通道是否允许业务回发
// type：SERIAL/NETWORK；proto：对应协议索引
inline bool canBusinessReply(CommType type, int proto)
{
    return type == NETWORK ? netCanBusinessReply(proto) : serialCanBusinessReply(proto);
}

// 网口测数能力拒绝原因（日志）
inline QString netMeasureDenyReason(int proto)
{
    switch (proto) {
    case 0:
        return QStringLiteral("JSON 库只解析指令1/3，不解析 values/tn=2 测数");
    case 1:
        return QStringLiteral("万测库仅匹配 CMD 文本，无 emitNewData");
    case 4:
        return QStringLiteral("触发存图协议无测数入站");
    case 6:
    case 7:
        return QStringLiteral("纳百川库无常规力温 emitNewData（仅事件/标距）");
    default:
        return QStringLiteral("本网口协议库无测数入站");
    }
}

// 网口业务回发拒绝原因（日志）
inline QString netReplyDenyReason(int proto)
{
    switch (proto) {
    case 3:
        return QStringLiteral("三思网口库无 SendData(vector) 出站");
    case 4:
        return QStringLiteral("触发存图协议无业务回发");
    case 5:
        return QStringLiteral("福建威盛库无 SendData(vector) 出站");
    default:
        return QStringLiteral("本协议库无业务回发");
    }
}

} // namespace ProtoCapability
