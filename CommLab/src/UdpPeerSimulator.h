#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QObject>
#include <QUdpSocket>
#include <QVector>

// 本机 UDP 模拟对端。
// 可向 CommHandler 发送报文，也可监听端口以验证库侧发送。
class UdpPeerSimulator : public QObject
{
    Q_OBJECT

public:
    explicit UdpPeerSimulator(QObject* parent = nullptr);

    // 绑定本机端口开始监听。
    bool startListen(quint16 port, const QHostAddress& addr = QHostAddress::LocalHost);
    void stop();

    // 向指定地址发送原始 UDP 报文。
    qint64 sendRaw(const QByteArray& packet,
                   const QHostAddress& host,
                   quint16 port);

    void clearReceived();
    int receivedCount() const;
    // 等待收到不少于 minCount 个报文；超时返回 false。
    bool waitReceived(int minCount, int timeoutMs);

    // 构造三思网口合法帧：三个 double，共 24 字节。
    static QByteArray makeSansiPacket(double force, double displacement, double strain);

signals:
    void datagramReceived(QByteArray payload, QHostAddress sender, quint16 senderPort);

private slots:
    void onReadyRead();

private:
    QUdpSocket m_socket;
    QVector<QByteArray> m_received;
};
