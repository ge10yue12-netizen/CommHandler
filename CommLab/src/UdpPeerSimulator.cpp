#include "UdpPeerSimulator.h"

#include "UIDef.h"

#include <QEventLoop>
#include <QTimer>
#include <cstring>

UdpPeerSimulator::UdpPeerSimulator(QObject* parent)
    : QObject(parent)
{
    // 有数据报可读时转入 onReadyRead。
    QObject::connect(&m_socket, &QUdpSocket::readyRead, this, &UdpPeerSimulator::onReadyRead);
}

// 绑定本机 UDP 端口；失败返回 false。
bool UdpPeerSimulator::startListen(quint16 port, const QHostAddress& addr)
{
    stop();
    clearReceived();
    return m_socket.bind(addr, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

// 关闭套接字。
void UdpPeerSimulator::stop()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
}

// 向指定地址发送原始数据报。
qint64 UdpPeerSimulator::sendRaw(const QByteArray& packet,
                                 const QHostAddress& host,
                                 quint16 port)
{
    return m_socket.writeDatagram(packet, host, port);
}

// 清空已收列表（自检用）。
void UdpPeerSimulator::clearReceived()
{
    m_received.clear();
}

// 已收数据报个数。
int UdpPeerSimulator::receivedCount() const
{
    return m_received.size();
}

// 阻塞等待至少 minCount 个包或超时。
bool UdpPeerSimulator::waitReceived(int minCount, int timeoutMs)
{
    if (m_received.size() >= minCount) {
        return true;
    }
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    const QMetaObject::Connection conn =
        QObject::connect(this, &UdpPeerSimulator::datagramReceived, &loop, [&]() {
            if (m_received.size() >= minCount) {
                loop.quit();
            }
        });
    timer.start(timeoutMs);
    loop.exec();
    QObject::disconnect(conn);
    return m_received.size() >= minCount;
}

// 组三思网口 24 字节 Data 包（力/位移/应变）。
QByteArray UdpPeerSimulator::makeSansiPacket(double force,
                                             double displacement,
                                             double strain)
{
    Data d{};
    d.dForce = force;
    d.dMove = displacement;
    d.dStrain = strain;
    QByteArray packet(static_cast<int>(sizeof(Data)), Qt::Uninitialized);
    std::memcpy(packet.data(), &d, sizeof(Data));
    return packet;
}

// 排空 pendingDatagrams 并记入 m_received。
void UdpPeerSimulator::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(static_cast<int>(m_socket.pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_socket.readDatagram(buf.data(), buf.size(), &sender, &senderPort);
        m_received.push_back(buf);
        emit datagramReceived(buf, sender, senderPort);
    }
}