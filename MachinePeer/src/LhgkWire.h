// LhgkWire.h — 联恒光科线帧工具，对齐 CommHandler SerLhgk / NetLhgk
#pragma once

#include "ProtocolResult.h"

#include <QByteArray>
#include <QtGlobal>
#include <cstring>
#include <vector>

namespace LhgkWire {

// CRC16-XMODEM（poly 0x1021，初值 0），与 DLL 一致
inline quint16 crc16Xmodem(const quint8* data, size_t length)
{
    quint16 crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<quint16>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? static_cast<quint16>((crc << 1) ^ 0x1021)
                                 : static_cast<quint16>(crc << 1);
    }
    return crc;
}

inline void appendDoubleLE(QByteArray* out, double value)
{
    quint64 raw = 0;
    std::memcpy(&raw, &value, sizeof(value));
    for (int i = 0; i < 8; ++i)
        out->append(static_cast<char>((raw >> (i * 8)) & 0xFF));
}

inline void appendDoubleBE(QByteArray* out, double value)
{
    quint64 raw = 0;
    std::memcpy(&raw, &value, sizeof(value));
    for (int i = 7; i >= 0; --i)
        out->append(static_cast<char>((raw >> (i * 8)) & 0xFF));
}

inline bool readDoubleLE(const QByteArray& payload, int offset, double* out)
{
    if (!out || offset < 0 || offset + 8 > payload.size())
        return false;
    quint64 raw = 0;
    for (int i = 0; i < 8; ++i)
        raw |= static_cast<quint64>(static_cast<quint8>(payload.at(offset + i))) << (i * 8);
    std::memcpy(out, &raw, sizeof(double));
    return true;
}

inline bool readDoubleBE(const QByteArray& payload, int offset, double* out)
{
    if (!out || offset < 0 || offset + 8 > payload.size())
        return false;
    quint64 raw = 0;
    for (int i = 0; i < 8; ++i)
        raw |= static_cast<quint64>(static_cast<quint8>(payload.at(offset + i))) << ((7 - i) * 8);
    std::memcpy(out, &raw, sizeof(double));
    return true;
}

// 串口 v1.1：AA55 | type | lenLE | payload | crcLE | 0D0A
inline QByteArray buildSerialFrame(quint8 type, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(char(0xAA));
    frame.append(char(0x55));
    frame.append(char(type));
    const quint16 len = static_cast<quint16>(payload.size());
    frame.append(char(len & 0xFF));
    frame.append(char((len >> 8) & 0xFF));
    frame.append(payload);
    const quint16 crc =
        crc16Xmodem(reinterpret_cast<const quint8*>(frame.constData()), static_cast<size_t>(frame.size()));
    frame.append(char(crc & 0xFF));
    frame.append(char((crc >> 8) & 0xFF));
    frame.append(char(0x0D));
    frame.append(char(0x0A));
    return frame;
}

// 网口：AA55 | type | lenBE | payload | crcBE | 0D0A（普通帧 length=payload 字节数）
inline QByteArray buildNetFrame(quint8 type, const QByteArray& payload)
{
    QByteArray frame;
    frame.append(char(0xAA));
    frame.append(char(0x55));
    frame.append(char(type));
    const quint16 len = static_cast<quint16>(payload.size());
    frame.append(char((len >> 8) & 0xFF));
    frame.append(char(len & 0xFF));
    frame.append(payload);
    const quint16 crc =
        crc16Xmodem(reinterpret_cast<const quint8*>(frame.constData()), static_cast<size_t>(frame.size()));
    frame.append(char((crc >> 8) & 0xFF));
    frame.append(char(crc & 0xFF));
    frame.append(char(0x0D));
    frame.append(char(0x0A));
    return frame;
}

// 控制：type=0x02，payload=0x01 开始 / 0x02 停止（串口/网口同语义）
inline QByteArray buildSerialControl(int command)
{
    const quint8 cmd = (command == 0) ? 0x01 : 0x02;
    return buildSerialFrame(0x02, QByteArray(1, char(cmd)));
}

inline QByteArray buildNetControl(int command)
{
    const quint8 cmd = (command == 0) ? 0x01 : 0x02;
    return buildNetFrame(0x02, QByteArray(1, char(cmd)));
}

