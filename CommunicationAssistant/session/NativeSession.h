#pragma once

#include "ICommSession.h"
#include "SerialTransport.h"
#include "SessionManager.h"
#include "TcpClientTransport.h"
#include "TcpServerTransport.h"
#include "UdpTransport.h"

namespace ca {

/**
 * @brief Native Tool：Serial / TcpClient / TcpServer / Udp（同时刻一种）。
 * @thread UI/应用线程。
 */
class NativeSession : public ICommSession
{
    Q_OBJECT

public:
    explicit NativeSession(SessionManager* manager, QObject* parent = nullptr);
    ~NativeSession() override;

    Result configure(const SessionConfig& config) override;
    Result open() override;
    Result close() override;
    Result send(const SendRequest& request) override;

    SessionState state() const override { return state_; }
    QUuid sessionId() const override { return config_.sessionId; }
    const ResourceClaim& claim() const { return claim_; }
    bool isConfigured() const { return configured_; }
    TransportKind activeTransportKind() const { return activeKind_; }

    QVector<TcpChannelInfo> tcpChannels() const;
    Result disconnectTcpChannel(const QString& channelId);

signals:
    void channelsChanged();

private slots:
    void onBytesReceived(const QByteArray& bytes);
    void onChannelBytesReceived(const QString& channelId, const QByteArray& bytes);
    void onDatagramReceived(const QByteArray& bytes, const QString& peerAddress, quint16 peerPort);
    void onTransportError(const ca::CommError& error);
    void onTransportStateChanged(ca::TransportState transportState);
    void onServerChannelsChanged();

private:
    ITransport* activeTransport();
    void closeActiveTransport();
    void setSessionState(SessionState state);
    void emitErrorEvent(const CommError& error);
    void emitClosedEvent();
    void tearDownAfterFault(const CommError* errorOrNull);
    CommRecord makeTxRecord(const SendRequest& request, RecordStatus status);
    QString endpointSummary() const;

    SessionManager* manager_ = nullptr;
    SerialTransport serial_;
    TcpClientTransport tcpClient_;
    TcpServerTransport tcpServer_;
    UdpTransport udp_;
    TransportKind activeKind_ = TransportKind::Serial;
    SessionConfig config_;
    ResourceClaim claim_;
    SessionState state_ = SessionState::Created;
    bool configured_ = false;
    bool tearingDown_ = false;
    quint64 sequence_ = 0;
};

} // namespace ca