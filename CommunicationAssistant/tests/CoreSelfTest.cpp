// CoreSelfTest.cpp — 核心模块离线自检（无 UI、无真实设备依赖项可 SKIP）
#include "ClaimFor.h"
#include "HexUtil.h"
#include "NativeSession.h"
#include "SessionConfig.h"
#include "SessionManager.h"
#include "CaptureJson.h"
#include "CaptureManager.h"
#include "LegacyCapability.h"
#include "LegacySession.h"
#include "LegacyWatchdog.h"
#include "SendScheduler.h"
#include "Types.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QHostAddress>
#include <QSerialPortInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>
#include <cstdio>

static int g_failed = 0;

static void expect(bool cond, const char* name)
{
    if (cond) {
        std::printf("[PASS] %s\n", name);
    } else {
        std::printf("[FAIL] %s\n", name);
        ++g_failed;
    }
}

// 校验日志/UI 使用的英文枚举名稳定，防止本地化误改导致 Golden 漂移
static void testEnumNames()
{
    expect(ca::recordKindName(ca::RecordKind::ErrorEvent) == QStringLiteral("ErrorEvent"), "kind ErrorEvent");
    expect(ca::recordStatusName(ca::RecordStatus::Submitted) == QStringLiteral("Submitted"), "status Submitted");
    expect(ca::recordStatusName(ca::RecordStatus::Failed) == QStringLiteral("Failed"), "status Failed");
}

// 未 Connected 时 send 必须拒绝且不得占用 Serial claim
static void testSendRequiresConnected()
{
    ca::SessionManager mgr;
    ca::NativeSession session(&mgr);
    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.payload = QByteArray(1, '\x01');
    expect(!session.send(req).ok, "send rejected when not connected");
    expect(mgr.serialClaimCount() == 0, "send-reject leaves no claim");
}

static void testNormalize()
{
    expect(ca::normalizeSerialPortName(QStringLiteral("com3")) == QStringLiteral("COM3"), "normalize com3");
    expect(ca::normalizeSerialPortName(QStringLiteral("\\\\.\\COM10")) == QStringLiteral("COM10"), "normalize \\\\.\\COM10");
    expect(ca::normalizeSerialPortName(QStringLiteral("3")) == QStringLiteral("COM3"), "normalize bare 3");
    expect(ca::normalizeSerialPortName(QStringLiteral("  COM5  ")) == QStringLiteral("COM5"), "normalize trim");
}

static void testHex()
{
    QString err;
    expect(ca::parseHexPayloadStrict(QStringLiteral("01 02 0A"), &err) == QByteArray::fromHex("01020A"), "hex spaced");
    expect(ca::parseHexPayloadStrict(QStringLiteral("0G"), &err).isEmpty(), "hex reject 0G");
    expect(ca::parseHexPayloadStrict(QStringLiteral("ABC"), &err).isEmpty(), "hex reject odd");
    expect(ca::parseHexPayloadStrict(QStringLiteral("0x01"), &err).isEmpty(), "hex reject 0x prefix");
    expect(ca::parseHexPayloadStrict(QStringLiteral(""), &err).isEmpty(), "hex empty");
}

static void testClaimFor()
{
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::Serial;
    cfg.transport.serial.portName = QStringLiteral("com7");
    cfg.transport.serial.baudRate = 115200;

    ca::ResourceClaim claim;
    const ca::Result r = ca::claimFor(cfg, &claim);
    expect(r.ok, "claimFor ok");
    expect(claim.serialPort == QStringLiteral("COM7"), "claimFor COM7");
    expect(claim.exclusive, "claimFor exclusive");

    cfg.transport.serial.portName.clear();
    expect(!ca::claimFor(cfg, &claim).ok, "claimFor empty port fails");
}

static void testSessionManager()
{
    ca::SessionManager mgr;
    const QUuid a = QUuid::createUuid();
    const QUuid b = QUuid::createUuid();
    ca::ResourceClaim c1;
    c1.transport = ca::TransportKind::Serial;
    c1.serialPort = QStringLiteral("COM9");
    ca::ResourceClaim c2 = c1;

    expect(mgr.acquire(a, c1).ok, "acquire A COM9");
    expect(!mgr.acquire(b, c2).ok, "B blocked on COM9");
    mgr.release(a);
    expect(mgr.acquire(b, c2).ok, "B after A release");

    ca::ResourceClaim c3;
    c3.transport = ca::TransportKind::Serial;
    c3.serialPort = QStringLiteral("COM8");
    expect(mgr.acquire(b, c3).ok, "B switch COM9->COM8");
    expect(!mgr.isSerialClaimed(QStringLiteral("COM9")), "COM9 freed after switch");
    expect(mgr.isSerialClaimed(QStringLiteral("COM8")), "COM8 claimed");
    mgr.release(b);
    expect(mgr.serialClaimCount() == 0, "all released");
}

// FIX-002 回归：open 失败须单次 ErrorEvent、释放 claim、状态 Closed（不闪 Faulted）
static void testNativeOpenFail()
{
    ca::SessionManager mgr;
    ca::NativeSession session(&mgr);

    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("selftest");
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::Serial;
    cfg.transport.serial.portName = QStringLiteral("COM247");
    cfg.transport.serial.baudRate = 115200;
    cfg.transport.serial.dataBits = 8;
    cfg.transport.serial.parity = QStringLiteral("none");
    cfg.transport.serial.stopBits = QStringLiteral("1");

    int errorEvents = 0;
    QObject::connect(&session, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind == ca::RecordKind::ErrorEvent)
            ++errorEvents;
    });

    expect(session.configure(cfg).ok, "configure COM247");
    const ca::Result opened = session.open();
    expect(!opened.ok, "open nonexistent COM247 fails");
    expect(session.state() == ca::SessionState::Closed, "state Closed after open fail");
    expect(mgr.serialClaimCount() == 0, "claim released after open fail");
    expect(errorEvents == 1, "open fail emits single ErrorEvent");

    cfg.sessionId = QUuid::createUuid();
    expect(session.configure(cfg).ok, "reconfigure after fail");
    expect(!session.open().ok, "second open fail still clean");
    expect(mgr.serialClaimCount() == 0, "claim clean after second fail");
}

