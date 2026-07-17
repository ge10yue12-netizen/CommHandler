// DemoComm.h — MachinePeer：按库入站格式组指令/测数；无能力时返回空并写 fail
#pragma once

#include "ProtoCapability.h"

#include <QByteArray>
#include <QHostAddress>
#include <QIODevice>
#include <QSerialPort>
#include <QString>
#include <QUdpSocket>
#include <QtGlobal>
#include <cstring>

#pragma pack(push, 1)
// 与库 SocketProtol::ControlMessage / ForceData 同布局
namespace ZhongjiWire {
enum MsgType : quint8 { CONTROL_MESSAGE = 0x01, DATA_MESSAGE = 0x02 };
enum CtrlCmd : quint8 { START_CALC = 0x0A, STOP_CALC = 0x0B };
struct ControlMessage {
    // 消息类型：CONTROL_MESSAGE=0x01
    quint8 msgType;
    // 控制命令字：START_CALC/STOP_CALC 等
    quint8 command;
};
struct ForceData {
    // 消息类型：DATA_MESSAGE=0x02
    quint8 msgType;
    // 四通道力值 x1,y1,x2,y2
    double forceValues[4];
};
} // namespace ZhongjiWire

// 与库 struct Data 同布局（三思网口）
struct SansiNetData {
    // 力
    double dForce;
    // 位移
    double dMove;
    // 应变
    double dStrain;
};
#pragma pack(pop)

// 界面波特率下拉索引 → QSerialPort::BaudRate
inline QSerialPort::BaudRate baudFromIndex(int baudIdx)
{
    static const QSerialPort::BaudRate kBaud[] = {
        QSerialPort::Baud4800,   QSerialPort::Baud9600,  QSerialPort::Baud19200,
        QSerialPort::Baud38400,  QSerialPort::Baud57600, QSerialPort::Baud115200,
    };
    if (baudIdx < 0 || baudIdx > 5)
        return QSerialPort::Baud115200;
    return kBaud[baudIdx];
}

// 打开试验机串口；失败时写入 fail 并返回 false
inline bool openPeerSerial(QSerialPort* port, int comPort, int baudIdx, int dataBits, int parity, int stopBits,
                           QString* fail)
{
    if (!port) {
        if (fail)
            *fail = QStringLiteral("串口无效");
        return false;
    }
    if (port->isOpen())
        port->close();
    port->setPortName(QStringLiteral("COM%1").arg(comPort + 1));
    port->setBaudRate(baudFromIndex(baudIdx));
    QSerialPort::DataBits db = QSerialPort::Data8;
    if (dataBits == 0)
        db = QSerialPort::Data5;
    else if (dataBits == 1)
        db = QSerialPort::Data6;
    port->setDataBits(db);
    QSerialPort::Parity py = QSerialPort::NoParity;
    if (parity == 1)
        py = QSerialPort::EvenParity;
    else if (parity == 2)
        py = QSerialPort::OddParity;
    else if (parity == 3)
        py = QSerialPort::SpaceParity;
    else if (parity == 4)
        py = QSerialPort::MarkParity;
    port->setParity(py);
    QSerialPort::StopBits sb = QSerialPort::OneStop;
    if (stopBits == 1)
        sb = QSerialPort::OneAndHalfStop;
    else if (stopBits == 2)
        sb = QSerialPort::TwoStop;
    port->setStopBits(sb);
    port->setFlowControl(QSerialPort::NoFlowControl);
    if (!port->open(QIODevice::ReadWrite)) {
        if (fail)
            *fail = port->errorString();
        return false;
    }
    return true;
}

// 关闭试验机串口
inline void closePeerSerial(QSerialPort* port)
{
    if (port && port->isOpen())
        port->close();
}