// 串口测数：与 SerLhgkProtocol::buildExternalDataFrame 一致（TX=RX）
inline QByteArray buildSerialMeasurement(double force, double temp)
{
    QByteArray payload;
    payload.append(char(0x06));
    payload.append(char(0x0C));
    appendDoubleLE(&payload, force);
    payload.append(char(0x06));
    payload.append(char(0x0D));
    appendDoubleLE(&payload, temp);
    return buildSerialFrame(0x01, payload);
}

// 网口测数（机→库）：RX 布局 — length=段数 2，力 0x0C / 温 0x0D，BE，无段计数字节
inline QByteArray buildNetMeasurementToDll(double force, double temp)
{
    QByteArray payload;
    payload.append(char(0x06));
    payload.append(char(0x0C));
    appendDoubleBE(&payload, force);
    payload.append(char(0x06));
    payload.append(char(0x0D));
    appendDoubleBE(&payload, temp);
    // 手工组帧：length 字段写段数 2，而非 payload 字节数
    QByteArray frame;
    frame.append(char(0xAA));
    frame.append(char(0x55));
    frame.append(char(0x01));
    frame.append(char(0x00));
    frame.append(char(0x02));
    frame.append(payload);
    const quint16 crc =
        crc16Xmodem(reinterpret_cast<const quint8*>(frame.constData()), static_cast<size_t>(frame.size()));
    frame.append(char((crc >> 8) & 0xFF));
    frame.append(char(crc & 0xFF));
    frame.append(char(0x0D));
    frame.append(char(0x0A));
    return frame;
}

