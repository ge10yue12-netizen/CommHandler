// ReplyParse.h — 解析软件经库 SendData 回传的业务包（与库出站格式对齐）
#pragma once

#include "DemoComm.h"
#include "ProtoCapability.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <cstring>

namespace ReplyParse {

// 回包解析结果：业务结果 / 协议 ACK / 无法识别
struct Result {
    // 是否完成协议层识别（含 ACK）
    bool ok = false;
    // true：库自动应答或控制回执，非算好的业务量
    bool isAck = false;
    // true：已解出至少一个业务数值（通常为力）
    bool hasValues = false;
    // true：温度字段由协议编码且已解析；false 时勿打印温=0 造成误解
    bool hasTemp = false;
    // 解析得到的力（或第一通道）
    double force = 0.0;
    // 解析得到的温度（仅当 hasTemp 为 true 时有效）
    double temp = 0.0;
    // 日志用协议说明
    QString detail;
};

// 网口：库对该协议是否有业务 SendData 出站（才值得解力温）
inline bool netHasBusinessReply(int proto)
{
    switch (proto) {
    case 0:
    case 1:
    case 2:
    case 6:
    case 7:
        return true;
    default:
        return false;
    }
}

// 串口：库是否有业务 SendData 出站
inline bool serialHasBusinessReply(int proto)
{
    switch (proto) {
    case 0:
    case 1:
    case 3:
    case 4:
        return true;
    default:
        return false;
    }
}

#pragma pack(push, 1)
// 与库 SocketProtol::ServerData 同布局（软件→机业务回传）
struct ZhongjiServerData {
    quint8 msgType; // 0x04
    double valueX;
    double valueY;
};
#pragma pack(pop)

// 解析网口 JSON 业务包：须 tn=2 且含 values；tn=1/3 等视为 ACK
inline Result parseNetJson(const QByteArray& raw)
{
    Result r;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        r.detail = QStringLiteral("非 JSON");
        return r;
    }
    const QJsonObject obj = doc.object();
    const int tn = obj.value(QStringLiteral("tn")).toInt(-1);
    // 业务结果包：与 CreateJsonDataPack 同形
    if (tn == 2 && obj.contains(QStringLiteral("values")) && obj.value(QStringLiteral("values")).isArray()) {
        const QJsonArray arr = obj.value(QStringLiteral("values")).toArray();
        if (arr.isEmpty()) {
            r.detail = QStringLiteral("values 为空");
            return r;
        }
        r.ok = true;
        r.hasValues = true;
        r.hasTemp = arr.size() >= 2;
        r.force = arr.at(0).toDouble();
        r.temp = r.hasTemp ? arr.at(1).toDouble() : 0.0;
        r.detail = QStringLiteral("JSON tn=2");
        return r;
    }
    // 指令自动应答等
    r.ok = true;
    r.isAck = true;
    r.detail = QStringLiteral("JSON ACK tn=%1 code=%2")
                   .arg(tn)
                   .arg(obj.value(QStringLiteral("code")).toInt());
    return r;
}

// 解析万测文本：分隔符连接的若干 double（CreateWanceSimbolDataPack）
inline Result parseNetWance(const QByteArray& raw)
{
    Result r;
    QString s = QString::fromUtf8(raw).trimmed();
    if (s.isEmpty()) {
        r.detail = QStringLiteral("空包");
        return r;
    }
    // 库可能用 , 空格 ;
    s.replace(QLatin1Char(';'), QLatin1Char(','));
    s.replace(QLatin1Char(' '), QLatin1Char(','));
    const QStringList parts = s.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        r.detail = QStringLiteral("无数值");
        return r;
    }
    bool ok0 = false;
    r.force = parts.at(0).toDouble(&ok0);
    if (!ok0) {
        r.detail = QStringLiteral("首字段非数");
        return r;
    }
    r.ok = true;
    r.hasValues = true;
    r.hasTemp = false;
    if (parts.size() >= 2) {
        bool ok1 = false;
        r.temp = parts.at(1).toDouble(&ok1);
        // 第二字段成功转为数值时才记温度
        r.hasTemp = ok1;
        if (!ok1)
            r.temp = 0.0;
    }
    r.detail = QStringLiteral("万测文本");
    return r;
}

