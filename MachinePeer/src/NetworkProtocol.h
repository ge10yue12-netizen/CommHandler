// NetworkProtocol.h — 试验机网口协议适配，严格对齐 SocketComm
#pragma once

#include "ProtocolResult.h"

#include <QByteArray>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QUdpSocket>
#include <cstring>

namespace NetworkProtocol {

#pragma pack(push, 1)
// 中机控制帧，匹配 SocketProtol::ControlMessage
struct ZhongjiControl {
    quint8 type;
    quint8 command;
};

// 中机测数帧，匹配 SocketProtol::ForceData
struct ZhongjiMeasurement {
    quint8 type;
    double values[4];
};

// 中机理论业务回包，匹配 SocketProtol::ServerData
struct ZhongjiReply {
    quint8 type;
    double valueX;
    double valueY;
};

// 三思网口测数帧，匹配库 Data 布局
struct SansiMeasurement {
    double force;
    double move;
    double strain;
};
#pragma pack(pop)

static_assert(sizeof(ZhongjiControl) == 2, "Unexpected ZhongjiControl layout");
static_assert(sizeof(ZhongjiMeasurement) == 33, "Unexpected ZhongjiMeasurement layout");
static_assert(sizeof(ZhongjiReply) == 17, "Unexpected ZhongjiReply layout");
static_assert(sizeof(SansiMeasurement) == 24, "Unexpected SansiMeasurement layout");

// 判断动态库网口收路径是否支持指定控制指令
inline bool canSendCommand(int proto, int command)
{
    if (proto == 0 || proto == 1 || proto == 2 || proto == 6 || proto == 7)
        return command == 0 || command == 1;
    return proto == 4 && command == 2;
}

// 判断动态库网口收路径是否能产生测量数据信号
inline bool canSendMeasurement(int proto)
{
    return proto == 2 || proto == 3 || proto == 5;
}

// 返回动态库网口入站能力限制
inline QString unsupportedReason(int proto, bool command)
{
    if (command) {
        if (proto == 3)
            return QStringLiteral("三思网口库无控制指令");
        if (proto == 5)
            return QStringLiteral("福建威盛库无控制指令");
        if (proto == 4)
            return QStringLiteral("触发存图协议仅支持存图");
        return QStringLiteral("该网口协议不支持此指令");
    }
    if (proto == 0)
        return QStringLiteral("JSON 库不解析业务测数");
    if (proto == 1)
        return QStringLiteral("万测库仅解析控制文本");
    if (proto == 4)
        return QStringLiteral("触发存图协议无测数");
    if (proto == 6 || proto == 7)
        return QStringLiteral("纳百川库无常规测数入站");
    return QStringLiteral("该网口协议无测数入站");
}

// 绑定试验机本机 UDP 地址
inline bool open(QUdpSocket* socket, const QString& localIp, int localPort, QString* error)
{
    if (!socket) {
        if (error)
            *error = QStringLiteral("UDP 对象无效");
        return false;
    }
    socket->close();
    if (socket->bind(QHostAddress(localIp), static_cast<quint16>(localPort),
                     QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
        return true;
    if (error)
        *error = socket->errorString();
    return false;
}

// 关闭试验机 UDP
inline void close(QUdpSocket* socket)
{
    if (socket)
        socket->close();
}

// 将完整 UDP 数据报写到软件端
inline bool write(QUdpSocket* socket, const QString& destIp, int destPort, const QByteArray& data)
{
    return socket && !data.isEmpty()
        && socket->writeDatagram(data, QHostAddress(destIp), static_cast<quint16>(destPort))
            == data.size();
}

// 按 SocketComm 控制分支组网口指令
inline QByteArray packCommand(int proto, int command, QString* error)
{
    if (!canSendCommand(proto, command)) {
        if (error)
            *error = unsupportedReason(proto, true);
        return {};
    }
    if (proto == 0)
        return command == 0 ? QByteArray("{\"tn\":1}") : QByteArray("{\"tn\":3}");
    if (proto == 1)
        return command == 0 ? QByteArray("START") : QByteArray("STOP");
    if (proto == 2) {
        const ZhongjiControl control = {0x01, command == 0 ? quint8(0x0A) : quint8(0x0B)};
        return QByteArray(reinterpret_cast<const char*>(&control), sizeof(control));
    }
    if (proto == 4)
        return QByteArray("SAVE\r\n");
    if (proto == 6)
        return command == 0 ? QByteArray("{\"name\":\"alphaStartTest\"}")
                            : QByteArray("{\"name\":\"alphaStopTest\"}");
    return command == 0 ? QByteArray("calcStart") : QByteArray("calcEnd");
}

// 按 SocketComm 测数分支组网口测量帧
inline QByteArray packMeasurement(int proto, double force, double temp, QString* error)
{
    if (!canSendMeasurement(proto)) {
        if (error)
            *error = unsupportedReason(proto, false);
        return {};
    }
    if (proto == 2) {
        ZhongjiMeasurement measurement{};
        measurement.type = 0x02;
        measurement.values[0] = force;
        measurement.values[1] = temp;
        return QByteArray(reinterpret_cast<const char*>(&measurement), sizeof(measurement));
    }
    if (proto == 3) {
        const SansiMeasurement measurement = {force, 0.0, 0.0};
        return QByteArray(reinterpret_cast<const char*>(&measurement), sizeof(measurement));
    }
    return QStringLiteral("TL%1E0D0M0").arg(force, 0, 'f', 2).toLatin1();
}

// 解析 JSON tn=2 业务包或控制 ACK
inline ProtocolResult parseJson(const QByteArray& raw)
{
    ProtocolResult result;
    const QJsonDocument document = QJsonDocument::fromJson(raw);
    if (!document.isObject()) {
        result.detail = QStringLiteral("JSON 格式无效");
        return result;
    }
    const QJsonObject object = document.object();
    const QJsonArray values = object.value(QStringLiteral("values")).toArray();
    if (object.value(QStringLiteral("tn")).toInt(-1) == 2 && !values.isEmpty()) {
        result.ok = true;
        result.hasValues = true;
        result.hasTemp = values.size() > 1;
        result.force = values.at(0).toDouble();
        if (result.hasTemp)
            result.temp = values.at(1).toDouble();
        result.detail = QStringLiteral("JSON tn=2");
        return result;
    }
    result.ok = true;
    result.isAck = true;
    result.detail = QStringLiteral("JSON ACK tn=%1 code=%2")
                        .arg(object.value(QStringLiteral("tn")).toInt(-1))
                        .arg(object.value(QStringLiteral("code")).toInt());
    return result;
}

// 解析万测分隔文本业务包
inline ProtocolResult parseWance(QByteArray raw)
{
    ProtocolResult result;
    raw.replace(';', ',');
    raw.replace(' ', ',');
    const QStringList parts =
        QString::fromLatin1(raw).trimmed().split(QLatin1Char(','), Qt::SkipEmptyParts);
    bool forceOk = false;
    if (!parts.isEmpty())
        result.force = parts.at(0).toDouble(&forceOk);
    if (!forceOk) {
        result.detail = QStringLiteral("万测回包无有效力值");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    if (parts.size() > 1) {
        bool tempOk = false;
        result.temp = parts.at(1).toDouble(&tempOk);
        result.hasTemp = tempOk;
    }
    result.detail = QStringLiteral("万测文本");
    return result;
}

// 解析中机控制 ACK，并识别库二进制转 QString 的已知缺陷
inline ProtocolResult parseZhongji(const QByteArray& raw)
{
    ProtocolResult result;
    if (raw.size() == static_cast<int>(sizeof(ZhongjiControl))) {
        ZhongjiControl control{};
        std::memcpy(&control, raw.constData(), sizeof(control));
        if (control.type != 0x03) {
            result.detail =
                QStringLiteral("中机 2B 帧不是 ACK：type=%1").arg(control.type);
            return result;
        }
        result.ok = true;
        result.isAck = true;
        result.detail =
            QStringLiteral("中机 ACK type=%1 cmd=0x%2").arg(control.type).arg(control.command, 0, 16);
        return result;
    }
    if (raw.size() == static_cast<int>(sizeof(ZhongjiReply))) {
        ZhongjiReply reply{};
        std::memcpy(&reply, raw.constData(), sizeof(reply));
        if (reply.type == 0x04) {
            result.ok = true;
            result.hasValues = true;
            result.hasTemp = true;
            result.force = reply.valueX;
            result.temp = reply.valueY;
            result.detail = QStringLiteral("中机 ServerData");
            return result;
        }
    }
    if (raw == QByteArray(1, char(0x04))) {
        result.detail = QStringLiteral("库缺陷：ServerData 经 QString 截断为 04，数值不可恢复");
        return result;
    }
    result.detail = QStringLiteral("中机帧长或类型不匹配");
    return result;
}

// 解析纳百川 LVEComputedValues JSON 业务包
inline ProtocolResult parseNbc(const QByteArray& raw)
{
    ProtocolResult result;
    const QJsonObject object = QJsonDocument::fromJson(raw).object();
    const QJsonArray values = object.value(QStringLiteral("values")).toArray();
    if (object.value(QStringLiteral("name")).toString() != QLatin1String("LVEComputedValues")
        || values.isEmpty()) {
        result.detail = QStringLiteral("纳百川回包不是业务结果");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = values.size() > 1;
    result.force = values.at(0).toDouble();
    if (result.hasTemp)
        result.temp = values.at(1).toDouble();
    result.detail = QStringLiteral("纳百川 LVEComputedValues");
    return result;
}

// 解析纳百川线条 hsm JSON 业务包
inline ProtocolResult parseNbcLine(const QByteArray& raw)
{
    ProtocolResult result;
    const QJsonObject hsm =
        QJsonDocument::fromJson(raw).object().value(QStringLiteral("hsm")).toObject();
    if (hsm.isEmpty()) {
        result.detail = QStringLiteral("纳百川线条回包缺少 hsm");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = true;
    result.force = hsm.value(QStringLiteral("L1")).toString().toDouble();
    result.temp = hsm.value(QStringLiteral("b1")).toString().toDouble();
    result.detail = QStringLiteral("纳百川线条 hsm");
    return result;
}

// 按网口协议分派软件回包解析
inline ProtocolResult parseReply(int proto, const QByteArray& raw)
{
    if (raw.isEmpty())
        return ProtocolResult{false, false, false, false, 0.0, 0.0, QStringLiteral("空包")};
    if (proto == 0)
        return parseJson(raw);
    if (proto == 1)
        return parseWance(raw);
    if (proto == 2)
        return parseZhongji(raw);
    if (proto == 6)
        return parseNbc(raw);
    if (proto == 7)
        return parseNbcLine(raw);
    return ProtocolResult{true, true, false, false, 0.0, 0.0,
                          QStringLiteral("该协议库无业务回发")};
}

// 判断网口载荷是否适合按文本记录
inline bool isTextFrame(const QByteArray& raw)
{
    if (raw.isEmpty())
        return false;
    const char first = raw.at(0);
    return first == '{' || first == '+' || first == '-' || (first >= '0' && first <= '9');
}

} // namespace NetworkProtocol
