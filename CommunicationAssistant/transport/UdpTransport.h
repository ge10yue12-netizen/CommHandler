#pragma once

#include "ITransport.h"

#include <QHostAddress>
#include <QUdpSocket>

namespace ca {

/**
 * @brief UDP 传输：保持 Datagram 边界；接收携带源端点。
 * @thread 应用线程；禁止永久 wait。
 */
class UdpTransport : public ITransport
{
    Q_OBJECT

public:
    explicit UdpTransport(QObject* parent = nullptr);
    ~UdpTransport() override;

    Result open(const TransportConfig& config) override;
    void close() override;
    Result send(const QByteArray& bytes) override;
    TransportState state() const override { return state_; }
    TransportKind kind() const override { return TransportKind::Udp; }

signals:
    // 单次 readDatagram 对应一包；peer 为源端点
    void datagramReceived(const QByteArray& bytes, const QString& peerAddress, quint16 peerPort);

private slots:
    void onReadyRead();

private:
    void setState(TransportState state);

    QUdpSocket socket_;
    TransportState state_ = TransportState::Closed;
    QHostAddress remoteHost_;
    quint16 remotePort_ = 0;
    bool hasRemote_ = false;
};

} // namespace ca