static bool hasPort(const QString& name)
{
    for (const QSerialPortInfo& p : QSerialPortInfo::availablePorts()) {
        if (p.portName().compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// [ASSUMPTION] 本机 Eltima 虚拟串口对 COM4<->COM5；缺失时 SKIP 不影响 CI
static void testVirtualLoopback()
{
    if (!hasPort(QStringLiteral("COM4")) || !hasPort(QStringLiteral("COM5"))) {
        std::printf("[SKIP] virtual loopback (COM4/COM5 not both present)\n");
        return;
    }

    ca::SessionManager mgr;
    ca::NativeSession tx(&mgr);
    ca::NativeSession rx(&mgr);

    QByteArray got;
    QObject::connect(&rx, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind == ca::RecordKind::RawChunk && rec.direction == ca::Direction::Rx)
            got.append(rec.bytes);
    });

    auto makeCfg = [](const QString& port) {
        ca::SessionConfig cfg;
        cfg.sessionId = QUuid::createUuid();
        cfg.name = QStringLiteral("loop");
        cfg.mode = ca::WorkMode::Native;
        cfg.role = ca::SessionRole::Tool;
        cfg.transport.kind = ca::TransportKind::Serial;
        cfg.transport.serial.portName = port;
        cfg.transport.serial.baudRate = 115200;
        cfg.transport.serial.dataBits = 8;
        cfg.transport.serial.parity = QStringLiteral("none");
        cfg.transport.serial.stopBits = QStringLiteral("1");
        return cfg;
    };

    expect(tx.configure(makeCfg(QStringLiteral("COM4"))).ok, "loop configure TX COM4");
    expect(rx.configure(makeCfg(QStringLiteral("COM5"))).ok, "loop configure RX COM5");
    expect(tx.open().ok, "loop open TX");
    expect(rx.open().ok, "loop open RX");

    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.sessionId = tx.sessionId();
    req.payload = QByteArray::fromHex("CAFE0102");
    expect(tx.send(req).ok, "loop send");

    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + 1500;
    while (got.size() < req.payload.size() && QDateTime::currentMSecsSinceEpoch() < deadline)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    expect(got == req.payload, "loop RX matches TX payload");

    expect(tx.close().ok, "loop close TX");
    expect(rx.close().ok, "loop close RX");
    expect(mgr.serialClaimCount() == 0, "loop claims released");
}

// 本机 TCP 回显：轻量 QTcpServer 仅用于自测，不进入产品能力
static void testTcpClientLoopback()
{
    QTcpServer server;
    expect(server.listen(QHostAddress::LocalHost, 0), "tcp echo server listen");
    const quint16 port = server.serverPort();

    QByteArray peerGot;
    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        QTcpSocket* peer = server.nextPendingConnection();
        QObject::connect(peer, &QTcpSocket::readyRead, peer, [peer, &peerGot]() {
            peerGot.append(peer->readAll());
            peer->write(peerGot);
            peer->flush();
        });
    });

    ca::SessionManager mgr;
    ca::NativeSession session(&mgr);

    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("tcp-selftest");
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::TcpClient;
    cfg.transport.tcpClient.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.tcpClient.remotePort = port;

    QByteArray rx;
    QObject::connect(&session, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind == ca::RecordKind::RawChunk && rec.direction == ca::Direction::Rx)
            rx.append(rec.bytes);
    });

    expect(session.configure(cfg).ok, "tcp configure");
    expect(session.open().ok, "tcp open loopback");

    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.sessionId = session.sessionId();
    req.payload = QByteArray::fromHex("AABBCC");
    expect(session.send(req).ok, "tcp send");

    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + 2000;
    while ((peerGot.size() < req.payload.size() || rx.size() < req.payload.size())
           && QDateTime::currentMSecsSinceEpoch() < deadline) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    expect(peerGot == req.payload, "tcp server got payload");
    expect(rx == req.payload, "tcp client got echo");
    expect(session.close().ok, "tcp close");
}

static void testTcpConnectFail()
{
    ca::SessionManager mgr;
    ca::NativeSession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::TcpClient;
    cfg.transport.tcpClient.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.tcpClient.remotePort = 1; // 通常无监听

    int errors = 0;
    QObject::connect(&session, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind == ca::RecordKind::ErrorEvent)
            ++errors;
    });

    expect(session.configure(cfg).ok, "tcp fail configure");
    expect(!session.open().ok, "tcp fail open refused/timeout");
    expect(session.state() == ca::SessionState::Closed, "tcp fail Closed");
    expect(errors == 1, "tcp fail single ErrorEvent");
}

// 探测本机可用 ephemeral 端口（claimFor 要求 localPort != 0）
static quint16 probeFreePort()
{
    QTcpServer probe;
    probe.listen(QHostAddress::LocalHost, 0);
    const quint16 port = probe.serverPort();
    probe.close();
    return port;
}

// 两路会话抢占同一监听端口，第二路 open 应失败
static void testListenConflict()
{
    const quint16 port = probeFreePort();
    ca::SessionManager mgr;
    ca::NativeSession sessionA(&mgr);
    ca::NativeSession sessionB(&mgr);

    auto makeCfg = [](quint16 listenPort) {
        ca::SessionConfig cfg;
        cfg.sessionId = QUuid::createUuid();
        cfg.name = QStringLiteral("listen-conflict");
        cfg.mode = ca::WorkMode::Native;
        cfg.role = ca::SessionRole::Tool;
        cfg.transport.kind = ca::TransportKind::TcpServer;
        cfg.transport.tcpServer.localAddress = QStringLiteral("127.0.0.1");
        cfg.transport.tcpServer.localPort = listenPort;
        return cfg;
    };

    expect(sessionA.configure(makeCfg(port)).ok, "listen conflict configure A");
    expect(sessionA.open().ok, "listen conflict open A");
    expect(mgr.listenClaimCount() == 1, "listen conflict A holds claim");

    expect(sessionB.configure(makeCfg(port)).ok, "listen conflict configure B");
    expect(!sessionB.open().ok, "listen conflict open B fails");
    expect(sessionB.state() == ca::SessionState::Created, "listen conflict B stays Created");
    expect(mgr.listenClaimCount() == 1, "listen conflict claim unchanged");

    expect(sessionA.close().ok, "listen conflict close A");
    expect(mgr.listenClaimCount() == 0, "listen conflict claims released");
}

template <typename Pred>
static bool waitUntil(Pred pred, int timeoutMs)
{
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (!pred() && QDateTime::currentMSecsSinceEpoch() < deadline)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return pred();
}

