// SerialProtocol.h — 试验机串口协议适配，严格对齐 SerialPortComm
#pragma once

#include "ProtocolResult.h"

#include <QByteArray>
#include <QIODevice>
#include <QRegularExpression>
#include <QSerialPort>
#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace SerialProtocol {

// 判断动态库串口收路径是否支持指定控制指令
inline bool canSendCommand(int proto, int command)
{
    return (proto == 0 && command >= 0 && command <= 3)
        || (proto == 3 && (command == 0 || command == 1));
}

// 判断动态库串口收路径是否能产生测量数据信号
inline bool canSendMeasurement(int proto)
{
    return proto >= 0 && proto <= 2;
}

// 返回动态库串口入站能力限制
inline QString unsupportedReason(int proto, bool command)
{
    if (command) {
        if (proto == 1)
            return QStringLiteral("科新库不产生控制事件");
        if (proto == 2)
            return QStringLiteral("时代新材库不产生控制事件");
        if (proto == 4)
            return QStringLiteral("冠腾库仅实现业务出站");
        return QStringLiteral("该串口协议不支持此指令");
    }
    if (proto == 3)
        return QStringLiteral("IEEE 库收路径仅解析开始/停止");
    if (proto == 4)
        return QStringLiteral("冠腾库无测数入站");
    return QStringLiteral("该串口协议无测数入站");
}

// 将库波特率索引转换为 Qt 波特率
inline QSerialPort::BaudRate baudRate(int index)
{
    static const QSerialPort::BaudRate rates[] = {
        QSerialPort::Baud4800, QSerialPort::Baud9600, QSerialPort::Baud19200,
        QSerialPort::Baud38400, QSerialPort::Baud57600, QSerialPort::Baud115200,
    };
    return index >= 0 && index < 6 ? rates[index] : QSerialPort::Baud115200;
}

