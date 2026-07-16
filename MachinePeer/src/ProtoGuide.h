// ProtoGuide.h — 协议显示与解包日志（MachinePeer 试验机侧：只收指令、只发测量）
#pragma once

#include <QComboBox>
#include <QVector>
#include <QString>

namespace ProtoGuide {

struct Cap {
    bool sendVec;
    const char* hint;
};

inline const Cap& cap(bool udp, int p)
{
    static const Cap kUdp[] = {
        {false, "UDP 收 JSON 指令 tn=1/3"},
        {false, "收万测文本指令"},
        {false, "收中机控制"},
        {false, "仅收24B测量"},
        {false, "收\\r\\n触发"},
        {false, "收威盛文本"},
        {false, "收纳百川JSON"},
        {false, "收纳百川线条"},
    };
    static const Cap kSer[] = {
        {true, "三思：串口 E2 测量 + UDP 收指令"},
        {true, "串口发测量"},
        {false, "库无发送"},
        {true, "串口发测量"},
        {true, "串口发测量"},
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

inline bool canSendValues(bool udp, int p) { return !udp && cap(udp, p).sendVec; }

inline QString measurePlaceholder(bool udp, int p)
{
    if (!udp && p == 0) return QStringLiteral("如 1.0 或 1.0,2.0");
    if (!udp && p == 3) return QStringLiteral("须≥5个数");
    if (!udp && p == 4) return QStringLiteral("须2个数");
    return QStringLiteral("逗号分隔测量值");
}

inline QString fmt(const QVector<double>& v)
{
    QStringList p;
    for (double x : v) p << QString::number(x, 'g', 8);
    return QStringLiteral("[%1]").arg(p.join(QLatin1Char(',')));
}

inline QString explainSendVector(bool udp, int p, const QVector<double>& v)
{
    if (udp)
        return QStringLiteral("SendData(vector,NETWORK) · 协议=%1 · 源数据 %2").arg(protoName(true, p), fmt(v));
    return QStringLiteral("SendData(vector,SERIAL) · 协议=%1 · 源数据 %2").arg(protoName(false, p), fmt(v));
}

inline QString explainRecvEvent(int msg, const QString& name)
{
    return QStringLiteral("emitEventMsg 解包 · 事件=%1(0x%2) · 含义=软件指令")
        .arg(name).arg(msg, 0, 16);
}

inline QVector<double> parseValues(const QString& text)
{
    QVector<double> out;
    for (const QString& part : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        bool ok = false;
        const double v = part.trimmed().toDouble(&ok);
        if (!ok) return {};
        out.push_back(v);
    }
    return out;
}

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
