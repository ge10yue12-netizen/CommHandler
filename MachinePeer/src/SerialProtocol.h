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
#include <array>
#include <cstring>

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
    // dataBits：0=5 1=6 2=7 3=8（与 UIDef / CommHandler 一致）
    port->setDataBits(dataBits == 0 ? QSerialPort::Data5
                                   : dataBits == 1 ? QSerialPort::Data6
                                                   : dataBits == 2 ? QSerialPort::Data7
                                                                   : QSerialPort::Data8);
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

// 对齐 SerialPortComm::HexToFloat：将 18 个 HEX 字符解码为 ASCII 数字串
inline QString hexToFloatAscii(const QByteArray& hexLatin1)
{
    QByteArray ascii;
    for (int i = 0; i + 1 < hexLatin1.size(); i += 2) {
        const QByteArray pair = hexLatin1.mid(i, 2);
        if (pair == "2D")
            ascii.append('-');
        else if (pair == "2E")
            ascii.append('.');
        else if (pair == "30")
            ascii.append('0');
        else if (pair == "31")
            ascii.append('1');
        else if (pair == "32")
            ascii.append('2');
        else if (pair == "33")
            ascii.append('3');
        else if (pair == "34")
            ascii.append('4');
        else if (pair == "35")
            ascii.append('5');
        else if (pair == "36")
            ascii.append('6');
        else if (pair == "37")
            ascii.append('7');
        else if (pair == "38")
            ascii.append('8');
        else if (pair == "39")
            ascii.append('9');
    }
    return QString::fromLatin1(ascii);
}

// 对齐 SerialPortComm::MsgDataInput PT1：组 9 字节 ASCII 力值体（HexToFloat 可解码）
inline QByteArray encodeKexinMeasurementBody(double force)
{
    const QString numeric = QString::number(qAbs(force), 'f', 4);
    const int capacity = force < 0.0 ? 8 : 9;
    if (!qIsFinite(force) || numeric.size() > capacity)
        return {};
    QString body;
    if (force < 0.0) {
        const int pad = 8 - numeric.size();
        const QString digits = QString(pad, QLatin1Char('0')) + numeric;
        body = QLatin1Char('-') + digits;
    } else {
        const int pad = 9 - numeric.size();
        body = QString(pad, QLatin1Char('0')) + numeric;
    }
    return body.toLatin1();
}

// 对齐 SerialPortComm::FixedDoubleToHex（SendData PT1 case 1）：解析 14 字节回包
inline bool decodeFixedDoubleToHexReply(const QByteArray& raw, double* forceOut, QString* detail)
{
    if (raw.size() < 14) {
        if (detail)
            *detail = QStringLiteral("科新回包不足 14 字节（当前 %1）").arg(raw.size());
        return false;
    }
    const QByteArray frame = raw.left(14);
    if (static_cast<unsigned char>(frame.at(13)) != 0x23) {
        if (detail)
            *detail = QStringLiteral("科新回包第 14 字节应为 '#' (0x23)");
        return false;
    }
    if (frame.at(7) != char(0x2E)) {
        if (detail)
            *detail = QStringLiteral("科新回包第 8 字节应为 '.' (0x2E)");
        return false;
    }
    const QByteArray numField = frame.mid(2, 11);
    for (char ch : numField) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte != 0x2D && byte != 0x2E && (byte < 0x30 || byte > 0x39)) {
            if (detail)
                *detail = QStringLiteral("科新回包含非法 ASCII 0x%1").arg(byte, 2, 16, QLatin1Char('0'));
            return false;
        }
    }
    bool ok = false;
    const double value = QString::fromLatin1(numField).toDouble(&ok);
    if (!ok) {
        if (detail)
            *detail = QStringLiteral("科新回包力字段无法转 double：%1")
                          .arg(QString::fromLatin1(numField));
        return false;
    }
    if (forceOut)
        *forceOut = value;
    return true;
}