// 按库索引配置并打开试验机串口
inline bool open(QSerialPort* port, int comPort, int baudIndex, int dataBits, int parity, int stopBits,
                 QString* error)
{
    if (!port) {
        if (error)
            *error = QStringLiteral("串口对象无效");
        return false;
    }
    port->close();
    port->setPortName(QStringLiteral("COM%1").arg(comPort + 1));
    port->setBaudRate(baudRate(baudIndex));
    port->setDataBits(dataBits == 0 ? QSerialPort::Data5
                                   : dataBits == 1 ? QSerialPort::Data6 : QSerialPort::Data8);
    port->setParity(parity == 1 ? QSerialPort::EvenParity
                               : parity == 2 ? QSerialPort::OddParity
                                             : parity == 3 ? QSerialPort::SpaceParity
                                                           : parity == 4 ? QSerialPort::MarkParity
                                                                         : QSerialPort::NoParity);
    port->setStopBits(stopBits == 1 ? QSerialPort::OneAndHalfStop
                                   : stopBits == 2 ? QSerialPort::TwoStop : QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    if (port->open(QIODevice::ReadWrite))
        return true;
    if (error)
        *error = port->errorString();
    return false;
}

// 关闭试验机串口
inline void close(QSerialPort* port)
{
    if (port)
        port->close();
}

// 写出一个完整串口帧并返回日志用 HEX
inline bool write(QSerialPort* port, const QByteArray& frame, QString* hex)
{
    if (!port || !port->isOpen() || frame.isEmpty() || port->write(frame) != frame.size())
        return false;
    port->flush();
    if (hex)
        *hex = QString::fromLatin1(frame.toHex().toUpper());
    return true;
}

// 按 SerialPortComm 控制分支组串口指令
inline QByteArray packCommand(int proto, int command, QString* error)
{
    if (!canSendCommand(proto, command)) {
        if (error)
            *error = unsupportedReason(proto, true);
        return {};
    }
    if (proto == 0) {
        static const char frames[][2] = {{char(0x3D), char(0x0D)}, {char(0x3E), char(0x0D)},
                                         {char(0x6B), char(0x0D)}, {char(0x5A), char(0x0D)}};
        return QByteArray(frames[command], 2);
    }
    return command == 0 ? QByteArray("\x24\x00\x0D\x0A", 4)
                        : QByteArray("\x24\xFF\x0D\x0A", 4);
}

// 按 SerialPortComm 测数分支组串口测量帧
inline QByteArray packMeasurement(int proto, double force, double /*temp*/, QString* error)
{
    if (!canSendMeasurement(proto)) {
        if (error)
            *error = unsupportedReason(proto, false);
        return {};
    }
    if (proto == 0) {
        const qint64 raw = qRound64(force * 1000000.0);
        const quint32 value =
            static_cast<quint32>(qBound(0LL, raw, static_cast<qint64>(0xFFFFFFFFu)));
        QByteArray frame(1, char(0xE2));
        frame.append(char((value >> 24) & 0xFF));
        frame.append(char((value >> 16) & 0xFF));
        frame.append(char((value >> 8) & 0xFF));
        frame.append(char(value & 0xFF));
        return frame;
    }
    if (proto == 1) {
        QString body = force < 0.0
            ? QLatin1Char('-') + QStringLiteral("%1").arg(-force, 8, 'f', 4, QLatin1Char('0'))
            : QStringLiteral("%1").arg(force, 9, 'f', 4, QLatin1Char('0'));
        body = body.right(9);
        return QByteArray("\x02\x4C", 2) + body.toLatin1();
    }
    return QStringLiteral("%1,0,RUN,0\r\n").arg(force, 0, 'f', 3).toLatin1();
}

// 解析三思 FloatToHex ASCII 业务回包
inline ProtocolResult parseSansi(const QByteArray& raw)
{
    ProtocolResult result;
    QString text = QString::fromLatin1(raw);
    text.remove(QLatin1Char('\r'));
    QRegularExpression expression(QStringLiteral("([+-][0-9.*]+)"));
    QRegularExpressionMatchIterator matches = expression.globalMatch(text);
    QStringList values;
    while (matches.hasNext()) {
        QString value = matches.next().captured(1);
        value.remove(QLatin1Char('*'));
        values.append(value);
    }
    if (values.isEmpty()) {
        result.detail = QStringLiteral("三思回包无法拆数");
        return result;
    }
    bool forceOk = false;
    result.force = values.at(0).toDouble(&forceOk);
    if (!forceOk) {
        result.detail = QStringLiteral("三思力字段无效");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = values.size() >= 2;
    if (result.hasTemp)
        result.temp = values.at(1).toDouble();
    result.detail = QStringLiteral("三思 ASCII");
    return result;
}

// 解析科新 FixedDoubleToHex 仅力回包
inline ProtocolResult parseKexin(const QByteArray& raw)
{
    ProtocolResult result;
    const int hash = raw.indexOf('#');
    const QByteArray body = hash >= 0 ? raw.left(hash) : raw;
    const QRegularExpression expression(QStringLiteral("[-+]?[0-9]+\\.[0-9]+"));
    const QRegularExpressionMatch match = expression.match(QString::fromLatin1(body));
    if (!match.hasMatch()) {
        result.detail = QStringLiteral("科新回包无有效力值");
        return result;
    }
    result.force = match.captured(0).toDouble();
    result.ok = true;
    result.hasValues = true;
    result.detail = QStringLiteral("科新仅力");
    return result;
}

// 解析冠腾 R{力}#N{温}# 回包
inline ProtocolResult parseGuanteng(const QByteArray& raw)
{
    ProtocolResult result;
    const QRegularExpression expression(
        QStringLiteral("R([+-]?[0-9.]+)#N([+-]?[0-9.]+)#"));
    const QRegularExpressionMatch match = expression.match(QString::fromLatin1(raw));
    if (!match.hasMatch()) {
        result.detail = QStringLiteral("冠腾帧格式不匹配");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = true;
    result.force = match.captured(1).toDouble();
    result.temp = match.captured(2).toDouble();
    result.detail = QStringLiteral("冠腾 R#N#");
    return result;
}

// 按串口协议分派软件回包解析
inline ProtocolResult parseReply(int proto, const QByteArray& raw)
{
    if (raw.isEmpty())
        return ProtocolResult{false, false, false, false, 0.0, 0.0, QStringLiteral("空包")};
    if (proto == 0)
        return parseSansi(raw);
    if (proto == 1)
        return parseKexin(raw);
    if (proto == 3)
        return ProtocolResult{raw.size() == 56, false, false, false, 0.0, 0.0,
                              QStringLiteral("IEEE 56 字节 ASC 回包")};
    if (proto == 4)
        return parseGuanteng(raw);
    return ProtocolResult{true, true, false, false, 0.0, 0.0,
                          QStringLiteral("时代新材库无业务出站")};
}

} // namespace SerialProtocol