// 解析串口联恒完整帧中的力/温（type 0x01）
inline ProtocolResult parseSerialMeasurement(const QByteArray& raw)
{
    ProtocolResult result;
    if (raw.size() < 9 || static_cast<quint8>(raw.at(0)) != 0xAA
        || static_cast<quint8>(raw.at(1)) != 0x55) {
        result.detail = QStringLiteral("联恒串口帧头不匹配");
        return result;
    }
    if (static_cast<quint8>(raw.at(raw.size() - 2)) != 0x0D
        || static_cast<quint8>(raw.at(raw.size() - 1)) != 0x0A) {
        result.detail = QStringLiteral("联恒串口帧尾不匹配");
        return result;
    }
    const quint8 type = static_cast<quint8>(raw.at(2));
    const quint16 length =
        static_cast<quint16>(static_cast<quint8>(raw.at(3)) | (static_cast<quint8>(raw.at(4)) << 8));
    if (raw.size() != 9 + length) {
        result.detail = QStringLiteral("联恒串口帧长不匹配");
        return result;
    }
    const quint16 expectCrc =
        static_cast<quint16>(static_cast<quint8>(raw.at(raw.size() - 4))
                             | (static_cast<quint8>(raw.at(raw.size() - 3)) << 8));
    const quint16 actualCrc = crc16Xmodem(reinterpret_cast<const quint8*>(raw.constData()),
                                          static_cast<size_t>(raw.size() - 4));
    if (expectCrc != actualCrc) {
        result.detail = QStringLiteral("联恒串口 CRC 失败");
        return result;
    }
    if (type != 0x01) {
        result.ok = true;
        result.isAck = (type == 0x02);
        if (type == 0x02 && length >= 1 && raw.size() >= 6) {
            const quint8 cmd = static_cast<quint8>(raw.at(5));
            result.detail = (cmd == 0x01) ? QStringLiteral("联恒串口控制开始")
                                          : (cmd == 0x02) ? QStringLiteral("联恒串口控制停止")
                                                          : QStringLiteral("联恒串口控制");
        } else {
            result.detail = QStringLiteral("联恒串口 type=0x%1").arg(type, 2, 16, QLatin1Char('0'));
        }
        return result;
    }
    const QByteArray payload = raw.mid(5, length);
    double force = 0.0;
    double temp = 0.0;
    bool gotForce = false;
    bool gotTemp = false;
    for (int i = 0; i + 10 <= payload.size(); i += 10) {
        const quint8 mod = static_cast<quint8>(payload.at(i));
        const quint8 tid = static_cast<quint8>(payload.at(i + 1));
        double value = 0.0;
        if (!readDoubleLE(payload, i + 2, &value) || mod != 0x06)
            continue;
        if (tid == 0x0C) {
            force = value;
            gotForce = true;
        } else if (tid == 0x0D) {
            temp = value;
            gotTemp = true;
        }
    }
    if (!gotForce) {
        result.detail = QStringLiteral("联恒串口缺少力段");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = gotTemp;
    result.force = force;
    result.temp = temp;
    result.detail = QStringLiteral("联恒串口测数");
    return result;
}

// 解析网口联恒完整帧：自动区分 RX（len=段数）与软件 TX（len=payload 字节）
inline ProtocolResult parseNetFrame(const QByteArray& raw)
{
    ProtocolResult result;
    if (raw.size() < 9 || static_cast<quint8>(raw.at(0)) != 0xAA
        || static_cast<quint8>(raw.at(1)) != 0x55) {
        result.detail = QStringLiteral("联恒网口帧头不匹配");
        return result;
    }
    if (static_cast<quint8>(raw.at(raw.size() - 2)) != 0x0D
        || static_cast<quint8>(raw.at(raw.size() - 1)) != 0x0A) {
        result.detail = QStringLiteral("联恒网口帧尾不匹配");
        return result;
    }
    const quint8 type = static_cast<quint8>(raw.at(2));
    const quint16 length =
        static_cast<quint16>((static_cast<quint8>(raw.at(3)) << 8) | static_cast<quint8>(raw.at(4)));
    const quint16 expectCrc =
        static_cast<quint16>((static_cast<quint8>(raw.at(raw.size() - 4)) << 8)
                             | static_cast<quint8>(raw.at(raw.size() - 3)));
    const quint16 actualCrc = crc16Xmodem(reinterpret_cast<const quint8*>(raw.constData()),
                                          static_cast<size_t>(raw.size() - 4));
    if (expectCrc != actualCrc) {
        result.detail = QStringLiteral("联恒网口 CRC 失败");
        return result;
    }
    if (type != 0x01) {
        result.ok = true;
        result.isAck = (type == 0x02);
        if (type == 0x02 && raw.size() >= 6) {
            const quint8 cmd = static_cast<quint8>(raw.at(5));
            result.detail = (cmd == 0x01) ? QStringLiteral("联恒网口控制开始")
                                          : (cmd == 0x02) ? QStringLiteral("联恒网口控制停止")
                                                          : QStringLiteral("联恒网口控制");
        } else {
            result.detail = QStringLiteral("联恒网口 type=0x%1").arg(type, 2, 16, QLatin1Char('0'));
        }
        return result;
    }

    // RX：length=段数，payload=length*10，力 0x0C / 温 0x0D
    const int rxSize = 9 + static_cast<int>(length) * 10;
    // TX：length=payload 字节数（含段计数字节），力 0x0B / 温 0x0C
    const int txSize = 9 + static_cast<int>(length);
    const bool isRx = (raw.size() == rxSize);
    const bool isTx = (raw.size() == txSize);
    if (!isRx && !isTx) {
        result.detail = QStringLiteral("联恒网口测数帧长不匹配");
        return result;
    }

    const int payloadLen = raw.size() - 9;
    const QByteArray payload = raw.mid(5, payloadLen);
    int index = 0;
    quint8 forceId = 0x0C;
    quint8 tempId = 0x0D;
    if (isTx) {
        forceId = 0x0B;
        tempId = 0x0C;
        if (!payload.isEmpty() && static_cast<quint8>(payload.at(0)) <= 8)
            index = 1;
    }
    double force = 0.0;
    double temp = 0.0;
    bool gotForce = false;
    bool gotTemp = false;
    while (index + 10 <= payload.size()) {
        const quint8 mod = static_cast<quint8>(payload.at(index));
        const quint8 tid = static_cast<quint8>(payload.at(index + 1));
        double value = 0.0;
        if (!readDoubleBE(payload, index + 2, &value) || mod != 0x06) {
            index += 10;
            continue;
        }
        if (tid == forceId) {
            force = value;
            gotForce = true;
        } else if (tid == tempId) {
            temp = value;
            gotTemp = true;
        }
        index += 10;
    }
    if (!gotForce) {
        result.detail = QStringLiteral("联恒网口缺少力段");
        return result;
    }
    result.ok = true;
    result.hasValues = true;
    result.hasTemp = gotTemp;
    result.force = force;
    result.temp = temp;
    result.detail = isRx ? QStringLiteral("联恒网口 RX（机→库）") : QStringLiteral("联恒网口软件 TX");
    return result;
}

// 兼容旧名
inline ProtocolResult parseNetDllTxMeasurement(const QByteArray& raw)
{
    return parseNetFrame(raw);
}

} // namespace LhgkWire