// 解析中机：ServerData 业务 / ControlMessage 回执
inline Result parseNetZhongji(const QByteArray& raw)
{
    Result r;
    if (raw.size() == static_cast<int>(sizeof(ZhongjiServerData))) {
        ZhongjiServerData sd{};
        std::memcpy(&sd, raw.constData(), sizeof(sd));
        if (sd.msgType == 0x04) {
            r.ok = true;
            r.hasValues = true;
            r.hasTemp = true;
            r.force = sd.valueX;
            r.temp = sd.valueY;
            r.detail = QStringLiteral("中机 ServerData");
            return r;
        }
    }
    if (raw.size() == static_cast<int>(sizeof(ZhongjiWire::ControlMessage))) {
        ZhongjiWire::ControlMessage cm{};
        std::memcpy(&cm, raw.constData(), sizeof(cm));
        r.ok = true;
        r.isAck = true;
        r.detail = QStringLiteral("中机控制回执 type=%1 cmd=0x%2")
                       .arg(cm.msgType)
                       .arg(cm.command, 0, 16);
        return r;
    }
    r.detail = QStringLiteral("中机帧长不匹配");
    return r;
}

// 解析纳百川：name=LVEComputedValues + values
inline Result parseNetNbc(const QByteArray& raw)
{
    Result r;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        r.detail = QStringLiteral("非 JSON");
        return r;
    }
    const QJsonObject obj = doc.object();
    const QString name = obj.value(QStringLiteral("name")).toString();
    if (name == QLatin1String("LVEComputedValues") && obj.contains(QStringLiteral("values"))) {
        const QJsonArray arr = obj.value(QStringLiteral("values")).toArray();
        if (arr.isEmpty()) {
            r.detail = QStringLiteral("values 为空");
            return r;
        }
        r.ok = true;
        r.hasValues = true;
        r.hasTemp = arr.size() >= 2;
        r.force = arr.at(0).toDouble();
        r.temp = r.hasTemp ? arr.at(1).toDouble() : 0.0;
        r.detail = QStringLiteral("纳百川 LVEComputedValues");
        return r;
    }
    r.detail = QStringLiteral("非业务计算结果包");
    return r;
}

// 解析纳百川线条：根对象 hsm.L1/b1 等（GenJsonDocument）
inline Result parseNetNbcLine(const QByteArray& raw)
{
    Result r;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        r.detail = QStringLiteral("非 JSON");
        return r;
    }
    const QJsonObject hsm = doc.object().value(QStringLiteral("hsm")).toObject();
    if (hsm.isEmpty()) {
        r.detail = QStringLiteral("无 hsm");
        return r;
    }
    r.ok = true;
    r.hasValues = true;
    r.hasTemp = true;
    r.force = hsm.value(QStringLiteral("L1")).toString().toDouble();
    r.temp = hsm.value(QStringLiteral("b1")).toString().toDouble();
    r.detail = QStringLiteral("纳百川线条 hsm");
    return r;
}

// 按网口协议分派解析软件回包
inline Result parseNetReply(int proto, const QByteArray& raw)
{
    Result r;
    if (raw.isEmpty()) {
        r.detail = QStringLiteral("空包");
        return r;
    }
    if (!netHasBusinessReply(proto)) {
        // 仍可能有非业务流量：尽量记原文类别
        r.ok = true;
        r.isAck = true;
        r.detail = QStringLiteral("本协议库无业务回发，原文仅记录");
        return r;
    }
    switch (proto) {
    case 0:
        return parseNetJson(raw);
    case 1:
        return parseNetWance(raw);
    case 2:
        return parseNetZhongji(raw);
    case 6:
        return parseNetNbc(raw);
    case 7:
        return parseNetNbcLine(raw);
    default:
        r.detail = QStringLiteral("未知网口协议");
        return r;
    }
}

