// CommIntegrationTests.cpp — 使用真实 CommHandler.dll 和虚拟串口验证 PT0/PT1 收发
#include "CommHandler.h"
#include "NetworkProtocol.h"
#include "SerialProtocol.h"
#include "UIDef.h"
#include "../../MachinePeer/src/NetworkProtocol.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QSerialPort>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QUdpSocket>
#include <QVector>
#include <cmath>
#include <cstring>
#include <exception>
#include <vector>

namespace {

int g_failures = 0;

// 记录集成断言结果
void check(bool condition, const QString& name)
{
    QTextStream(stdout) << (condition ? "PASS " : "FAIL ") << name << Qt::endl;
    if (!condition)
        ++g_failures;
}

// 在事件循环中等待条件满足
template <typename Predicate>
bool waitUntil(Predicate predicate, int timeoutMs = 1500)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    return predicate();
}

// 打开试验机侧虚拟串口
bool openPeer(QSerialPort* peer, const QString& portName)
{
    peer->setPortName(portName);
    peer->setBaudRate(QSerialPort::Baud115200);
    peer->setDataBits(QSerialPort::Data8);
    peer->setParity(QSerialPort::NoParity);
    peer->setStopBits(QSerialPort::OneStop);
    peer->setFlowControl(QSerialPort::NoFlowControl);
    return peer->open(QIODevice::ReadWrite);
}

// 等待并读取试验机侧全部回包
QByteArray receivePeerBytes(QSerialPort* peer, int minimumSize)
{
    QByteArray bytes;
    waitUntil([&]() {
        bytes += peer->readAll();
        return bytes.size() >= minimumSize;
    });
    bytes += peer->readAll();
    return bytes;
}

// 配置并连接 CommHandler 串口协议
bool connectProtocol(CommHandler* comm, int protocol)
{
    SerialProtocol::configure(comm, 3, 5, 3, 0, 0, protocol);
    return comm->Connect();
}

// 验证三思 PT0 测数入站和业务回发
void testSerialPt0(CommHandler* comm, QSerialPort* peer, QVector<double>* received)
{
    received->clear();
    check(connectProtocol(comm, 0), QStringLiteral("ABI PT0 连接"));
    peer->write(QByteArray::fromHex("E2000F4240"));
    peer->flush();
    check(waitUntil([&]() { return !received->isEmpty(); }),
          QStringLiteral("ABI PT0 测数入站"));
    check(!received->isEmpty() && std::fabs(received->at(0) - 1.0) < 1e-8,
          QStringLiteral("ABI PT0 力值"));

    bool sendOk = true;
    try {
        comm->SendData(std::vector<double>{1.0}, SERIAL);
    } catch (const std::exception&) {
        sendOk = false;
    }
    check(sendOk, QStringLiteral("ABI PT0 SendData"));
    const QByteArray reply = receivePeerBytes(peer, 9);
    check(reply.size() == 9 && reply.endsWith('\r'), QStringLiteral("ABI PT0 回包"));
    comm->Disconnect(0);
}

// 验证科新 PT1 测数入站和 14 字节业务回发
void testSerialPt1(CommHandler* comm, QSerialPort* peer, QVector<double>* received)
{
    received->clear();
    check(connectProtocol(comm, 1), QStringLiteral("ABI PT1 连接"));
    peer->write(QByteArray::fromHex("024C303030312E30303030"));
    peer->flush();
    check(waitUntil([&]() { return !received->isEmpty(); }),
          QStringLiteral("ABI PT1 测数入站"));
    check(!received->isEmpty() && std::fabs(received->at(0) - 1.0) < 1e-8,
          QStringLiteral("ABI PT1 力值"));

    bool sendOk = true;
    try {
        comm->SendData(std::vector<double>{1.0}, SERIAL);
    } catch (const std::exception&) {
        sendOk = false;
    }
    check(sendOk, QStringLiteral("ABI PT1 SendData"));
    const QByteArray reply = receivePeerBytes(peer, 14);
    check(reply.size() == 14 && reply.at(7) == '.' && reply.at(13) == '#',
          QStringLiteral("ABI PT1 14B 回包"));
    comm->Disconnect(0);
}

// 验证时代新材 PT2 测数入站
void testSerialPt2(CommHandler* comm, QSerialPort* peer, QVector<double>* received)
{
    received->clear();
    check(connectProtocol(comm, 2), QStringLiteral("ABI PT2 连接"));
    peer->write(QByteArray("1.000,0,RUN,0\r\n"));
    peer->flush();
    check(waitUntil([&]() { return !received->isEmpty(); }),
          QStringLiteral("ABI PT2 测数入站"));
    comm->Disconnect(0);
}

// 验证 IEEE PT3 控制入站和固定帧出站
void testSerialPt3(CommHandler* comm, QSerialPort* peer)
{
    bool eventReceived = false;
    const QMetaObject::Connection connection =
        QObject::connect(comm, &CommHandler::emitEventMsg, peer,
                         [&](int, int, int) { eventReceived = true; });
    check(connectProtocol(comm, 3), QStringLiteral("ABI PT3 连接"));
    peer->write(QByteArray::fromHex("24000D0A"));
    peer->flush();
    check(waitUntil([&]() { return eventReceived; }),
          QStringLiteral("ABI PT3 指令入站"));
    comm->SendData(std::vector<double>{1.0, 0.0, 0.0, 0.0, 0.0}, SERIAL);
    check(receivePeerBytes(peer, 56).size() == 56,
          QStringLiteral("ABI PT3 56B 回包"));
    comm->Disconnect(0);
    QObject::disconnect(connection);
}

