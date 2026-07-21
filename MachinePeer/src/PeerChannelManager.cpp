// PeerChannelManager.cpp — MachinePeer 串口/网口（TCP/UDP）初始化与收发实现
#include "PeerChannelManager.h"

#include "NetworkProtocol.h"
#include "SerialProtocol.h"

#include <QAbstractSocket>
#include <QHostAddress>

// 创建串口/UDP/TCP 对象并挂接统一接收槽
PeerChannelManager::PeerChannelManager(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&m_serial, &QSerialPort::readyRead,
                     this, &PeerChannelManager::receiveSerialBytes);
    QObject::connect(&m_udp, &QUdpSocket::readyRead,
                     this, &PeerChannelManager::receiveNetworkDatagrams);
    QObject::connect(&m_tcpServer, &QTcpServer::newConnection,
                     this, &PeerChannelManager::onTcpNewConnection);
    QObject::connect(&m_tcpClient, &QTcpSocket::readyRead,
                     this, &PeerChannelManager::receiveTcpBytes);
    QObject::connect(&m_tcpClient, &QTcpSocket::disconnected,
                     this, &PeerChannelManager::onTcpDisconnected);
}

// 关闭旧通道并打开串口
bool PeerChannelManager::openSerial(const PeerSerialSettings& settings, QString* error)
{
    close();
    m_networkMode = false;
    m_open = SerialProtocol::open(&m_serial, settings.portIndex, settings.baudIndex,
                                  settings.dataBitsIndex, settings.parityIndex,
                                  settings.stopBitsIndex, error);
    return m_open;
}

// 关闭旧通道并打开网口（TCP 服务端/客户端或 UDP）
bool PeerChannelManager::openNetwork(const PeerNetworkSettings& settings, QString* error)
{
    close();
    m_networkMode = true;
    m_network = settings;

    if (settings.localPort <= 0 || settings.destinationPort <= 0) {
        if (error)
            *error = QStringLiteral("本地/远端端口须为 1–65535");
        return false;
    }

    // UDP：绑定本机，发往软件侧
    if (settings.transferType == 1) {
        m_open = NetworkProtocol::open(&m_udp, settings.localIp, settings.localPort, error);
        return m_open;
    }

    // TCP 服务端：监听，等待助手（默认 TCP 客户端）接入
    if (settings.model == 0) {
        const QHostAddress addr = settings.localIp.trimmed().isEmpty()
                                      ? QHostAddress::Any
                                      : QHostAddress(settings.localIp);
        if (!m_tcpServer.listen(addr, static_cast<quint16>(settings.localPort))) {
            if (error)
                *error = m_tcpServer.errorString();
            return false;
        }
        m_open = true;
        return true;
    }

    // TCP 客户端：连接软件侧服务端
    m_tcpClient.connectToHost(QHostAddress(settings.destinationIp),
                              static_cast<quint16>(settings.destinationPort));
    if (!m_tcpClient.waitForConnected(3000)) {
        if (error)
            *error = m_tcpClient.errorString().isEmpty()
                         ? QStringLiteral("TCP 连接超时")
                         : m_tcpClient.errorString();
        m_tcpClient.abort();
        return false;
    }
    m_open = true;
    emit networkPeerConnected(QStringLiteral("%1:%2")
                                  .arg(settings.destinationIp)
                                  .arg(settings.destinationPort));
    return true;
}

// 关闭所有通道
void PeerChannelManager::close()
{
    SerialProtocol::close(&m_serial);
    NetworkProtocol::close(&m_udp);
    m_tcpServer.close();
    clearAcceptedSocket();
    if (m_tcpClient.state() != QAbstractSocket::UnconnectedState)
        m_tcpClient.abort();
    m_open = false;
}

// 经当前串口写出完整帧并返回 HEX
bool PeerChannelManager::sendSerial(const QByteArray& frame, QString* hex, QString* error)
{
    if (!m_open || m_networkMode) {
        if (error)
            *error = QStringLiteral("串口未打开");
        return false;
    }
    if (SerialProtocol::write(&m_serial, frame, hex))
        return true;
    if (error)
        *error = m_serial.errorString();
    return false;
}