// TCP 服务端：双客户端定向隔离与广播
static void testTcpServerTwoClients()
{
    const quint16 port = probeFreePort();
    ca::SessionManager mgr;
    ca::NativeSession server(&mgr);

    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("tcp-server-two-clients");
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::TcpServer;
    cfg.transport.tcpServer.localAddress = QStringLiteral("127.0.0.1");
    cfg.transport.tcpServer.localPort = port;

    expect(server.configure(cfg).ok, "tcp server two configure");
    expect(server.open().ok, "tcp server two open");

    QTcpSocket client1;
    QTcpSocket client2;
    client1.connectToHost(QHostAddress::LocalHost, port);
    client2.connectToHost(QHostAddress::LocalHost, port);
    expect(client1.waitForConnected(2000), "tcp server client1 connected");
    expect(client2.waitForConnected(2000), "tcp server client2 connected");

    expect(waitUntil([&]() { return server.tcpChannels().size() >= 2; }, 2000), "tcp server two channels");

    const QVector<ca::TcpChannelInfo> channels = server.tcpChannels();
    expect(channels.size() == 2, "tcp server channel count");

    // 按客户端本地端口匹配 Channel，避免 accept 顺序与变量顺序不一致
    QString channelForClient1;
    QString channelForClient2;
    for (const ca::TcpChannelInfo& ch : channels) {
        if (ch.remotePort == client1.localPort())
            channelForClient1 = ch.channelId;
        if (ch.remotePort == client2.localPort())
            channelForClient2 = ch.channelId;
    }
    expect(!channelForClient1.isEmpty() && !channelForClient2.isEmpty(), "tcp server channel map by port");
    expect(channelForClient1 != channelForClient2, "tcp server channels distinct");

    QByteArray got1;
    QByteArray got2;
    QObject::connect(&client1, &QTcpSocket::readyRead, [&]() { got1.append(client1.readAll()); });
    QObject::connect(&client2, &QTcpSocket::readyRead, [&]() { got2.append(client2.readAll()); });

    const QByteArray payloadA = QByteArray::fromHex("AA01");
    const QByteArray payloadB = QByteArray::fromHex("BB02");
    const QByteArray payloadBroadcast = QByteArray::fromHex("CC03");

    ca::SendRequest req;
    req.sessionId = server.sessionId();
    req.channelId = channelForClient1;
    req.payload = payloadA;
    req.requestId = QUuid::createUuid();
    expect(server.send(req).ok, "tcp server directed A");

    expect(waitUntil([&]() { return got1.contains(payloadA); }, 2000), "tcp server client1 got A");
    expect(!got2.contains(payloadA), "tcp server client2 isolated from A");

    got1.clear();
    got2.clear();

    req.requestId = QUuid::createUuid();
    req.channelId = channelForClient2;
    req.payload = payloadB;
    expect(server.send(req).ok, "tcp server directed B");

    expect(waitUntil([&]() { return got2.contains(payloadB); }, 2000), "tcp server client2 got B");
    expect(!got1.contains(payloadB), "tcp server client1 isolated from B");

    got1.clear();
    got2.clear();

    req.requestId = QUuid::createUuid();
    req.channelId.clear();
    req.broadcast = true;
    req.payload = payloadBroadcast;
    expect(server.send(req).ok, "tcp server broadcast");

    expect(waitUntil([&]() { return got1.contains(payloadBroadcast) && got2.contains(payloadBroadcast); }, 2000),
           "tcp server broadcast both");

    expect(server.close().ok, "tcp server two close");
    expect(mgr.listenClaimCount() == 0, "tcp server two claim released");
}

// UDP：双端互通、空 Datagram、记录为 UdpDatagram 且带端点
static void testUdpPeerExchange()
{
    const quint16 portA = probeFreePort();
    const quint16 portB = probeFreePort();
    expect(portA != 0 && portB != 0 && portA != portB, "udp ports distinct");

    ca::SessionManager mgr;
    ca::NativeSession sessionA(&mgr);
    ca::NativeSession sessionB(&mgr);

    auto makeUdp = [](quint16 localPort, quint16 remotePort) {
        ca::SessionConfig cfg;
        cfg.sessionId = QUuid::createUuid();
        cfg.name = QStringLiteral("udp-peer");
        cfg.mode = ca::WorkMode::Native;
        cfg.role = ca::SessionRole::Tool;
        cfg.transport.kind = ca::TransportKind::Udp;
        cfg.transport.udp.localAddress = QStringLiteral("127.0.0.1");
        cfg.transport.udp.localPort = localPort;
        cfg.transport.udp.remoteAddress = QStringLiteral("127.0.0.1");
        cfg.transport.udp.remotePort = remotePort;
        return cfg;
    };

    expect(sessionA.configure(makeUdp(portA, portB)).ok, "udp A configure");
    expect(sessionB.configure(makeUdp(portB, portA)).ok, "udp B configure");
    expect(sessionA.open().ok, "udp A open");
    expect(sessionB.open().ok, "udp B open");
    expect(mgr.listenClaimCount() == 2, "udp two listen claims");

    QByteArray rxFromA;
    QByteArray rxFromB;
    quint16 peerPortSeen = 0;
    int udpRxOnB = 0;
    int udpRxOnA = 0;
    QObject::connect(&sessionB, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind != ca::RecordKind::UdpDatagram || rec.direction != ca::Direction::Rx)
            return;
        ++udpRxOnB;
        rxFromA = rec.bytes;
        peerPortSeen = rec.remoteEndpoint.port;
        expect(rec.localEndpoint.port == portB, "udp B rx local port");
    });
    QObject::connect(&sessionA, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& rec) {
        if (rec.kind != ca::RecordKind::UdpDatagram || rec.direction != ca::Direction::Rx)
            return;
        ++udpRxOnA;
        rxFromB = rec.bytes;
    });

    const QByteArray payload = QByteArray::fromHex("DEADBEEF");
    ca::SendRequest req;
    req.sessionId = sessionA.sessionId();
    req.requestId = QUuid::createUuid();
    req.payload = payload;
    expect(sessionA.send(req).ok, "udp A send");

    expect(waitUntil([&]() { return udpRxOnB >= 1; }, 2000), "udp B received datagram");
    expect(rxFromA == payload, "udp payload preserved");
    expect(peerPortSeen == portA, "udp peer port from A");

    // 空 Datagram
    req.requestId = QUuid::createUuid();
    req.payload.clear();
    expect(sessionB.send(req).ok, "udp B empty send");
    expect(waitUntil([&]() { return udpRxOnA >= 1; }, 2000), "udp A got empty datagram event");
    expect(rxFromB.isEmpty(), "udp empty payload preserved");

    expect(sessionA.close().ok, "udp A close");
    expect(sessionB.close().ok, "udp B close");
    expect(mgr.listenClaimCount() == 0, "udp claims released");
}