// 验证冠腾 PT4 主动业务出站格式
void testSerialPt4(CommHandler* comm, QSerialPort* peer)
{
    check(connectProtocol(comm, 4), QStringLiteral("ABI PT4 连接"));
    comm->SendData(std::vector<double>{1.0, 25.0}, SERIAL);
    const QByteArray reply = receivePeerBytes(peer, 24);
    check(reply.startsWith('R') && reply.contains("#N") && reply.endsWith('#'),
          QStringLiteral("ABI PT4 回包"));
    comm->Disconnect(0);
}

// 等待并读取一个 UDP 数据报
QByteArray receiveDatagram(QUdpSocket* socket)
{
    waitUntil([&]() { return socket->hasPendingDatagrams(); });
    if (!socket->hasPendingDatagrams())
        return {};
    QByteArray datagram;
    datagram.resize(static_cast<int>(socket->pendingDatagramSize()));
    socket->readDatagram(datagram.data(), datagram.size());
    return datagram;
}

// 经试验机 UDP 向软件端发送一个完整协议包
bool sendDatagram(QUdpSocket* socket, int destinationPort, const QByteArray& datagram)
{
    return socket->writeDatagram(datagram, QHostAddress::LocalHost,
                                 static_cast<quint16>(destinationPort))
        == datagram.size();
}

// 验证一个网口协议的真实 DLL 入站路径
void testNetworkProtocol(int protocol)
{
    const int softwarePort = 21000 + protocol * 2;
    const int peerPort = softwarePort + 1;
    QUdpSocket peer;
    check(peer.bind(QHostAddress::LocalHost, static_cast<quint16>(peerPort)),
          QStringLiteral("ABI NET PT%1 PeerBind").arg(protocol));

    CommHandler comm;
    bool eventReceived = false;
    QVector<double> received;
    QObject::connect(&comm, &CommHandler::emitEventMsg, &peer,
                     [&](int, int, int) { eventReceived = true; });
    QObject::connect(&comm, &CommHandler::emitNewData, &peer,
                     [&](void* data, int size, int) {
                         if (!data || size <= 0 || size > 16)
                             return;
                         received.resize(size);
                         std::memcpy(received.data(), data,
                                     static_cast<size_t>(size) * sizeof(double));
                     },
                     Qt::DirectConnection);

    NetworkProtocol::configure(&comm, 1, 0, QStringLiteral("127.0.0.1"), softwarePort,
                               QStringLiteral("127.0.0.1"), peerPort, protocol);
    check(comm.Connect(), QStringLiteral("ABI NET PT%1 连接").arg(protocol));
    waitUntil([]() { return false; }, 100);

    QString error;
    if (NetworkProtocol::canSendCommand(protocol, 0)) {
        const QByteArray command = NetworkProtocol::packCommand(protocol, 0, &error);
        check(sendDatagram(&peer, softwarePort, command),
              QStringLiteral("ABI NET PT%1 指令发送").arg(protocol));
        check(waitUntil([&]() { return eventReceived; }),
              QStringLiteral("ABI NET PT%1 指令入站").arg(protocol));
        if (protocol == 0 || protocol == 2)
            check(!receiveDatagram(&peer).isEmpty(),
                  QStringLiteral("ABI NET PT%1 ACK").arg(protocol));
    } else if (protocol == 4) {
        const QByteArray command = NetworkProtocol::packCommand(protocol, 2, &error);
        check(sendDatagram(&peer, softwarePort, command),
              QStringLiteral("ABI NET PT4 存图发送"));
        check(waitUntil([&]() { return eventReceived; }),
              QStringLiteral("ABI NET PT4 存图入站"));
    }

    if (NetworkProtocol::canSendMeasurement(protocol)) {
        const QByteArray measurement =
            NetworkProtocol::packMeasurement(protocol, 1.0, 25.0, &error);
        check(sendDatagram(&peer, softwarePort, measurement),
              QStringLiteral("ABI NET PT%1 测数发送").arg(protocol));
        check(waitUntil([&]() { return !received.isEmpty(); }),
              QStringLiteral("ABI NET PT%1 测数入站").arg(protocol));
    }
    comm.Disconnect(0);
}

} // namespace

// 运行真实 DLL 与 COM4/COM5 的串口集成测试
int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    const int networkArgument = arguments.indexOf(QStringLiteral("--network"));
    if (networkArgument >= 0 && networkArgument + 1 < arguments.size()) {
        testNetworkProtocol(arguments.at(networkArgument + 1).toInt());
        QTextStream(stdout) << "TOTAL_FAILURES " << g_failures << Qt::endl;
        return g_failures == 0 ? 0 : 1;
    }

    QSerialPort peer;
    if (!openPeer(&peer, QStringLiteral("COM5"))) {
        QTextStream(stderr) << "SKIP COM5 " << peer.errorString() << Qt::endl;
        return 2;
    }

    CommHandler comm;
    QVector<double> received;
    QObject::connect(&comm, &CommHandler::emitNewData, &application,
                     [&](void* data, int size, int) {
                         if (!data || size <= 0 || size > 16)
                             return;
                         received.resize(size);
                         std::memcpy(received.data(), data,
                                     static_cast<size_t>(size) * sizeof(double));
                     },
                     Qt::DirectConnection);

    testSerialPt0(&comm, &peer, &received);
    testSerialPt1(&comm, &peer, &received);
    testSerialPt2(&comm, &peer, &received);
    testSerialPt3(&comm, &peer);
    testSerialPt4(&comm, &peer);
    QTextStream(stdout) << "TOTAL_FAILURES " << g_failures << Qt::endl;
    return g_failures == 0 ? 0 : 1;
}
