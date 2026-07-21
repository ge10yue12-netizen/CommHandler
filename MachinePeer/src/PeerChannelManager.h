// PeerChannelManager.h — MachinePeer 串口/网口初始化、切换与收发统一入口
#pragma once

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
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

// 试验机网口初始化参数（transferType/model 对齐 CommHandler iTransferType/iModel）
struct PeerNetworkSettings
{
    QString localIp;
    int localPort = 9000;
    QString destinationIp;
    int destinationPort = 9000;
    // 0=TCP 1=UDP
    int transferType = 0;
    // 0=服务端 1=客户端（仅 TCP 有效；UDP 忽略）
    int model = 0;
};

// 保证试验机同一时刻只打开串口或网口（TCP/UDP）
class PeerChannelManager : public QObject
{
    Q_OBJECT

public:
    // 创建串口/UDP/TCP 对象并挂接统一接收槽
    explicit PeerChannelManager(QObject* parent = nullptr);

    // 关闭旧通道并打开串口
    bool openSerial(const PeerSerialSettings& settings, QString* error);

    // 关闭旧通道并打开网口（TCP 服务端/客户端或 UDP 绑定）
    bool openNetwork(const PeerNetworkSettings& settings, QString* error);

    // 关闭所有通道
    void close();

    // 经当前串口写出完整帧并返回 HEX
    bool sendSerial(const QByteArray& frame, QString* hex, QString* error);

    // 经当前网口写出（TCP 已连接套接字或 UDP 目标）
    bool sendNetwork(const QByteArray& datagram, QString* error);

    // 返回当前是否已打开（TCP 服务端在等待客户端时亦为 true）
    bool isOpen() const { return m_open; }

    // 返回当前网口是否已具备可写对端（UDP 恒为 true；TCP 需已连接）
    bool isNetworkPeerReady() const;

signals:
    // 串口收到一批原始字节
    void serialBytesReceived(QByteArray bytes);

    // 网口收到一批原始字节（UDP 数据报或 TCP 流片段）
    void networkDatagramReceived(QByteArray datagram);

    // TCP 对端已连接（服务端 accept 或客户端 Connected）
    void networkPeerConnected(QString endpoint);

    // TCP 对端已断开
    void networkPeerDisconnected();

private slots:
    // 读取串口当前全部可用字节
    void receiveSerialBytes();

    // 逐个读取 UDP 完整数据报
    void receiveNetworkDatagrams();

    // 读取当前 TCP 套接字可用字节
    void receiveTcpBytes();

    // TCP 服务端接受新连接
    void onTcpNewConnection();

    // TCP 套接字断开
    void onTcpDisconnected();

private:
    // 释放已接受的 TCP 客户端套接字
    void clearAcceptedSocket();

    // 返回当前用于收发的 TCP 套接字；无则 nullptr
    QTcpSocket* activeTcpSocket() const;

    QSerialPort m_serial;
    QUdpSocket m_udp;
    QTcpServer m_tcpServer;
    QTcpSocket m_tcpClient;
    QTcpSocket* m_tcpAccepted = nullptr;
    PeerNetworkSettings m_network;
    bool m_open = false;
    bool m_networkMode = false;
};