// UDP 同端口绑定冲突；UDP 与 TCP Server 同端口号不互相占用 claim
static void testUdpBindConflictAndTcpCoexist()
{
    const quint16 port = probeFreePort();
    ca::SessionManager mgr;
    ca::NativeSession udpA(&mgr);
    ca::NativeSession udpB(&mgr);
    ca::NativeSession tcpS(&mgr);

    auto makeUdp = [](quint16 p) {
        ca::SessionConfig cfg;
        cfg.sessionId = QUuid::createUuid();
        cfg.name = QStringLiteral("udp-bind");
        cfg.mode = ca::WorkMode::Native;
        cfg.role = ca::SessionRole::Tool;
        cfg.transport.kind = ca::TransportKind::Udp;
        cfg.transport.udp.localAddress = QStringLiteral("127.0.0.1");
        cfg.transport.udp.localPort = p;
        cfg.transport.udp.remoteAddress = QStringLiteral("127.0.0.1");
        cfg.transport.udp.remotePort = 1;
        return cfg;
    };

    expect(udpA.configure(makeUdp(port)).ok, "udp conflict configure A");
    expect(udpA.open().ok, "udp conflict open A");

    expect(udpB.configure(makeUdp(port)).ok, "udp conflict configure B");
    expect(!udpB.open().ok, "udp conflict open B fails");
    expect(mgr.listenClaimCount() == 1, "udp conflict still one claim");

    ca::SessionConfig tcpCfg;
    tcpCfg.sessionId = QUuid::createUuid();
    tcpCfg.name = QStringLiteral("tcp-same-port");
    tcpCfg.mode = ca::WorkMode::Native;
    tcpCfg.role = ca::SessionRole::Tool;
    tcpCfg.transport.kind = ca::TransportKind::TcpServer;
    tcpCfg.transport.tcpServer.localAddress = QStringLiteral("127.0.0.1");
    tcpCfg.transport.tcpServer.localPort = port;
    expect(tcpS.configure(tcpCfg).ok, "tcp coexist configure");
    expect(tcpS.open().ok, "tcp coexist open same port number");
    expect(mgr.listenClaimCount() == 2, "udp+tcp two claims");

    expect(udpA.close().ok, "udp conflict close A");
    expect(tcpS.close().ok, "tcp coexist close");
    expect(mgr.listenClaimCount() == 0, "udp conflict claims cleared");
}

// UDP 未配置远端时发送失败
static void testUdpSendWithoutRemote()
{
    const quint16 port = probeFreePort();
    ca::SessionManager mgr;
    ca::NativeSession session(&mgr);

    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("udp-no-remote");
    cfg.mode = ca::WorkMode::Native;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.kind = ca::TransportKind::Udp;
    cfg.transport.udp.localAddress = QStringLiteral("127.0.0.1");
    cfg.transport.udp.localPort = port;

    expect(session.configure(cfg).ok, "udp no-remote configure");
    expect(session.open().ok, "udp no-remote open");

    ca::SendRequest req;
    req.sessionId = session.sessionId();
    req.requestId = QUuid::createUuid();
    req.payload = QByteArrayLiteral("x");
    expect(!session.send(req).ok, "udp no-remote send fails");
    expect(session.close().ok, "udp no-remote close");
}


// 测试用会话：send 同步发出 Queued→Submitted；可延迟 Submitted 或强制 Failed
class FakeSession : public ca::ICommSession
{
public:
    explicit FakeSession(QObject* parent = nullptr)
        : ca::ICommSession(parent)
        , id_(QUuid::createUuid())
    {
    }

    ca::Result configure(const ca::SessionConfig&) override { return ca::Result::success(); }
    ca::Result open() override
    {
        state_ = ca::SessionState::Connected;
        emit stateChanged(state_);
        return ca::Result::success();
    }
    ca::Result close() override
    {
        state_ = ca::SessionState::Closed;
        emit stateChanged(state_);
        return ca::Result::success();
    }
    ca::Result send(const ca::SendRequest& request) override
    {
        if (state_ != ca::SessionState::Connected)
            return ca::Result::fail(QStringLiteral("not_connected"), QStringLiteral("未连接"));
        if (failNextSend_) {
            failNextSend_ = false;
            ca::CommRecord failed;
            failed.sessionId = id_;
            failed.requestId = request.requestId;
            failed.taskId = request.taskId;
            failed.kind = ca::RecordKind::RawChunk;
            failed.status = ca::RecordStatus::Failed;
            failed.direction = ca::Direction::Tx;
            failed.bytes = request.payload;
            failed.errorCode = QStringLiteral("forced_fail");
            failed.errorMessage = QStringLiteral("forced");
            emit recordReceived(failed);
            return ca::Result::fail(QStringLiteral("forced_fail"), QStringLiteral("forced"));
        }

        ca::CommRecord queued;
        queued.sessionId = id_;
        queued.requestId = request.requestId;
        queued.taskId = request.taskId;
        queued.kind = ca::RecordKind::RawChunk;
        queued.status = ca::RecordStatus::Queued;
        queued.direction = ca::Direction::Tx;
        queued.bytes = request.payload;
        emit recordReceived(queued);

        auto emitSubmitted = [this, request]() {
            ca::CommRecord submitted;
            submitted.sessionId = id_;
            submitted.requestId = request.requestId;
            submitted.taskId = request.taskId;
            submitted.kind = ca::RecordKind::RawChunk;
            submitted.status = ca::RecordStatus::Submitted;
            submitted.direction = ca::Direction::Tx;
            submitted.bytes = request.payload;
            ++sendCount_;
            lastPayloads_.push_back(request.payload);
            emit recordReceived(submitted);
        };

        if (submittedDelayMs_ > 0)
            QTimer::singleShot(submittedDelayMs_, this, emitSubmitted);
        else
            emitSubmitted();
        return ca::Result::success();
    }

    ca::SessionState state() const override { return state_; }
    QUuid sessionId() const override { return id_; }

    int sendCount_ = 0;
    QVector<QByteArray> lastPayloads_;
    int submittedDelayMs_ = 0;
    bool failNextSend_ = false;

private:
    QUuid id_;
    ca::SessionState state_ = ca::SessionState::Created;
};