// 绑定本机 UDP，用于向软件发指令/测数并接收回传
inline bool openPeerUdp(QUdpSocket* sock, const QString& localIp, int localPort, QString* fail)
{
    if (!sock) {
        if (fail)
            *fail = QStringLiteral("UDP无效");
        return false;
    }
    sock->close();
    if (!sock->bind(QHostAddress(localIp), static_cast<quint16>(localPort),
                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        if (fail)
            *fail = sock->errorString();
        return false;
    }
    return true;
}

// 关闭 UDP
inline void closePeerUdp(QUdpSocket* sock)
{
    if (sock)
        sock->close();
}

// 串口写出；成功时 hexOut 为大写 HEX 文本；失败返回 false
inline bool writeSerial(QSerialPort* port, const QByteArray& data, QString* hexOut)
{
    if (!port || !port->isOpen() || data.isEmpty())
        return false;
    if (port->write(data) != data.size())
        return false;
    port->flush();
    if (hexOut)
        *hexOut = QString::fromLatin1(data.toHex().toUpper());
    return true;
}

// UDP 数据报写出到软件地址；长度不一致则返回 false
inline bool writeUdp(QUdpSocket* sock, const QString& destIp, int destPort, const QByteArray& data)
{
    if (!sock || data.isEmpty())
        return false;
    return sock->writeDatagram(data, QHostAddress(destIp), static_cast<quint16>(destPort)) == data.size();
}

// 串口指令：仅组库能识别的帧；否则 fail
inline QByteArray packSerialCmd(int proto, int cmd, QString* fail)
{
    if (!ProtoCapability::serialCanReceiveCmd(proto, cmd)) {
        if (fail)
            *fail = ProtoCapability::serialCmdDenyReason(proto, cmd);
        return {};
    }
    if (proto == 0) {
        switch (cmd) {
        case 0:
            return QByteArray("\x3D\x0D", 2);
        case 1:
            return QByteArray("\x3E\x0D", 2);
        case 2:
            return QByteArray("\x6B\x0D", 2);
        case 3:
            return QByteArray("\x5A\x0D", 2);
        default:
            break;
        }
    }
    if (proto == 3) {
        if (cmd == 0)
            return QByteArray("\x24\x00\x0D\x0A", 4);
        if (cmd == 1)
            return QByteArray("\x24\xFF\x0D\x0A", 4);
    }
    if (fail)
        *fail = QStringLiteral("未知串口指令");
    return {};
}

// 串口测数：按库收侧格式组帧；库无入站能力时写 fail 并返回空
inline QByteArray packSerialMeasure(int proto, double force, double temp, QString* fail)
{
    Q_UNUSED(temp);
    if (!ProtoCapability::serialCanReceiveMeasure(proto)) {
        if (fail)
            *fail = ProtoCapability::serialMeasureDenyReason(proto);
        return {};
    }
    switch (proto) {
    case 0: {
        // 三思入站仅 E2+力；界面「温」不进入本帧（库亦不解析温）
        const qint64 raw = qRound64(force * 1000000.0);
        const quint32 u = static_cast<quint32>(qBound(0LL, raw, static_cast<qint64>(0xFFFFFFFFu)));
        QByteArray frame;
        frame.append(char(0xE2));
        frame.append(char((u >> 24) & 0xFF));
        frame.append(char((u >> 16) & 0xFF));
        frame.append(char((u >> 8) & 0xFF));
        frame.append(char(u & 0xFF));
        return frame;
    }
    case 1: {
        // 科新：库将收字节 toHex 后找 "024C"，再取 18 个 HEX 字符（9 字节 ASCII）经 HexToFloat
        // 正确载荷示例：02 4C + "0001.0000"（力=1.0）
        QString body;
        if (force < 0.0)
            body = QLatin1Char('-') + QStringLiteral("%1").arg(-force, 8, 'f', 4, QLatin1Char('0'));
        else
            body = QStringLiteral("%1").arg(force, 9, 'f', 4, QLatin1Char('0'));
        if (body.size() > 9)
            body = body.right(9);
        while (body.size() < 9)
            body.prepend(QLatin1Char('0'));
        QByteArray frame;
        frame.append(char(0x02));
        frame.append(char(0x4C));
        frame.append(body.toLatin1());
        return frame;
    }
    case 2: // 时代新材：value,num,CONSTANT|RUN,flag\r\n
        return QStringLiteral("%1,0,RUN,0\r\n").arg(force, 0, 'f', 3).toLatin1();
    default:
        if (fail)
            *fail = QStringLiteral("未知串口协议");
        return {};
    }
}

// 网口指令：按库 Parsing* 入站格式
inline QByteArray packNetCmd(int proto, int cmd, QString* fail)
{
    if (!ProtoCapability::netCanReceiveCmd(proto, cmd)) {
        if (fail)
            *fail = ProtoCapability::netCmdDenyReason(proto, cmd);
        return {};
    }
    switch (proto) {
    case 0:
        if (cmd == 0)
            return QByteArray("{\"tn\":1}");
        if (cmd == 1)
            return QByteArray("{\"tn\":3}");
        break;
    case 1:
        if (cmd == 0)
            return QByteArray("START");
        if (cmd == 1)
            return QByteArray("STOP");
        break;
    case 2: {
        ZhongjiWire::ControlMessage cm{};
        cm.msgType = ZhongjiWire::CONTROL_MESSAGE;
        cm.command = (cmd == 0) ? ZhongjiWire::START_CALC : ZhongjiWire::STOP_CALC;
        return QByteArray(reinterpret_cast<const char*>(&cm), static_cast<int>(sizeof(cm)));
    }
    case 4:
        if (cmd == 2)
            return QByteArray("SAVE\r\n");
        break;
    case 6:
        if (cmd == 0)
            return QByteArray("{\"name\":\"alphaStartTest\"}");
        if (cmd == 1)
            return QByteArray("{\"name\":\"alphaStopTest\"}");
        break;
    case 7:
        if (cmd == 0)
            return QByteArray("calcStart");
        if (cmd == 1)
            return QByteArray("calcEnd");
        break;
    default:
        break;
    }
    if (fail)
        *fail = QStringLiteral("未知网口指令");
    return {};
}

// 网口测数：仅组库能 emitNewData 的帧；否则写 fail
inline QByteArray packNetMeasure(int proto, double force, double temp, QString* fail)
{
    if (!ProtoCapability::netCanReceiveMeasure(proto)) {
        if (fail)
            *fail = ProtoCapability::netMeasureDenyReason(proto);
        return {};
    }
    switch (proto) {
    case 2: {
        ZhongjiWire::ForceData fd{};
        fd.msgType = ZhongjiWire::DATA_MESSAGE;
        fd.forceValues[0] = force;
        fd.forceValues[1] = temp;
        fd.forceValues[2] = 0.0;
        fd.forceValues[3] = 0.0;
        return QByteArray(reinterpret_cast<const char*>(&fd), static_cast<int>(sizeof(fd)));
    }
    case 3: {
        SansiNetData d{};
        d.dForce = force;
        d.dMove = 0.0;
        d.dStrain = 0.0;
        return QByteArray(reinterpret_cast<const char*>(&d), static_cast<int>(sizeof(d)));
    }
    case 5:
        return QStringLiteral("TL%1E0D0M0").arg(force, 0, 'f', 2).toLatin1();
    default:
        if (fail)
            *fail = QStringLiteral("未知网口测数协议");
        return {};
    }
}
