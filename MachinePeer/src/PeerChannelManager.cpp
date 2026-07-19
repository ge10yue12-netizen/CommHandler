// PeerChannelManager.cpp — MachinePeer 串口/网口初始化与收发实现
#include "PeerChannelManager.h"

#include "NetworkProtocol.h"
#include "SerialProtocol.h"

// 创建串口与 UDP 对象并挂接统一接收槽
PeerChannelManager::PeerChannelManager(QObject* parent)
    : QObject(parent)
{
    QObject::connect(&m_serial, &QSerialPort::readyRead,
                     this, &PeerChannelManager::receiveSerialBytes);
    QObject::connect(&m_udp, &QUdpSocket::readyRead,
                     this, &PeerChannelManager::receiveNetworkDatagrams);
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

// 关闭旧通道并绑定 UDP
bool PeerChannelManager::openNetwork(const PeerNetworkSettings& settings, QString* error)
{
    close();
    m_networkMode = true;
    m_network = settings;
    m_open = NetworkProtocol::open(&m_udp, settings.localIp, settings.localPort, error);
    return m_open;
}

// 关闭所有通道
void PeerChannelManager::close()
{
    SerialProtocol::close(&m_serial);
    NetworkProtocol::close(&m_udp);
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

// 经当前 UDP 写出完整数据报
bool PeerChannelManager::sendNetwork(const QByteArray& datagram, QString* error)
{
    if (!m_open || !m_networkMode) {
        if (error)
            *error = QStringLiteral("网口未打开");
        return false;
    }
    if (NetworkProtocol::write(&m_udp, m_network.destinationIp,
                               m_network.destinationPort, datagram))
        return true;
    if (error)
        *error = m_udp.errorString();
    return false;
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