static void testSchedulerOnceAndCounted()
{
    FakeSession session;
    expect(session.open().ok, "sched once open");
    ca::SendScheduler sched;
    sched.setSession(&session);

    ca::ScheduleTaskSpec once;
    once.mode = ca::ScheduleMode::Once;
    once.payloads = QVector<QByteArray>() << QByteArrayLiteral("A");
    once.intervalMs = 10;
    expect(sched.startTask(once).ok, "sched once start");
    expect(waitUntil([&]() { return sched.activeTaskCount() == 0 && session.sendCount_ == 1; }, 2000),
           "sched once completed one send");

    session.sendCount_ = 0;
    session.lastPayloads_.clear();
    ca::ScheduleTaskSpec counted;
    counted.mode = ca::ScheduleMode::Counted;
    counted.maxCount = 3;
    counted.intervalMs = 20;
    counted.payloads = QVector<QByteArray>() << QByteArrayLiteral("B");
    expect(sched.startTask(counted).ok, "sched counted start");
    expect(waitUntil([&]() { return sched.activeTaskCount() == 0 && session.sendCount_ == 3; }, 3000),
           "sched counted three submits");
}

static void testSchedulerRoundRobinAndPause()
{
    FakeSession session;
    expect(session.open().ok, "sched rr open");
    ca::SendScheduler sched;
    sched.setSession(&session);

    ca::ScheduleTaskSpec rr;
    rr.taskId = QUuid::createUuid();
    rr.mode = ca::ScheduleMode::RoundRobin;
    rr.maxCount = 4;
    rr.intervalMs = 15;
    rr.payloads = QVector<QByteArray>() << QByteArrayLiteral("P0") << QByteArrayLiteral("P1");
    expect(sched.startTask(rr).ok, "sched rr start");
    expect(waitUntil([&]() { return sched.activeTaskCount() == 0 && session.sendCount_ == 4; }, 3000),
           "sched rr four sends");
    expect(session.lastPayloads_.size() == 4, "sched rr payload count");
    if (session.lastPayloads_.size() == 4) {
        expect(session.lastPayloads_.at(0) == QByteArrayLiteral("P0"), "sched rr[0]");
        expect(session.lastPayloads_.at(1) == QByteArrayLiteral("P1"), "sched rr[1]");
        expect(session.lastPayloads_.at(2) == QByteArrayLiteral("P0"), "sched rr[2]");
        expect(session.lastPayloads_.at(3) == QByteArrayLiteral("P1"), "sched rr[3]");
    }

    session.sendCount_ = 0;
    ca::ScheduleTaskSpec inf;
    inf.taskId = QUuid::createUuid();
    inf.mode = ca::ScheduleMode::Infinite;
    inf.intervalMs = 30;
    inf.payloads = QVector<QByteArray>() << QByteArrayLiteral("X");
    expect(sched.startTask(inf).ok, "sched inf start");
    expect(waitUntil([&]() { return session.sendCount_ >= 1; }, 2000), "sched inf first");
    expect(sched.pauseTask(inf.taskId).ok, "sched pause");
    const int frozen = session.sendCount_;
    {
        const qint64 until = QDateTime::currentMSecsSinceEpoch() + 150;
        while (QDateTime::currentMSecsSinceEpoch() < until)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    expect(session.sendCount_ == frozen, "sched pause holds");
    expect(sched.resumeTask(inf.taskId).ok, "sched resume");
    expect(waitUntil([&]() { return session.sendCount_ > frozen; }, 2000), "sched resume continues");
    expect(sched.stopTask(inf.taskId).ok, "sched stop");
    expect(sched.activeTaskCount() == 0, "sched stopped empty");
}

static void testSchedulerFailedStopsAndSubmittedAnchor()
{
    FakeSession session;
    expect(session.open().ok, "sched fail open");
    ca::SendScheduler sched;
    sched.setSession(&session);

    int failedSignals = 0;
    QObject::connect(&sched, &ca::SendScheduler::taskFailed, [&](const QUuid&, const QString&, const QString&) {
        ++failedSignals;
    });

    ca::ScheduleTaskSpec counted;
    counted.mode = ca::ScheduleMode::Counted;
    counted.maxCount = 5;
    counted.intervalMs = 10;
    counted.payloads = QVector<QByteArray>() << QByteArrayLiteral("Z");
    session.failNextSend_ = true;
    expect(sched.startTask(counted).ok, "sched fail start accepts");
    expect(waitUntil([&]() { return sched.activeTaskCount() == 0; }, 2000), "sched fail stops task");
    expect(failedSignals == 1, "sched fail signal");
    expect(session.sendCount_ == 0, "sched fail no submitted");

    // Submitted 锚点：延迟 Submitted 前不得连发
    session.submittedDelayMs_ = 80;
    session.sendCount_ = 0;
    session.lastPayloads_.clear();
    ca::ScheduleTaskSpec delayed;
    delayed.mode = ca::ScheduleMode::Counted;
    delayed.maxCount = 2;
    delayed.intervalMs = 20;
    delayed.payloads = QVector<QByteArray>() << QByteArrayLiteral("D");
    expect(sched.startTask(delayed).ok, "sched delay start");
    QCoreApplication::processEvents(QEventLoop::AllEvents, 30);
    expect(session.sendCount_ == 0, "sched delay not submitted yet");
    expect(waitUntil([&]() { return session.sendCount_ == 2 && sched.activeTaskCount() == 0; }, 3000),
           "sched delay two after submitted+interval");
}

static void testCaptureJsonRoundtripAndHeader()
{
    ca::CommRecord rec;
    rec.sessionId = QUuid::createUuid();
    rec.sequence = 42;
    rec.kind = ca::RecordKind::RawChunk;
    rec.status = ca::RecordStatus::Submitted;
    rec.direction = ca::Direction::Tx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = 1234567890123LL;
    rec.bytes = QByteArray::fromHex("010203");
    rec.requestId = QUuid::createUuid();
    rec.summary = QStringLiteral("测试");
    rec.attributes.insert(QStringLiteral("kind"), QStringLiteral("should-not-overwrite-top"));
    rec.attributes.insert(QStringLiteral("custom"), 7);

    const QByteArray line = ca::serializeCaptureRecord(rec);
    const QJsonObject o = QJsonDocument::fromJson(line).object();
    expect(o.value(QStringLiteral("recordType")).toString() == QStringLiteral("record"), "cap json recordType");
    expect(o.value(QStringLiteral("kind")).toString() == QStringLiteral("RawChunk"), "cap json kind");
    expect(o.value(QStringLiteral("status")).toString() == QStringLiteral("Submitted"), "cap json status");
    expect(o.value(QStringLiteral("sequence")).toString() == QStringLiteral("42"), "cap json sequence string");
    expect(o.value(QStringLiteral("monotonicNs")).toString() == QStringLiteral("1234567890123"), "cap json ns string");
    expect(QByteArray::fromBase64(o.value(QStringLiteral("bytesBase64")).toString().toLatin1())
               == QByteArray::fromHex("010203"),
           "cap json bytes roundtrip");
    const QJsonObject attrs = o.value(QStringLiteral("attributes")).toObject();
    expect(attrs.value(QStringLiteral("custom")).toInt() == 7, "cap json attrs custom");
    expect(!attrs.contains(QStringLiteral("kind")), "cap json attrs reserved stripped");

    const QByteArray header = ca::serializeCaptureFileHeader(rec.sessionId, QDateTime::currentDateTimeUtc());
    const QJsonObject h = QJsonDocument::fromJson(header).object();
    expect(ca::isCaptureMagicOk(h), "cap header magic");
}

static void testCaptureWriterQueueAndIsolation()
{
    QTemporaryDir tmp;
    expect(tmp.isValid(), "cap tmp dir");

    ca::CaptureManager mgr;
    mgr.setOutputDirectory(tmp.path());
    mgr.setMaxQueue(2);

    const QUuid sidA = QUuid::createUuid();
    const QUuid sidB = QUuid::createUuid();
    expect(mgr.startSession(sidA, QStringLiteral("alpha")).ok, "cap start A");
    expect(mgr.startSession(sidB, QStringLiteral("beta")).ok, "cap start B");
    expect(mgr.activeWriterCount() == 2, "cap two writers");

    ca::CommRecord a;
    a.sessionId = sidA;
    a.sequence = 1;
    a.kind = ca::RecordKind::RawChunk;
    a.status = ca::RecordStatus::Observed;
    a.direction = ca::Direction::Rx;
    a.wallTime = QDateTime::currentDateTimeUtc();
    a.bytes = QByteArrayLiteral("AAAA");

    ca::CommRecord b = a;
    b.sessionId = sidB;
    b.bytes = QByteArrayLiteral("BBBB");

    expect(mgr.enqueue(a).ok, "cap enqueue A");
    expect(mgr.enqueue(b).ok, "cap enqueue B");

    // 等待落盘
    expect(waitUntil([&]() {
                QFile fa(mgr.filePathFor(sidA));
                QFile fb(mgr.filePathFor(sidB));
                return fa.exists() && fb.exists() && fa.size() > 10 && fb.size() > 10;
            },
            3000),
           "cap files grown");

    const QString pathA = mgr.filePathFor(sidA);
    const QString pathB = mgr.filePathFor(sidB);
    mgr.stopSession(sidA);
    mgr.stopSession(sidB);

    QFile fileA(pathA);
    QFile fileB(pathB);
    expect(fileA.open(QIODevice::ReadOnly | QIODevice::Text), "cap open A");
    expect(fileB.open(QIODevice::ReadOnly | QIODevice::Text), "cap open B");
    const QByteArray contentA = fileA.readAll();
    const QByteArray contentB = fileB.readAll();
    expect(contentA.contains(QByteArrayLiteral("CAREC01")), "cap A magic");
    expect(contentB.contains(QByteArrayLiteral("CAREC01")), "cap B magic");
    expect(contentA.contains(QByteArrayLiteral("QUFBQQ==")), "cap A bytes");
    expect(contentB.contains(QByteArrayLiteral("QkJCQg==")), "cap B bytes");
    expect(!contentA.contains(QByteArrayLiteral("QkJCQg==")), "cap A isolated from B");
    expect(!contentB.contains(QByteArrayLiteral("QUFBQQ==")), "cap B isolated from A");
}

static void testCaptureQueueFull()
{
    QTemporaryDir tmp;
    expect(tmp.isValid(), "cap full tmp");
    ca::CaptureManager mgr;
    mgr.setOutputDirectory(tmp.path());
    mgr.setMaxQueue(1);

    const QUuid sid = QUuid::createUuid();
    expect(mgr.startSession(sid, QStringLiteral("full")).ok, "cap full start");

    // 阻塞写线程：通过塞满队列而不 wait drain —— Writer 会尽快 drain，
    // 故用直接 CaptureWriter 测试更稳
    mgr.stopSession(sid);

    ca::CaptureWriter writer;
    expect(writer.open(QDir(tmp.path()).filePath(QStringLiteral("direct.clog.jsonl")), sid, 1).ok, "cap writer open");

    ca::CommRecord rec;
    rec.sessionId = sid;
    rec.sequence = 1;
    rec.kind = ca::RecordKind::ErrorEvent;
    rec.status = ca::RecordStatus::Observed;
    rec.direction = ca::Direction::System;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.bytes = QByteArray(256 * 1024, 'Z'); // 较大，拖慢写盘

    // 快速连续入队：容量 1，第二条在第一条未取走前应满
    int sawFull = 0;
    for (int i = 0; i < 1000; ++i) {
        rec.sequence = static_cast<quint64>(i);
        const ca::Result r = writer.enqueue(rec);
        if (!r.ok && r.code == QStringLiteral("capture_queue_full")) {
            ++sawFull;
            break;
        }
    }
    expect(sawFull == 1, "cap queue full observed");
    writer.close();
}

static void testCaptureTruncatedTailIgnored()
{
    // 模拟异常退出：完整行 + 半行；读取时只解析完整行
    QTemporaryDir tmp;
    expect(tmp.isValid(), "cap trunc tmp");
    const QString path = QDir(tmp.path()).filePath(QStringLiteral("trunc.clog.jsonl"));
    QFile f(path);
    expect(f.open(QIODevice::WriteOnly | QIODevice::Text), "cap trunc write open");
    const QUuid sid = QUuid::createUuid();
    f.write(ca::serializeCaptureFileHeader(sid, QDateTime::currentDateTimeUtc()));
    f.write("\n");
    ca::CommRecord rec;
    rec.sessionId = sid;
    rec.sequence = 9;
    rec.kind = ca::RecordKind::ConnectionEvent;
    rec.status = ca::RecordStatus::Observed;
    rec.direction = ca::Direction::System;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    f.write(ca::serializeCaptureRecord(rec));
    f.write("\n");
    f.write("{\"recordType\":\"record\",\"sequence\":\"99\""); // 残缺尾部
    f.close();

    QFile r(path);
    expect(r.open(QIODevice::ReadOnly | QIODevice::Text), "cap trunc read");
    int good = 0;
    int bad = 0;
    while (!r.atEnd()) {
        const QByteArray line = r.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            ++bad;
            continue;
        }
        ++good;
    }
    expect(good == 2, "cap trunc keeps complete lines");
    expect(bad == 1, "cap trunc drops incomplete");
}

