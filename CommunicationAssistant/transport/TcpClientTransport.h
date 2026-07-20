#pragma once

#include "ITransport.h"

#include <QTcpSocket>

namespace ca {

/**
 * @brief TCP 客户端字节传输（M1）；不解析协议帧。
 *
 * @thread 须在应用线程创建与调用；readyRead/error 同线程回调。
 * open 使用有界 waitForConnected，禁止永久等待。
 */
class TcpClientTransport : public ITransport
{
    Q_OBJECT

public:
    explicit TcpClientTransport(QObject* parent = nullptr);
    ~TcpClientTransport() override;

    Result open(const TransportConfig& config) override;
    void close() override;
    Result send(const QByteArray& bytes) override;
    TransportState state() const override { return state_; }
    TransportKind kind() const override { return TransportKind::TcpClient; }

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onDisconnected();

private:
    void setState(TransportState state);

    QTcpSocket socket_;
    TransportState state_ = TransportState::Closed;
};

} // namespace ca