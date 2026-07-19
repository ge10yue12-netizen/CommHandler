// PeerChannelManager.h — MachinePeer 串口/网口初始化、切换与收发统一入口
#pragma once

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QUdpSocket>

// 试验机串口初始化参数
struct PeerSerialSettings
{
    int portIndex = 0;
    int baudIndex = 5;
    int dataBitsIndex = 3;
    int parityIndex = 0;
    int stopBitsIndex = 0;
};

// 试验机网口初始化参数
struct PeerNetworkSettings
{
    QString localIp;
    int localPort = 0;
    QString destinationIp;
    int destinationPort = 0;
};

// 保证试验机同一时刻只打开串口或网口
class PeerChannelManager : public QObject
{
    Q_OBJECT

public:
    // 创建串口与 UDP 对象并挂接统一接收槽
    explicit PeerChannelManager(QObject* parent = nullptr);

    // 关闭旧通道并打开串口
    bool openSerial(const PeerSerialSettings& settings, QString* error);

    // 关闭旧通道并绑定 UDP
    bool openNetwork(const PeerNetworkSettings& settings, QString* error);

    // 关闭所有通道
    void close();

    // 经当前串口写出完整帧并返回 HEX
    bool sendSerial(const QByteArray& frame, QString* hex, QString* error);

    // 经当前 UDP 写出完整数据报
    bool sendNetwork(const QByteArray& datagram, QString* error);

    // 返回当前是否已打开
    bool isOpen() const { return m_open; }

signals:
    // 串口收到一批原始字节
    void serialBytesReceived(QByteArray bytes);

    // UDP 收到一个完整数据报
    void networkDatagramReceived(QByteArray datagram);

private slots:
    // 读取串口当前全部可用字节
    void receiveSerialBytes();

    // 逐个读取 UDP 完整数据报
    void receiveNetworkDatagrams();

private:
    QSerialPort m_serial;
    QUdpSocket m_udp;
    PeerNetworkSettings m_network;
    bool m_open = false;
    bool m_networkMode = false;
};
