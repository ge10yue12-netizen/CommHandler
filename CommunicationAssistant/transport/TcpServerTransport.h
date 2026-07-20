#pragma once

#include "ITransport.h"

#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

namespace ca {

/** 单个 TCP 客户端 Channel 快照（供 UI/Session 展示） */
struct TcpChannelInfo
{
    QString channelId;
    QString remoteAddress;
    quint16 remotePort = 0;
    qint64 connectedAtMs = 0;
    quint64 rxBytes = 0;
    quint64 txBytes = 0;
};

/**
 * @brief TCP 服务端传输：每客户端独立 Channel；send() 为广播。
 *
 * @thread 应用线程；禁止永久 wait。
 * [DECISION] M1 支持定向/广播/断开选中客户端；无 Codec。
 */
class TcpServerTransport : public ITransport
{
    Q_OBJECT

public:
    explicit TcpServerTransport(QObject* parent = nullptr);
    ~TcpServerTransport() override;

    Result open(const TransportConfig& config) override;
    void close() override;
    // 广播到全部 Channel
    Result send(const QByteArray& bytes) override;
    TransportState state() const override { return state_; }
    TransportKind kind() const override { return TransportKind::TcpServer; }

    Result sendToChannel(const QString& channelId, const QByteArray& bytes);
    Result disconnectChannel(const QString& channelId);
    QVector<TcpChannelInfo> channels() const;

signals:
    void channelBytesReceived(const QString& channelId, const QByteArray& bytes);
    void channelsChanged();

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onClientError(QAbstractSocket::SocketError socketError);

private:
    struct Channel
    {
        QString channelId;
        QTcpSocket* socket = nullptr;
        QString remoteAddress;
        quint16 remotePort = 0;
        qint64 connectedAtMs = 0;
        quint64 rxBytes = 0;
        quint64 txBytes = 0;
    };

    void setState(TransportState state);
    void removeChannel(QTcpSocket* socket, bool emitChanged);
    QString makeChannelId(QTcpSocket* socket) const;
    Result writeSocket(QTcpSocket* socket, const QByteArray& bytes, Channel* stats);

    QTcpServer server_;
    QHash<QTcpSocket*, Channel> channels_;
    TransportState state_ = TransportState::Closed;
};

} // namespace ca