static void testLegacyCapabilityTable()
{
    // 网口 0–7：逐项核对 Raw 恒关，并抽检关键能力位
    for (int i = 0; i <= 7; ++i) {
        const ca::LegacyCapabilityProfile p = ca::legacyCapabilityFor(ca::LegacyCommKind::Network, i);
        expect(!p.supports(ca::LegacyCapability::RawReceive), "cap net raw off");
        expect(!p.supports(ca::LegacyCapability::RawSend), "cap net rawsend off");
        expect(p.protocolIndex == i, "cap net index");
    }
    // 串口 0–4
    for (int i = 0; i <= 4; ++i) {
        const ca::LegacyCapabilityProfile p = ca::legacyCapabilityFor(ca::LegacyCommKind::Serial, i);
        expect(!p.supports(ca::LegacyCapability::RawReceive), "cap ser raw off");
        expect(p.protocolIndex == i, "cap ser index");
    }

    const ca::LegacyCapabilityProfile net0 = ca::legacyCapabilityFor(ca::LegacyCommKind::Network, 0);
    expect(!net0.supports(ca::LegacyCapability::ReceiveValues), "cap net0 no values");
    expect(net0.supports(ca::LegacyCapability::SendTransparentText), "cap net0 text");

    const ca::LegacyCapabilityProfile net2 = ca::legacyCapabilityFor(ca::LegacyCommKind::Network, 2);
    expect(net2.supports(ca::LegacyCapability::ReceiveValues), "cap net2 values");
    expect(!net2.supports(ca::LegacyCapability::SendTransparentText), "cap net2 no text");

    const ca::LegacyCapabilityProfile ser3 = ca::legacyCapabilityFor(ca::LegacyCommKind::Serial, 3);
    expect(ser3.supports(ca::LegacyCapability::SendEncodedValues), "cap ser3 send");
    expect(ser3.entries.value(static_cast<int>(ca::LegacyCapability::SendEncodedValues)).limitation.contains(QStringLiteral("5")),
           "cap ser3 need 5");
}

