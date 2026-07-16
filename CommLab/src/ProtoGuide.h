// ProtoGuide.h — 协议显示、组包/解包日志（CommLab 软件侧：只发指令、只收测量）
#pragma once

#include <QComboBox>
#include <QVector>
#include <QString>

namespace ProtoGuide {

// 软件侧四指令（与库串口 HEX / JSON tn 对应）
enum class CommCmd {
    Start,
    Stop,
    SaveImage,
    ZeroClear,
};

inline QByteArray sansiHexPayload(CommCmd cmd)
{
    switch (cmd) {
    case CommCmd::Start: return QByteArray("\x3D\x0D", 2);
    case CommCmd::Stop: return QByteArray("\x3E\x0D", 2);
    case CommCmd::SaveImage: return QByteArray("\x6B\x0D", 2);
    case CommCmd::ZeroClear: return QByteArray("\x5A\x0D", 2);
    }
    return {};
}

inline QString jsonCmdText(CommCmd cmd)
{
    switch (cmd) {
    case CommCmd::Start: return QStringLiteral("{\"tn\":1}");
    case CommCmd::Stop: return QStringLiteral("{\"tn\":3}");
    default: return {};
    }
}

struct Cap {
    const char* hint;
};

inline const Cap& cap(bool udp, int p)
{
    static const Cap kUdp[] = {
        {"网口：发 JSON 指令 tn=1/3；不收 tn=2 测量"},
        {"万测：自填 SendData(QString)"},
        {"中机：自填网口文本"},
        {"三思UDP：仅收24B测量"},
        {"触发存图：QString含\\r\\n"},
        {"威盛UDP：仅收文本测量"},
        {"纳百川：自填 JSON"},
        {"纳百川线条：不适用主闭环"},
    };
    static const Cap kSer[] = {
        {"三思：串口 E2 测量帧 + UDP JSON 指令"},
        {"科新：试验机 vector 发测量"},
        {"时代新材：库无发送"},
        {"IEEE：试验机 vector 发测量"},
        {"冠腾：试验机 vector 发测量"},
    };
    if (udp)
        return (p >= 0 && p <= 7) ? kUdp[p] : kUdp[0];
    return (p >= 0 && p <= 4) ? kSer[p] : kSer[0];
}

inline QString protoName(bool udp, int p)
{
    if (udp) {
        static const char* n[] = {"JSON", "万测", "中机", "三思", "触发存图", "福建威盛", "纳百川", "纳百川线条"};
        return (p >= 0 && p <= 7) ? QString::fromUtf8(n[p]) : QStringLiteral("#%1").arg(p);
    }
    static const char* n[] = {"三思", "科新", "时代新材", "IEEE", "冠腾"};
    return (p >= 0 && p <= 4) ? QString::fromUtf8(n[p]) : QStringLiteral("#%1").arg(p);
}

inline QString statusLine(bool udp, int p)
{
    if (!udp)
        return QStringLiteral("串口%1 · UDP JSON 指令面 · %2").arg(protoName(udp, p), QString::fromUtf8(cap(udp, p).hint));
    return QStringLiteral("%1 · %2").arg(protoName(udp, p), QString::fromUtf8(cap(udp, p).hint));
}

inline QString measurePlaceholder(bool, int) { return {}; }

inline QString cmdPlaceholder(bool udp, int p)
{
    if (udp && p == 0) return QStringLiteral("JSON 指令 tn=1/3");
    return QStringLiteral("网口自填文本");
}

inline QString fmt(const QVector<double>& v)
{
    QStringList p;
    for (double x : v) p << QString::number(x, 'g', 8);
    return QStringLiteral("[%1]").arg(p.join(QLatin1Char(',')));
}

// 发指令组包说明
inline QString explainSendCommand(bool dataPlaneUdp, int netProto, CommCmd cmd)
{
    if (!dataPlaneUdp && (cmd == CommCmd::SaveImage || cmd == CommCmd::ZeroClear)) {
        const QByteArray hex = sansiHexPayload(cmd);
        return QStringLiteral("SendData(QString,SERIAL) HEX=%1 · 三思控制帧")
            .arg(QString::fromLatin1(hex.toHex().toUpper()));
    }
    if (dataPlaneUdp || netProto == 0) {
        const QString t = jsonCmdText(cmd);
        if (!t.isEmpty())
            return QStringLiteral("SendData(QString,NETWORK) · %1 · 试验机收 emitEventMsg").arg(t);
    }
    switch (cmd) {
    case CommCmd::Start: return QStringLiteral("开始指令");
    case CommCmd::Stop: return QStringLiteral("停止指令");
    case CommCmd::SaveImage: return QStringLiteral("存图指令 6B0D");
    case CommCmd::ZeroClear: return QStringLiteral("清零指令 5A0D");
    }
    return QStringLiteral("指令");
}

inline QString cmdDisplayName(CommCmd cmd)
{
    switch (cmd) {
    case CommCmd::Start: return QStringLiteral("开始");
    case CommCmd::Stop: return QStringLiteral("停止");
    case CommCmd::SaveImage: return QStringLiteral("存图");
    case CommCmd::ZeroClear: return QStringLiteral("清零");
    }
    return QStringLiteral("指令");
}

// 接收 emitNewData（试验机经串口发来的测量）
inline QString explainRecvData(bool udp, int p, int type, const QVector<double>& v)
{
    QString base = QStringLiteral("emitNewData 解包 · 协议=%1 · 个数=%2 · 值=%3 · type=%4")
                       .arg(protoName(udp, p)).arg(v.size()).arg(fmt(v)).arg(type);
    if (!udp && p == 0)
        return base + QStringLiteral(" · 三思串口 E2 测量帧");
    return base + QStringLiteral(" · 含义=试验机测量");
}

inline QString explainRecvEvent(int msg, const QString& name)
{
    return QStringLiteral("emitEventMsg · 事件=%1(0x%2) · 软件侧不应收指令（请查 iTriggerCtrl）")
        .arg(name).arg(msg, 0, 16);
}

inline QVector<double> parseValues(const QString&) { return {}; }

inline void fillProtoCombos(QComboBox* net, QComboBox* ser)
{
    net->clear();
    const char* np[] = {"0 JSON", "1 万测", "2 中机", "3 三思", "4 触发存图", "5 福建威盛", "6 纳百川", "7 纳百川线条"};
    for (int i = 0; i < 8; ++i) net->addItem(QString::fromUtf8(np[i]), i);
    ser->clear();
    const char* sp[] = {"0 三思", "1 科新", "2 时代新材", "3 IEEE", "4 冠腾"};
    for (int i = 0; i < 5; ++i) ser->addItem(QString::fromUtf8(sp[i]), i);
    ser->setCurrentIndex(0);
}

inline void fillSerialCombos(QComboBox* baud, QComboBox* dataBits,
                             QComboBox* parity, QComboBox* stopBits)
{
    baud->clear();
    const char* bd[] = {"4800", "9600", "19200", "38400", "57600", "115200"};
    for (int i = 0; i < 6; ++i) baud->addItem(QString::fromUtf8(bd[i]), i);
    baud->setCurrentIndex(5);
    dataBits->addItem(QStringLiteral("5"), 0);
    dataBits->addItem(QStringLiteral("6"), 1);
    dataBits->addItem(QStringLiteral("8"), 3);
    dataBits->setCurrentIndex(2);
    parity->addItems({QStringLiteral("N"), QStringLiteral("E"), QStringLiteral("O"),
                      QStringLiteral("S"), QStringLiteral("M")});
    stopBits->addItems({QStringLiteral("1"), QStringLiteral("1.5"), QStringLiteral("2")});
}

} // namespace ProtoGuide