// 串口三思出站：FloatToHex 段为 ASCII 符号数字，末字节 0x0D
inline Result parseSerialSansi(const QByteArray& raw)
{
    Result r;
    QByteArray body = raw;
    if (!body.isEmpty() && static_cast<unsigned char>(body.at(body.size() - 1)) == 0x0D)
        body.chop(1);
    // 按 +/- 切段为 ASCII 浮点文本
    const QString text = QString::fromLatin1(body);
    QRegularExpression re(QStringLiteral("([+-][0-9.*]+)"));
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    QStringList nums;
    while (it.hasNext()) {
        QString tok = it.next().captured(1);
        tok.remove(QLatin1Char('*'));
        nums.append(tok);
    }
    if (nums.isEmpty()) {
        // 退化：整段当一个数
        bool ok = false;
        const double v = text.trimmed().toDouble(&ok);
        if (!ok) {
            r.detail = QStringLiteral("三思回包无法拆数");
            return r;
        }
        r.ok = true;
        r.hasValues = true;
        r.hasTemp = false;
        r.force = v;
        r.detail = QStringLiteral("三思 HEX/ASCII");
        return r;
    }
    r.ok = true;
    r.hasValues = true;
    r.hasTemp = nums.size() >= 2;
    r.force = nums.at(0).toDouble();
    r.temp = r.hasTemp ? nums.at(1).toDouble() : 0.0;
    r.detail = QStringLiteral("三思 ASCII 段");
    return r;
}

// 串口科新出站：FixedDoubleToHex 仅一通道力 + '#'；无温度字段（温=0 为误读）
inline Result parseSerialKexin(const QByteArray& raw)
{
    Result r;
    const QString text = QString::fromLatin1(raw);
    const int hash = text.indexOf(QLatin1Char('#'));
    QString num = (hash > 0) ? text.left(hash) : text;
    // 去掉未初始化前导（如 CDCD），从数字/符号起截取
    int start = 0;
    while (start < num.size()) {
        const QChar c = num.at(start);
        if (c.isDigit() || c == QLatin1Char('+') || c == QLatin1Char('-') || c == QLatin1Char('.'))
            break;
        ++start;
    }
    num = num.mid(start).trimmed();
    bool ok = false;
    r.force = num.toDouble(&ok);
    if (!ok) {
        r.detail = QStringLiteral("科新回包非数值");
        return r;
    }
    r.ok = true;
    r.hasValues = true;
    // 库出站无温：hasTemp 保持 false，日志只打力
    r.hasTemp = false;
    r.detail = QStringLiteral("科新定长(仅力)");
    return r;
}

// 串口冠腾：R{力}#N{温}#（formatValue）
inline Result parseSerialGuanteng(const QByteArray& raw)
{
    Result r;
    const QString text = QString::fromLatin1(raw);
    QRegularExpression re(QStringLiteral("R([+-]?[0-9.]+)#N([+-]?[0-9.]+)#"));
    const QRegularExpressionMatch m = re.match(text);
    if (!m.hasMatch()) {
        r.detail = QStringLiteral("冠腾帧不匹配");
        return r;
    }
    r.ok = true;
    r.hasValues = true;
    r.hasTemp = true;
    r.force = m.captured(1).toDouble();
    r.temp = m.captured(2).toDouble();
    r.detail = QStringLiteral("冠腾 R#N#");
    return r;
}

// 串口 IEEE：定长二进制，Demo 识别为业务回传但不展开 ASC 通道
inline Result parseSerialIeee(const QByteArray& raw)
{
    Result r;
    if (raw.size() < 16) {
        r.detail = QStringLiteral("IEEE 帧过短");
        return r;
    }
    r.ok = true;
    r.hasValues = false;
    r.detail = QStringLiteral("IEEE 定长回包 %1 字节（Demo 不展开 ASC）").arg(raw.size());
    return r;
}

// 按串口协议分派解析软件回包
inline Result parseSerialReply(int proto, const QByteArray& raw)
{
    Result r;
    if (raw.isEmpty()) {
        r.detail = QStringLiteral("空包");
        return r;
    }
    if (!serialHasBusinessReply(proto)) {
        r.ok = true;
        r.isAck = true;
        r.detail = QStringLiteral("本协议库无业务回发");
        return r;
    }
    switch (proto) {
    case 0:
        return parseSerialSansi(raw);
    case 1:
        return parseSerialKexin(raw);
    case 3:
        return parseSerialIeee(raw);
    case 4:
        return parseSerialGuanteng(raw);
    default:
        r.detail = QStringLiteral("未知串口协议");
        return r;
    }
}

} // namespace ReplyParse