// Voltage(2)/Pulse(3) 须在 configure 阶段拒绝，不得进入 Worker
static void testLegacyVoltagePulseRejected()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.useMockBackend = true;
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 1;

    cfg.transport.legacy.commType = 2;
    expect(!session.configure(cfg).ok, "voltage configure rejected");
    cfg.transport.legacy.commType = 3;
    expect(!session.configure(cfg).ok, "pulse configure rejected");
    cfg.transport.legacy.commType = 0;
    expect(session.configure(cfg).ok, "network configure accepted");
}

static void testLegacySessionManagerExclusive()
{
    ca::SessionManager mgr;
    const QUuid a = QUuid::createUuid();
    const QUuid b = QUuid::createUuid();
    expect(mgr.acquireLegacy(a).ok, "legacy acquire A");
    expect(!mgr.acquireLegacy(b).ok, "legacy B blocked");
    mgr.releaseLegacy(a);
    expect(mgr.acquireLegacy(b).ok, "legacy B after release");
    mgr.releaseLegacy(b);
}

static void testLegacyWatchdog()
{
    ca::LegacyWatchdog dog;
    dog.setTimeoutMs(40);
    bool hit = false;
    QObject::connect(&dog, &ca::LegacyWatchdog::timedOut, [&](const QUuid&) { hit = true; });
    dog.arm(QUuid::createUuid());
    expect(waitUntil([&]() { return hit; }, 1000), "watchdog fires");
}

static void testLegacySessionMockPath()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);

    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("legacy-mock");
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 0;
    cfg.transport.legacy.protocolIndex = 2; // 中机：可收数值、可发数值
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 9000;
    cfg.transport.legacy.useMockBackend = true;

    expect(session.configure(cfg).ok, "legacy mock configure");
    expect(session.capabilityProfile().supports(ca::LegacyCapability::ReceiveValues), "legacy mock profile");

    QVector<ca::CommRecord> records;
    QObject::connect(&session, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& r) {
        records.push_back(r);
    });

    expect(session.open().ok, "legacy mock open accept");
    expect(waitUntil([&]() { return session.state() == ca::SessionState::Connected; }, 3000),
           "legacy mock connected");
    expect(mgr.hasActiveLegacySession(), "legacy mock exclusive held");

    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.sessionId = session.sessionId();
    req.attributes.insert(QStringLiteral("legacySend"), QStringLiteral("values"));
    QVariantList vals;
    vals << 1.0 << 2.0 << 3.0;
    req.attributes.insert(QStringLiteral("values"), vals);
    expect(session.send(req).ok, "legacy mock send");
    expect(waitUntil([&]() {
                for (const ca::CommRecord& r : records) {
                    if (r.requestId == req.requestId && r.status == ca::RecordStatus::Submitted)
                        return true;
                }
                return false;
            },
            3000),
           "legacy mock submitted");

    // 注入裸指针数值事件
    ca::MockLegacyBackend* mock = nullptr;
    // 通过 Worker 无法直接取 Mock；再开一条仅测 inject API
    ca::MockLegacyBackend injector;
    QVector<double> got;
    QObject::connect(&injector, &ca::ILegacyBackend::valuesReceived, [&](const QVector<double>& v, int) {
        got = v;
    });
    QVector<double> raw;
    raw << 9.0 << 8.0;
    injector.injectRawPointerData(raw, 1);
    expect(got.size() == 2 && got.at(0) == 9.0, "legacy direct copy");

    expect(session.close().ok, "legacy mock close");
    expect(waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000),
           "legacy mock closed");
    expect(!mgr.hasActiveLegacySession(), "legacy mock exclusive released");
}

static void testLegacySendCapabilityDenied()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 0;
    cfg.transport.legacy.protocolIndex = 3; // 三思：不支持发送
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 1;
    cfg.transport.legacy.useMockBackend = true;
    expect(session.configure(cfg).ok, "legacy deny configure");
    expect(session.open().ok, "legacy deny open");
    expect(waitUntil([&]() { return session.state() == ca::SessionState::Connected; }, 3000),
           "legacy deny connected");

    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.attributes.insert(QStringLiteral("legacySend"), QStringLiteral("values"));
    req.attributes.insert(QStringLiteral("values"), QVariantList() << 1.0);
    expect(!session.send(req).ok, "legacy deny send values");
    session.close();
    waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000);
}

