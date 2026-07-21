#include "LegacyCapability.h"

namespace ca {

QString legacyCapabilityName(LegacyCapability c)
{
    switch (c) {
    case LegacyCapability::ReceiveValues: return QStringLiteral("ReceiveValues");
    case LegacyCapability::ReceiveControlEvents: return QStringLiteral("ReceiveControlEvents");
    case LegacyCapability::ReceiveParameterEvents: return QStringLiteral("ReceiveParameterEvents");
    case LegacyCapability::SendEncodedValues: return QStringLiteral("SendEncodedValues");
    case LegacyCapability::SendTransparentText: return QStringLiteral("SendTransparentText");
    case LegacyCapability::RequiresPollingPermission: return QStringLiteral("RequiresPollingPermission");
    case LegacyCapability::RequiresStreamingState: return QStringLiteral("RequiresStreamingState");
    case LegacyCapability::RawReceive: return QStringLiteral("RawReceive");
    case LegacyCapability::RawSend: return QStringLiteral("RawSend");
    }
    return QStringLiteral("Unknown");
}

static LegacyCapabilityEntry yes()
{
    LegacyCapabilityEntry e;
    e.supported = true;
    return e;
}

static LegacyCapabilityEntry no(const QString& note = QString(), const QString& lim = QString())
{
    LegacyCapabilityEntry e;
    e.supported = false;
    e.verificationNote = note;
    e.limitation = lim;
    return e;
}

static void put(LegacyCapabilityProfile* p, LegacyCapability c, bool s, const QString& note = QString(),
                const QString& lim = QString())
{
    LegacyCapabilityEntry e;
    e.supported = s;
    e.verificationNote = note;
    e.limitation = lim;
    p->entries.insert(static_cast<int>(c), e);
}

static void setRawAlwaysOff(LegacyCapabilityProfile* p)
{
    put(p, LegacyCapability::RawReceive, false, QStringLiteral("兼容动态库不支持原始抓包"));
    put(p, LegacyCapability::RawSend, false, QStringLiteral("兼容动态库不支持原始发送"));
    put(p, LegacyCapability::RequiresPollingPermission, false);
    put(p, LegacyCapability::RequiresStreamingState, false);
}

LegacyCapabilityProfile legacyCapabilityFor(LegacyCommKind kind, int protocolIndex)
{
    LegacyCapabilityProfile p;
    p.commKind = kind;
    p.protocolIndex = protocolIndex;
    setRawAlwaysOff(&p);

    if (kind == LegacyCommKind::Network) {
        switch (protocolIndex) {
        case 0:
            put(&p, LegacyCapability::ReceiveValues, false, QStringLiteral("网口 JSON 数值接收不支持"));
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            put(&p, LegacyCapability::SendEncodedValues, true);
            put(&p, LegacyCapability::SendTransparentText, true);
            break;
        case 1:
            put(&p, LegacyCapability::ReceiveValues, false);
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            put(&p, LegacyCapability::SendEncodedValues, true);
            put(&p, LegacyCapability::SendTransparentText, true);
            break;
        case 2:
            put(&p, LegacyCapability::ReceiveValues, true);
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            put(&p, LegacyCapability::SendEncodedValues, true);
            put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("中机透明文本不支持"));
            break;
        case 3:
            put(&p, LegacyCapability::ReceiveValues, true);
            put(&p, LegacyCapability::ReceiveControlEvents, false, QStringLiteral("三思控制事件未经验证，按不支持处理"));
            put(&p, LegacyCapability::ReceiveParameterEvents, false);
            // 对齐 SocketComm::SendData：无 case 3，数值发送不会写出线帧
            put(&p, LegacyCapability::SendEncodedValues, false,
                QStringLiteral("动态库 SendData(vector) 未实现网口三思编码分支"));
            put(&p, LegacyCapability::SendTransparentText, false);
            break;
        case 4:
            put(&p, LegacyCapability::ReceiveValues, false);
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            put(&p, LegacyCapability::SendEncodedValues, false);
            put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("触发存图透明文本未经验证，按不支持处理"));
            break;
        case 5:
            put(&p, LegacyCapability::ReceiveValues, true);
            put(&p, LegacyCapability::ReceiveControlEvents, false);
            put(&p, LegacyCapability::ReceiveParameterEvents, false);
            put(&p, LegacyCapability::SendEncodedValues, false);
            put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("福建威盛透明文本未经验证，按不支持处理"));
            break;
        case 6:
            put(&p, LegacyCapability::ReceiveValues, false);
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            put(&p, LegacyCapability::SendEncodedValues, true);
            put(&p, LegacyCapability::SendTransparentText, true);
            break;
        case 7:
            put(&p, LegacyCapability::ReceiveValues, false,
                QStringLiteral("纳百川线条收路径为控制/标距事件，不产生数值通道"));
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, true);
            // 对齐 SocketComm::GenJsonDocument：要求恰好 4 个 double
            put(&p, LegacyCapability::SendEncodedValues, true, QString(),
                QStringLiteral("恰好 4 个数值（L1,b1,Le1,bo1）；收侧不回显数值通道"));
            put(&p, LegacyCapability::SendTransparentText, true);
            break;
        case 8:
            // 联恒光科网口（BASELINE V1.2.5 / iProtoType=8）
            put(&p, LegacyCapability::ReceiveValues, true);
            put(&p, LegacyCapability::ReceiveControlEvents, true);
            put(&p, LegacyCapability::ReceiveParameterEvents, false);
            put(&p, LegacyCapability::SendEncodedValues, true, QString(), QStringLiteral("至少 2 个数值（力、温）"));
            put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("联恒不支持透明文本"));
            put(&p, LegacyCapability::RequiresPollingPermission, true);
            put(&p, LegacyCapability::RequiresStreamingState, true);
            break;
        default:
            put(&p, LegacyCapability::ReceiveValues, false, QStringLiteral("未知网口协议"));
            put(&p, LegacyCapability::ReceiveControlEvents, false);
            put(&p, LegacyCapability::ReceiveParameterEvents, false);
            put(&p, LegacyCapability::SendEncodedValues, false);
            put(&p, LegacyCapability::SendTransparentText, false);
            break;
        }
        return p;
    }

    switch (protocolIndex) {
    case 0:
        put(&p, LegacyCapability::ReceiveValues, true);
        put(&p, LegacyCapability::ReceiveControlEvents, true);
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, true);
        put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("SendData(QString) 空实现"));
        break;
    case 1:
        // 收：仅力值 1 路；发：仅编码 vData[0]；控制字 {QLI[1]}/{QLI[2]} 上报开始/停止
        put(&p, LegacyCapability::ReceiveValues, true, QString(),
            QStringLiteral("收 1 路力值；发送仅取 CSV 第 1 个数值（14 字节 02 4C…#）"));
        put(&p, LegacyCapability::ReceiveControlEvents, true, QString(),
            QStringLiteral("{QLI[1]} 开始 / {QLI[2]} 停止"));
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, true, QString(),
            QStringLiteral("仅第 1 个数值写入线帧"));
        put(&p, LegacyCapability::SendTransparentText, false);
        break;
    case 2:
        // 时代新材：value,num,TYPE,flag；库只取 parts[0]
        put(&p, LegacyCapability::ReceiveValues, true, QString(),
            QStringLiteral("收 1 路（CSV 第 1 段）；第 2 段为设备序号非测量通道"));
        put(&p, LegacyCapability::ReceiveControlEvents, false,
            QStringLiteral("时代新材库无独立控制事件分支"));
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, false);
        put(&p, LegacyCapability::SendTransparentText, false);
        break;
    case 3:
        put(&p, LegacyCapability::ReceiveValues, false);
        put(&p, LegacyCapability::ReceiveControlEvents, true);
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, true, QString(), QStringLiteral("至少 5 个数值"));
        put(&p, LegacyCapability::SendTransparentText, false);
        break;
    case 4:
        put(&p, LegacyCapability::ReceiveValues, false);
        put(&p, LegacyCapability::ReceiveControlEvents, false);
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, true, QString(), QStringLiteral("至少 2 个数值"));
        put(&p, LegacyCapability::SendTransparentText, false);
        break;
    case 5:
        // 联恒光科串口（BASELINE V1.2.5 / iProtocolType=5）
        put(&p, LegacyCapability::ReceiveValues, true);
        put(&p, LegacyCapability::ReceiveControlEvents, true);
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, true, QString(), QStringLiteral("至少 2 个数值（力、温）"));
        put(&p, LegacyCapability::SendTransparentText, false, QStringLiteral("SendData(QString) 空实现"));
        put(&p, LegacyCapability::RequiresPollingPermission, true);
        put(&p, LegacyCapability::RequiresStreamingState, true);
        break;
    default:
        put(&p, LegacyCapability::ReceiveValues, false);
        put(&p, LegacyCapability::ReceiveControlEvents, false);
        put(&p, LegacyCapability::ReceiveParameterEvents, false);
        put(&p, LegacyCapability::SendEncodedValues, false);
        put(&p, LegacyCapability::SendTransparentText, false);
        break;
    }
    return p;
}

QStringList unsupportedCapabilityTips(const LegacyCapabilityProfile& profile)
{
    QStringList tips;
    for (auto it = profile.entries.constBegin(); it != profile.entries.constEnd(); ++it) {
        if (it.value().supported)
            continue;
        const auto key = static_cast<LegacyCapability>(it.key());
        if (key == LegacyCapability::RawReceive || key == LegacyCapability::RawSend)
            continue;
        QString line = legacyCapabilityName(key) + QStringLiteral(":不支持");
        if (!it.value().limitation.isEmpty())
            line += QStringLiteral("（") + it.value().limitation + QStringLiteral("）");
        tips << line;
    }
    return tips;
}

} // namespace ca