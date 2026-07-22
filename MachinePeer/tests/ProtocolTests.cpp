// ProtocolTests.cpp — MachinePeer 协议黄金帧 + TCP/UDP 通道连通自检
#include "../src/NetworkProtocol.h"
#include "../src/PeerChannelManager.h"
#include "../src/SerialProtocol.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTextStream>
#include <QTimer>
#include <array>
#include <cmath>
#include <cstring>

namespace {

int g_failures = 0;

// 记录布尔断言结果
void check(bool condition, const QString& name)
{
    QTextStream output(stdout);
    output << (condition ? "PASS " : "FAIL ") << name << Qt::endl;
    if (!condition)
        ++g_failures;
}

// 比较浮点值
bool near(double lhs, double rhs)
{
    return std::fabs(lhs - rhs) < 1e-8;
}

// HEX 空格串转字节
QByteArray hexBytes(const char* spacedHex)
{
    return QByteArray::fromHex(QByteArray(spacedHex));
}

// 构造动态库 FixedDoubleToHex 的 14 字节科新回包（含 02 4C 头）
QByteArray kexinReply(double value)
{
    return SerialProtocol::packMeasurement(1, value, 0.0, nullptr);
}

// 构造动态库 DataPack 的 56 字节 IEEE 回包
QByteArray ieeeReply(const std::array<float, 6>& values)
{
    QByteArray frame(56, '\0');
    frame[0] = char(0x24);
    frame[1] = char(0x01);
    unsigned int byteSum = 0;
    for (int channel = 0; channel < 6; ++channel) {
        unsigned char bytes[sizeof(float)]{};
        std::memcpy(bytes, &values[channel], sizeof(float));
        for (unsigned char byte : bytes)
            byteSum += byte;
        const QByteArray hex =
            QByteArray(reinterpret_cast<const char*>(bytes), sizeof(bytes)).toHex().toUpper();
        const QByteArray networkHex =
            hex.mid(6, 2) + hex.mid(4, 2) + hex.mid(2, 2) + hex.mid(0, 2);
        std::memcpy(frame.data() + 2 + channel * 8, networkHex.constData(), 8);
    }
    const float checksumSource = static_cast<float>(byteSum);
    unsigned char checksum[sizeof(float)]{};
    std::memcpy(checksum, &checksumSource, sizeof(float));
    const QByteArray checksumHex =
        QByteArray(1, static_cast<char>(checksum[1])).toHex().toUpper()
        + QByteArray(1, static_cast<char>(checksum[0])).toHex().toUpper();
    std::memcpy(frame.data() + 50, checksumHex.constData(), 4);
    frame[54] = char(0x0D);
    frame[55] = char(0x0A);
    return frame;
}

// 验证串口 PT0–PT5 的组包与解包（对齐 CommHandler）
void testSerialProtocols()
{
    QString error;
    check(SerialProtocol::packCommand(0, 0, &error).toHex().toUpper() == "3D0D",
          QStringLiteral("串口 PT0 开始"));
    check(SerialProtocol::packCommand(0, 1, &error).toHex().toUpper() == "3E0D",
          QStringLiteral("串口 PT0 停止"));
    check(SerialProtocol::packCommand(0, 2, &error).toHex().toUpper() == "6B0D",
          QStringLiteral("串口 PT0 存图"));
    check(SerialProtocol::packCommand(0, 3, &error).toHex().toUpper() == "5A0D",
          QStringLiteral("串口 PT0 清零"));
    check(SerialProtocol::packMeasurement(0, 1.0, 25.0, &error).toHex().toUpper()
              == "E2000F4240",
          QStringLiteral("串口 PT0 测数"));
    ProtocolResult result = SerialProtocol::parseReply(0, QByteArray("+001.000\r"));
    check(result.ok && result.hasValues && near(result.force, 1.0),
          QStringLiteral("串口 PT0 回包"));

    // 科新：控制字 + FixedDoubleToHex 14B（含 02 4C）
    check(SerialProtocol::canSendCommand(1, 0) && SerialProtocol::canSendCommand(1, 1),
          QStringLiteral("串口 PT1 可发控制"));
    check(SerialProtocol::packCommand(1, 0, &error) == QByteArray("{QLI[1]}"),
          QStringLiteral("串口 PT1 开始 {QLI[1]}"));
    check(SerialProtocol::packCommand(1, 1, &error) == QByteArray("{QLI[2]}"),
          QStringLiteral("串口 PT1 停止 {QLI[2]}"));
    check(SerialProtocol::packMeasurement(1, 12.3, 25.0, &error)
              == hexBytes("02 4C 30 30 30 31 32 2E 33 30 30 30 30 23"),
          QStringLiteral("串口 PT1 测数 12.3 黄金帧"));
    check(SerialProtocol::packMeasurement(1, 1.0, 25.0, &error).toHex().toUpper()
              == "024C30303030312E303030303023",
          QStringLiteral("串口 PT1 测数 1.0"));
    result = SerialProtocol::parseReply(1, kexinReply(1.0));
    check(result.ok && result.hasValues && near(result.force, 1.0),
          QStringLiteral("串口 PT1 14B 回包"));
    QByteArray sticky = kexinReply(1.0) + kexinReply(2.0);
    check(SerialProtocol::takeKexinReplyFrames(&sticky).size() == 2 && sticky.isEmpty(),
          QStringLiteral("串口 PT1 粘包"));

    check(SerialProtocol::packMeasurement(2, 1.0, 25.0, &error)
              == QByteArray("1.000,0,RUN,0\r\n"),
          QStringLiteral("串口 PT2 测数"));
    result = SerialProtocol::parseReply(2, QByteArray("NO_REPLY"));
    check(result.ok && !result.isAck && !result.hasValues,
          QStringLiteral("串口 PT2 无业务回包"));

    check(SerialProtocol::packCommand(3, 0, &error).toHex().toUpper() == "24000D0A",
          QStringLiteral("串口 PT3 开始"));
    check(SerialProtocol::packCommand(3, 1, &error).toHex().toUpper() == "24FF0D0A",
          QStringLiteral("串口 PT3 停止"));
    check(!SerialProtocol::canSendMeasurement(3), QStringLiteral("串口 PT3 无测数"));
    QByteArray ieee = ieeeReply({1.0f, 2.0f, 3.0f, 4.0f, 0.0f, 5.0f});
    result = SerialProtocol::parseReply(3, ieee);
    check(result.ok && result.hasValues && near(result.force, 1.0)
              && near(result.temp, 2.0),
          QStringLiteral("串口 PT3 56B 回包"));
    QByteArray ieeeBuffer = ieee.left(10);
    check(SerialProtocol::takeIeeeReplyFrames(&ieeeBuffer).isEmpty(),
          QStringLiteral("串口 PT3 半包等待"));
    ieeeBuffer.append(ieee.mid(10));
    ieeeBuffer.append(ieee);
    check(SerialProtocol::takeIeeeReplyFrames(&ieeeBuffer).size() == 2
              && ieeeBuffer.isEmpty(),
          QStringLiteral("串口 PT3 粘包"));
    ieee[50] = ieee.at(50) == '0' ? '1' : '0';
    check(!SerialProtocol::parseReply(3, ieee).ok,
          QStringLiteral("串口 PT3 校验失败拒绝"));

    check(!SerialProtocol::canSendCommand(4, 0), QStringLiteral("串口 PT4 无控制指令"));
    check(!SerialProtocol::canSendMeasurement(4), QStringLiteral("串口 PT4 无测数"));
    result = SerialProtocol::parseReply(4, QByteArray("R+001.0000#N+025.0000#"));
    check(result.ok && result.hasValues && result.hasTemp,
          QStringLiteral("串口 PT4 回包"));

    // 联恒串口 PT5：SER5 fixture
    check(SerialProtocol::canSendCommand(5, 0) && SerialProtocol::canSendMeasurement(5),
          QStringLiteral("串口 PT5 能力"));
    const QByteArray serLhgk = hexBytes(
        "AA 55 01 14 00 06 0C 9A 99 99 99 99 99 28 40 06 0D CD CC CC CC CC CC 46 40 02 67 0D 0A");
    check(SerialProtocol::packMeasurement(5, 12.3, 45.6, &error) == serLhgk,
          QStringLiteral("串口 PT5 测数黄金帧"));
    result = SerialProtocol::parseReply(5, serLhgk);
    check(result.ok && result.hasValues && result.hasTemp && near(result.force, 12.3)
              && near(result.temp, 45.6),
          QStringLiteral("串口 PT5 回包解析"));
    const QByteArray serStart = SerialProtocol::packCommand(5, 0, &error);
    check(serStart.size() == 10 && static_cast<quint8>(serStart.at(5)) == 0x01,
          QStringLiteral("串口 PT5 开始帧"));
    result = SerialProtocol::parseReply(5, serStart);
    check(result.ok && result.isAck && result.detail.contains(QStringLiteral("开始")),
          QStringLiteral("串口 PT5 控制解析"));
}

// 验证网口 PT0–PT8 的组包与解包（对齐 CommHandler）
void testNetworkProtocols()
{
    QString error;
    check(NetworkProtocol::packCommand(0, 0, &error) == QByteArray("{\"tn\":1}"),
          QStringLiteral("网口 PT0 开始"));
    check(NetworkProtocol::packCommand(0, 1, &error) == QByteArray("{\"tn\":3}"),
          QStringLiteral("网口 PT0 停止"));
    ProtocolResult result =
        NetworkProtocol::parseReply(0, QByteArray("{\"tn\":2,\"values\":[1.0,25.0]}"));
    check(result.ok && result.hasValues && result.hasTemp,
          QStringLiteral("网口 PT0 回包"));

    check(NetworkProtocol::packCommand(1, 0, &error) == QByteArray("START"),
          QStringLiteral("网口 PT1 开始"));
    check(NetworkProtocol::packCommand(1, 1, &error) == QByteArray("STOP"),
          QStringLiteral("网口 PT1 停止"));
    result = NetworkProtocol::parseReply(1, QByteArray("1.0,25.0"));
    check(result.ok && result.hasValues && result.hasTemp,
          QStringLiteral("网口 PT1 回包"));

    const QByteArray zhongjiStart = NetworkProtocol::packCommand(2, 0, &error);
    check(zhongjiStart.toHex().toUpper() == "010A", QStringLiteral("网口 PT2 开始"));
    const QByteArray zhongjiData = NetworkProtocol::packMeasurement(2, 1.0, 25.0, &error);
    check(zhongjiData.size() == 33 && static_cast<unsigned char>(zhongjiData.at(0)) == 0x02,
          QStringLiteral("网口 PT2 测数"));
    NetworkProtocol::ZhongjiControl ack{0x03, 0x67};
    result = NetworkProtocol::parseReply(
        2, QByteArray(reinterpret_cast<const char*>(&ack), sizeof(ack)));
    check(result.ok && result.isAck, QStringLiteral("网口 PT2 ACK"));
    NetworkProtocol::ZhongjiControl notAck{0x01, 0x0A};
    result = NetworkProtocol::parseReply(
        2, QByteArray(reinterpret_cast<const char*>(&notAck), sizeof(notAck)));
    check(!result.ok && !result.isAck, QStringLiteral("网口 PT2 非 ACK 拒绝"));

    const QByteArray sansiData = NetworkProtocol::packMeasurement(3, 1.0, 25.0, &error);
    check(sansiData.size() == 24, QStringLiteral("网口 PT3 测数"));
    check(!NetworkProtocol::canSendCommand(3, 0), QStringLiteral("网口 PT3 无控制指令"));

    check(NetworkProtocol::packCommand(4, 2, &error) == QByteArray("SAVE\r\n"),
          QStringLiteral("网口 PT4 存图"));
    check(!NetworkProtocol::canSendMeasurement(4), QStringLiteral("网口 PT4 无测数"));

    check(NetworkProtocol::packMeasurement(5, 1.0, 25.0, &error)
              == QByteArray("TL1.00E0D0M0"),
          QStringLiteral("网口 PT5 测数"));
    check(!NetworkProtocol::canSendCommand(5, 0), QStringLiteral("网口 PT5 无控制指令"));

    check(NetworkProtocol::packCommand(6, 0, &error)
              == QByteArray("{\"name\":\"alphaStartTest\"}"),
          QStringLiteral("网口 PT6 开始"));
    result = NetworkProtocol::parseReply(
        6, QByteArray("{\"name\":\"LVEComputedValues\",\"values\":[1.0,25.0]}"));
    check(result.ok && result.hasValues && result.hasTemp,
          QStringLiteral("网口 PT6 回包"));

    check(NetworkProtocol::packCommand(7, 0, &error) == QByteArray("calcStart"),
          QStringLiteral("网口 PT7 开始"));
    result = NetworkProtocol::parseReply(
        7, QByteArray("{\"hsm\":{\"L1\":\"1.0\",\"b1\":\"25.0\"}}"));
    check(result.ok && result.hasValues && result.hasTemp,
          QStringLiteral("网口 PT7 回包"));

    // 联恒网口 PT8：RX 刺激 / TX 解析（NET8 fixtures）
    check(NetworkProtocol::canSendCommand(8, 0) && NetworkProtocol::canSendMeasurement(8),
          QStringLiteral("网口 PT8 能力"));
    const QByteArray netRx = hexBytes(
        "AA 55 01 00 02 06 0C 40 28 99 99 99 99 99 9A 06 0D 40 46 CC CC CC CC CC CD C6 0D 0D 0A");
    check(NetworkProtocol::packMeasurement(8, 12.3, 45.6, &error) == netRx,
          QStringLiteral("网口 PT8 RX 测数黄金帧"));
    result = NetworkProtocol::parseReply(8, netRx);
    check(result.ok && result.hasValues && result.hasTemp && near(result.force, 12.3)
              && near(result.temp, 45.6),
          QStringLiteral("网口 PT8 RX 解析"));
    const QByteArray netTx = hexBytes(
        "AA 55 01 00 15 02 06 0B 40 28 99 99 99 99 99 9A 06 0C 40 46 CC CC CC CC CC CD 6F 2D 0D 0A");
    result = NetworkProtocol::parseReply(8, netTx);
    check(result.ok && result.hasValues && result.hasTemp && near(result.force, 12.3)
              && near(result.temp, 45.6),
          QStringLiteral("网口 PT8 软件 TX 解析"));
    const QByteArray netStart = NetworkProtocol::packCommand(8, 0, &error);
    check(netStart.size() == 10 && static_cast<quint8>(netStart.at(5)) == 0x01,
          QStringLiteral("网口 PT8 开始帧"));
    result = NetworkProtocol::parseReply(8, netStart);
    check(result.ok && result.isAck && result.detail.contains(QStringLiteral("开始")),
          QStringLiteral("网口 PT8 控制解析"));
}

// 验证 PeerChannelManager TCP 服务端↔客户端环回
void testPeerChannelTcpLoopback()
{
    PeerChannelManager server;
    PeerNetworkSettings serverSettings;
    serverSettings.localIp = QStringLiteral("127.0.0.1");
    serverSettings.localPort = 19090;
    serverSettings.destinationIp = QStringLiteral("127.0.0.1");
    serverSettings.destinationPort = 19090;
    serverSettings.transferType = 0;
    serverSettings.model = 0;
    QString error;
    check(server.openNetwork(serverSettings, &error), QStringLiteral("TCP 服务端监听"));

    bool peerConnected = false;
    QObject::connect(&server, &PeerChannelManager::networkPeerConnected,
                     [&peerConnected](const QString&) { peerConnected = true; });

    QByteArray serverRx;
    QObject::connect(&server, &PeerChannelManager::networkDatagramReceived,
                     [&serverRx](const QByteArray& bytes) { serverRx.append(bytes); });

    PeerChannelManager client;
    PeerNetworkSettings clientSettings = serverSettings;
    clientSettings.model = 1;
    check(client.openNetwork(clientSettings, &error), QStringLiteral("TCP 客户端连接"));

    QEventLoop loop;
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
    check(peerConnected, QStringLiteral("TCP 服务端收到连接"));

    check(client.sendNetwork(QByteArray("HELLO"), &error), QStringLiteral("TCP 客户端发送"));
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
    check(serverRx == QByteArray("HELLO"), QStringLiteral("TCP 服务端收包"));

    QByteArray clientRx;
    QObject::connect(&client, &PeerChannelManager::networkDatagramReceived,
                     [&clientRx](const QByteArray& bytes) { clientRx.append(bytes); });
    check(server.sendNetwork(QByteArray("ACK"), &error), QStringLiteral("TCP 服务端发送"));
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
    check(clientRx == QByteArray("ACK"), QStringLiteral("TCP 客户端收包"));

    server.close();
    client.close();
}

// 验证 PeerChannelManager UDP 绑定与交叉端口收发
void testPeerChannelUdpLoopback()
{
    PeerChannelManager peerA;
    PeerNetworkSettings settingsA;
    settingsA.localIp = QStringLiteral("127.0.0.1");
    settingsA.localPort = 19116;
    settingsA.destinationIp = QStringLiteral("127.0.0.1");
    settingsA.destinationPort = 19115;
    settingsA.transferType = 1;
    settingsA.model = 0;
    QString error;
    check(peerA.openNetwork(settingsA, &error), QStringLiteral("UDP A 绑定"));

    PeerChannelManager peerB;
    PeerNetworkSettings settingsB = settingsA;
    settingsB.localPort = 19115;
    settingsB.destinationPort = 19116;
    check(peerB.openNetwork(settingsB, &error), QStringLiteral("UDP B 绑定"));

    QByteArray rxB;
    QObject::connect(&peerB, &PeerChannelManager::networkDatagramReceived,
                     [&rxB](const QByteArray& bytes) { rxB.append(bytes); });

    check(peerA.sendNetwork(QByteArray("UDPPING"), &error), QStringLiteral("UDP A 发送"));
    QEventLoop loop;
    QTimer::singleShot(150, &loop, &QEventLoop::quit);
    loop.exec();
    check(rxB == QByteArray("UDPPING"), QStringLiteral("UDP B 收包"));

    peerA.close();
    peerB.close();
}

} // namespace

// 运行全部协议黄金帧与通道连通测试
int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    testSerialProtocols();
    testNetworkProtocols();
    testPeerChannelTcpLoopback();
    testPeerChannelUdpLoopback();
    QTextStream(stdout) << "TOTAL_FAILURES " << g_failures << Qt::endl;
    return g_failures == 0 ? 0 : 1;
}