// Opening 中立即 close：不得落到 Connected，互斥须释放
static void testLegacyOpenCancel()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 0;
    cfg.transport.legacy.protocolIndex = 0;
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 1;
    cfg.transport.legacy.useMockBackend = true;
    expect(session.configure(cfg).ok, "cancel configure");
    expect(session.open().ok, "cancel open accept");
    expect(session.close().ok, "cancel close while opening");
    expect(waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000),
           "cancel reaches Closed");
    expect(session.state() != ca::SessionState::Connected, "cancel never Connected");
    expect(!mgr.hasActiveLegacySession(), "cancel exclusive released");
}

// Mock 透明文本发送（能力允许时）
static void testLegacyMockTextSend()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 0;
    cfg.transport.legacy.protocolIndex = 0; // JSON：可发文本
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 2;
    cfg.transport.legacy.useMockBackend = true;
    expect(session.configure(cfg).ok, "text configure");
    expect(session.open().ok, "text open");
    expect(waitUntil([&]() { return session.state() == ca::SessionState::Connected; }, 3000),
           "text connected");

    QVector<ca::CommRecord> records;
    QObject::connect(&session, &ca::ICommSession::recordReceived, [&](const ca::CommRecord& r) {
        records.push_back(r);
    });

    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.attributes.insert(QStringLiteral("legacySend"), QStringLiteral("text"));
    req.payload = QByteArrayLiteral("hello-legacy");
    expect(session.send(req).ok, "text send accept");
    expect(waitUntil([&]() {
                for (const ca::CommRecord& r : records) {
                    if (r.requestId == req.requestId && r.status == ca::RecordStatus::Submitted)
                        return true;
                }
                return false;
            },
            3000),
           "text submitted");
    session.close();
    waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000);
}

#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
// 真实 DLL：网口 configure/open 冒烟；无对端时可失败但不得崩溃、须释放互斥
static void testLegacyDllConfigureSmoke()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("legacy-dll-smoke");
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 0;
    cfg.transport.legacy.protocolIndex = 0;
    cfg.transport.legacy.transferType = 0;
    cfg.transport.legacy.model = 1;
    cfg.transport.legacy.localAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.localPort = 0;
    cfg.transport.legacy.remoteAddress = QStringLiteral("127.0.0.1");
    cfg.transport.legacy.remotePort = 39876;
    cfg.transport.legacy.useMockBackend = false;

    expect(session.configure(cfg).ok, "dll smoke configure");
    expect(session.open().ok, "dll smoke open accept");
    const bool settled = waitUntil(
        [&]() {
            const ca::SessionState st = session.state();
            return st == ca::SessionState::Connected || st == ca::SessionState::Closed
                   || st == ca::SessionState::Faulted;
        },
        5000);
    expect(settled, "dll smoke settled");
    if (session.state() == ca::SessionState::Connected) {
        expect(session.close().ok, "dll smoke close");
        waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000);
    }
    expect(!mgr.hasActiveLegacySession(), "dll smoke exclusive released");
}

// 真实 DLL：串口路径 configure/open 冒烟（无可用端口时允许失败，但不得崩溃）
static void testLegacyDllSerialSmoke()
{
    ca::SessionManager mgr;
    ca::LegacySession session(&mgr);
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("legacy-dll-serial");
    cfg.mode = ca::WorkMode::LegacyDll;
    cfg.role = ca::SessionRole::Tool;
    cfg.transport.legacy.commType = 1;
    cfg.transport.legacy.protocolIndex = 0;
    cfg.transport.legacy.portName = QStringLiteral("COM247");
    cfg.transport.legacy.baudRate = 115200;
    cfg.transport.legacy.dataBits = 8;
    cfg.transport.legacy.parity = QStringLiteral("none");
    cfg.transport.legacy.stopBits = QStringLiteral("1");
    cfg.transport.legacy.useMockBackend = false;

    expect(session.configure(cfg).ok, "dll serial configure");
    expect(session.open().ok, "dll serial open accept");
    const bool settled = waitUntil(
        [&]() {
            const ca::SessionState st = session.state();
            return st == ca::SessionState::Connected || st == ca::SessionState::Closed
                   || st == ca::SessionState::Faulted;
        },
        5000);
    expect(settled, "dll serial settled");
    if (session.state() == ca::SessionState::Connected) {
        expect(session.close().ok, "dll serial close");
        waitUntil([&]() { return session.state() == ca::SessionState::Closed; }, 3000);
    }
    expect(!mgr.hasActiveLegacySession(), "dll serial exclusive released");
}
#endif

static void testListPorts()
{
    const auto ports = QSerialPortInfo::availablePorts();
    std::printf("[INFO] available serial ports: %d\n", ports.size());
    for (const QSerialPortInfo& p : ports)
        std::printf("       - %s (%s)\n", qPrintable(p.portName()), qPrintable(p.description()));
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    setvbuf(stdout, nullptr, _IONBF, 0);
    qRegisterMetaType<ca::LegacyConfig>("ca::LegacyConfig");
    qRegisterMetaType<ca::LegacyConfig>("LegacyConfig");
    qRegisterMetaType<QVector<double>>("QVector<double>");
    std::printf("=== CommunicationAssistant CoreSelfTest ===\n");
    testEnumNames();
    testSendRequiresConnected();
    testNormalize();
    testHex();
    testClaimFor();
    testSessionManager();
    testNativeOpenFail();
    testVirtualLoopback();
    testTcpClientLoopback();
    testTcpConnectFail();
    testListenConflict();
    testTcpServerTwoClients();
    testUdpPeerExchange();
    testUdpBindConflictAndTcpCoexist();
    testUdpSendWithoutRemote();
    testSchedulerOnceAndCounted();
    testSchedulerRoundRobinAndPause();
    testSchedulerFailedStopsAndSubmittedAnchor();
    testCaptureJsonRoundtripAndHeader();
    testCaptureWriterQueueAndIsolation();
    testCaptureQueueFull();
    testCaptureTruncatedTailIgnored();
    testLegacyCapabilityTable();
    testLegacyVoltagePulseRejected();
    testLegacySessionManagerExclusive();
    testLegacyWatchdog();
    testLegacySessionMockPath();
    testLegacySendCapabilityDenied();
    testLegacyOpenCancel();
    testLegacyMockTextSend();
#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
    testLegacyDllConfigureSmoke();
    testLegacyDllSerialSmoke();
#endif
    testListPorts();
    std::printf("=== done: %d failed ===\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