// 经当前网口写出（TCP 已连接或 UDP）
bool PeerChannelManager::sendNetwork(const QByteArray& datagram, QString* error)
{
    if (!m_open || !m_networkMode) {
        if (error)
            *error = QStringLiteral("网口未打开");
        return false;
    }

    if (m_network.transferType == 1) {
        if (NetworkProtocol::write(&m_udp, m_network.destinationIp,
                                   m_network.destinationPort, datagram))
            return true;
        if (error)
            *error = m_udp.errorString();
        return false;
    }

    QTcpSocket* sock = activeTcpSocket();
    if (!sock || sock->state() != QAbstractSocket::ConnectedState) {
        if (error)
            *error = QStringLiteral("TCP 对端未连接（服务端须等待助手接入）");
        return false;
    }
    const qint64 n = sock->write(datagram);
    if (n != datagram.size()) {
        if (error)
            *error = sock->errorString().isEmpty()
                         ? QStringLiteral("TCP 写出不完整")
                         : sock->errorString();
        return false;
    }
    sock->flush();
    return true;
}

// 返回当前网口是否已具备可写对端
bool PeerChannelManager::isNetworkPeerReady() const
{
    if (!m_open || !m_networkMode)
        return false;
    if (m_network.transferType == 1)
        return true;
    QTcpSocket* sock = activeTcpSocket();
    return sock && sock->state() == QAbstractSocket::ConnectedState;
}

// 读取串口当前全部可用字节
void PeerChannelManager::receiveSerialBytes()
{
    const QByteArray bytes = m_serial.readAll();
    if (!bytes.isEmpty())
        emit serialBytesReceived(bytes);
}

// 逐个读取 UDP 完整数据报
void PeerChannelManager::receiveNetworkDatagrams()
{
    while (m_udp.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udp.pendingDatagramSize()));
        m_udp.readDatagram(datagram.data(), datagram.size());
        emit networkDatagramReceived(datagram);
    }
}

// 读取当前 TCP 套接字可用字节
void PeerChannelManager::receiveTcpBytes()
{
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock)
        sock = activeTcpSocket();
    if (!sock)
        return;
    const QByteArray bytes = sock->readAll();
    if (!bytes.isEmpty())
        emit networkDatagramReceived(bytes);
}

// TCP 服务端接受新连接（仅保留最近一个客户端，对齐助手单会话）
void PeerChannelManager::onTcpNewConnection()
{
    while (m_tcpServer.hasPendingConnections()) {
        QTcpSocket* incoming = m_tcpServer.nextPendingConnection();
        if (!incoming)
            continue;
        clearAcceptedSocket();
        m_tcpAccepted = incoming;
        m_tcpAccepted->setParent(this);
        QObject::connect(m_tcpAccepted, &QTcpSocket::readyRead,
                         this, &PeerChannelManager::receiveTcpBytes);
        QObject::connect(m_tcpAccepted, &QTcpSocket::disconnected,
                         this, &PeerChannelManager::onTcpDisconnected);
        const QString ep = QStringLiteral("%1:%2")
                               .arg(m_tcpAccepted->peerAddress().toString())
                               .arg(m_tcpAccepted->peerPort());
        emit networkPeerConnected(ep);
    }
}

// TCP 套接字断开
void PeerChannelManager::onTcpDisconnected()
{
    emit networkPeerDisconnected();
    if (sender() == m_tcpAccepted)
        clearAcceptedSocket();
}

// 释放已接受的 TCP 客户端套接字
void PeerChannelManager::clearAcceptedSocket()
{
    if (!m_tcpAccepted)
        return;
    m_tcpAccepted->disconnect(this);
    m_tcpAccepted->deleteLater();
    m_tcpAccepted = nullptr;
}

// 返回当前用于收发的 TCP 套接字
QTcpSocket* PeerChannelManager::activeTcpSocket() const
{
    if (m_network.transferType != 0)
        return nullptr;
    if (m_network.model == 0)
        return m_tcpAccepted;
    return const_cast<QTcpSocket*>(&m_tcpClient);
}