// 科新 PT1 软件回包粘包缓冲：按 14 字节定长切帧（对齐 SendData 写出的 byteArray）
inline QList<QByteArray> takeKexinReplyFrames(QByteArray* buffer)
{
    QList<QByteArray> frames;
    if (!buffer)
        return frames;
    while (buffer->size() >= 14) {
        frames.append(buffer->left(14));
        buffer->remove(0, 14);
    }
    return frames;
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
        const QByteArray payload = encodeKexinMeasurementBody(force);
        if (payload.size() != 9) {
            if (error)
                *error = QStringLiteral("科新力值超出 9 字节 ASCII 测数体范围");
            return {};
        }
        const QByteArray hexLatin1 = payload.toHex().toUpper();
        if (hexLatin1.size() != 18) {
            if (error)
                *error = QStringLiteral("科新测数 HEX 体应为 18 字符，当前 %1").arg(hexLatin1.size());
            return {};
        }
        bool ok = false;
        const double roundTrip = hexToFloatAscii(hexLatin1).toDouble(&ok);
        if (!ok || qAbs(roundTrip - force) > 1e-3) {
            if (error)
                *error = QStringLiteral("科新测数体未通过 HexToFloat 往返校验");
            return {};
        }
        const QByteArray frame = QByteArray("\x02\x4C", 2) + payload;
        if (frame.size() != 11) {
            if (error)
                *error = QStringLiteral("科新测数帧必须为 11 字节，当前 %1 字节").arg(frame.size());
            return {};
        }
        return frame;
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

// 解析科新 FixedDoubleToHex 仅力回包（14 字节定长，有效载荷在 [2,13)）
inline ProtocolResult parseKexin(const QByteArray& raw)
{
    ProtocolResult result;
    double force = 0.0;
    if (!decodeFixedDoubleToHexReply(raw, &force, &result.detail))
        return result;
    result.force = force;
    result.ok = true;
    result.hasValues = true;
    result.detail = QStringLiteral("科新 FixedDoubleToHex 14B");
    return result;
}

// 解析 IEEE 56 字节帧并校验头尾、HEX 字段及库定义校验和
inline ProtocolResult parseIeee(const QByteArray& raw)
{
    ProtocolResult result;
    if (raw.size() != 56 || static_cast<unsigned char>(raw.at(0)) != 0x24
        || static_cast<unsigned char>(raw.at(1)) != 0x01
        || static_cast<unsigned char>(raw.at(54)) != 0x0D
        || static_cast<unsigned char>(raw.at(55)) != 0x0A) {
        result.detail = QStringLiteral("IEEE 帧长或头尾不匹配");
        return result;
    }

    std::array<float, 6> values{};
    unsigned int byteSum = 0;
    for (int channel = 0; channel < 6; ++channel) {
        const QByteArray hex = raw.mid(2 + channel * 8, 8);
        bool validHex = hex.size() == 8;
        for (char ch : hex) {
            const bool digit = ch >= '0' && ch <= '9';
            const bool upper = ch >= 'A' && ch <= 'F';
            const bool lower = ch >= 'a' && ch <= 'f';
            validHex = validHex && (digit || upper || lower);
        }
        if (!validHex) {
            result.detail = QStringLiteral("IEEE 第 %1 路 HEX 字段无效").arg(channel + 1);
            return result;
        }
        const QByteArray networkBytes = QByteArray::fromHex(hex);
        unsigned char littleEndian[4] = {
            static_cast<unsigned char>(networkBytes.at(3)),
            static_cast<unsigned char>(networkBytes.at(2)),
            static_cast<unsigned char>(networkBytes.at(1)),
            static_cast<unsigned char>(networkBytes.at(0))};
        for (unsigned char byte : littleEndian)
            byteSum += byte;
        std::memcpy(&values[channel], littleEndian, sizeof(float));
    }

    const float checksumSource = static_cast<float>(byteSum);
    unsigned char checksumBytes[sizeof(float)]{};
    std::memcpy(checksumBytes, &checksumSource, sizeof(float));
    const QByteArray expectedChecksum =
        QByteArray(1, static_cast<char>(checksumBytes[1])).toHex().toUpper()
        + QByteArray(1, static_cast<char>(checksumBytes[0])).toHex().toUpper();
    if (raw.mid(50, 4).toUpper() != expectedChecksum) {
        result.detail = QStringLiteral("IEEE 校验和不匹配");
        return result;
    }

    result.ok = true;
    result.hasValues = true;
    result.hasTemp = true;
    result.force = values[0];
    result.temp = values[1];
    result.detail = QStringLiteral("IEEE 56B 六通道");
    return result;
}

// IEEE PT3 软件回包缓冲：按 24 01 帧头和 56 字节定长切帧
inline QList<QByteArray> takeIeeeReplyFrames(QByteArray* buffer)
{
    QList<QByteArray> frames;
    if (!buffer)
        return frames;
    const QByteArray header("\x24\x01", 2);
    while (!buffer->isEmpty()) {
        const int headerIndex = buffer->indexOf(header);
        if (headerIndex < 0) {
            const bool keepPrefix = buffer->endsWith(char(0x24));
            buffer->clear();
            if (keepPrefix)
                buffer->append(char(0x24));
            break;
        }
        if (headerIndex > 0)
            buffer->remove(0, headerIndex);
        if (buffer->size() < 56)
            break;
        const QByteArray frame = buffer->left(56);
        buffer->remove(0, 56);
        if (parseIeee(frame).ok)
            frames.append(frame);
    }
    return frames;
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
        return parseIeee(raw);
    if (proto == 4)
        return parseGuanteng(raw);
    return ProtocolResult{true, false, false, false, 0.0, 0.0,
                          QStringLiteral("时代新材库无业务出站")};
}

} // namespace SerialProtocol
