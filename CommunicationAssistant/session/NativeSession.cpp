#include "NativeSession.h"

#include "ClaimFor.h"

#include <QDateTime>
#include <QElapsedTimer>

namespace ca {

namespace {
qint64 monotonicNsNow()
{
    static QElapsedTimer timer;
    if (!timer.isValid())
        timer.start();
    return timer.nsecsElapsed();
}
} // namespace

NativeSession::NativeSession(SessionManager* manager, QObject* parent)
    : ICommSession(parent)
    , manager_(manager)
{
    connect(&serial_, &SerialTransport::bytesReceived, this, &NativeSession::onBytesReceived);
    connect(&serial_, &SerialTransport::transportError, this, &NativeSession::onTransportError);
    connect(&serial_, &SerialTransport::transportStateChanged, this, &NativeSession::onTransportStateChanged);

    connect(&tcpClient_, &TcpClientTransport::bytesReceived, this, &NativeSession::onBytesReceived);
    connect(&tcpClient_, &TcpClientTransport::transportError, this, &NativeSession::onTransportError);
    connect(&tcpClient_, &TcpClientTransport::transportStateChanged, this, &NativeSession::onTransportStateChanged);

    connect(&tcpServer_, &TcpServerTransport::channelBytesReceived, this, &NativeSession::onChannelBytesReceived);
    connect(&tcpServer_, &TcpServerTransport::transportError, this, &NativeSession::onTransportError);
    connect(&tcpServer_, &TcpServerTransport::transportStateChanged, this, &NativeSession::onTransportStateChanged);
    connect(&tcpServer_, &TcpServerTransport::channelsChanged, this, &NativeSession::onServerChannelsChanged);

    connect(&udp_, &UdpTransport::datagramReceived, this, &NativeSession::onDatagramReceived);
    connect(&udp_, &UdpTransport::transportError, this, &NativeSession::onTransportError);
    connect(&udp_, &UdpTransport::transportStateChanged, this, &NativeSession::onTransportStateChanged);
}

NativeSession::~NativeSession()
{
    close();
}

ITransport* NativeSession::activeTransport()
{
    switch (activeKind_) {
    case TransportKind::TcpClient: return &tcpClient_;
    case TransportKind::TcpServer: return &tcpServer_;
    case TransportKind::Udp: return &udp_;
    case TransportKind::Serial:
    default: return &serial_;
    }
}

void NativeSession::closeActiveTransport()
{
    serial_.close();
    tcpClient_.close();
    tcpServer_.close();
    udp_.close();
}

Result NativeSession::configure(const SessionConfig& config)
{
    if (state_ != SessionState::Created && state_ != SessionState::Closed)
        return Result::fail(QStringLiteral("invalid_state"), QStringLiteral("仅允许在 Created/Closed 下配置"));
    if (config.mode != WorkMode::Native)
        return Result::fail(QStringLiteral("unsupported_mode"), QStringLiteral("NativeSession 仅支持 Native 模式"));
    if (config.role != SessionRole::Tool)
        return Result::fail(QStringLiteral("unsupported_role"), QStringLiteral("M1 角色必须为 Tool"));
    if (config.sessionId.isNull())
        return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));

    const TransportKind k = config.transport.kind;
    if (k != TransportKind::Serial && k != TransportKind::TcpClient && k != TransportKind::TcpServer
        && k != TransportKind::Udp)
        return Result::fail(QStringLiteral("unsupported_transport"),
                            QStringLiteral("当前仅支持串口、TCP 客户端/服务端或 UDP"));

    ResourceClaim claim;
    const Result claimResult = claimFor(config, &claim);
    if (!claimResult.ok)
        return claimResult;

    config_ = config;
    claim_ = claim;
    activeKind_ = k;
    configured_ = true;
    if (state_ == SessionState::Closed)
        setSessionState(SessionState::Created);
    return Result::success();
}

Result NativeSession::open()
{
    if (!configured_)
        return Result::fail(QStringLiteral("not_configured"), QStringLiteral("请先调用 configure"));
    if (state_ == SessionState::Connected || state_ == SessionState::Opening)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("会话已在打开或已连接"));
    if (!manager_)
        return Result::fail(QStringLiteral("no_manager"), QStringLiteral("缺少 SessionManager"));

    const Result acquired = manager_->acquire(config_.sessionId, claim_);
    if (!acquired.ok)
        return acquired;

    setSessionState(SessionState::Opening);
    const Result opened = activeTransport()->open(config_.transport);
    if (!opened.ok) {
        manager_->release(config_.sessionId);
        CommError err;
        err.code = opened.code;
        err.message = opened.message;
        err.sessionId = config_.sessionId.toString(QUuid::WithoutBraces);
        emitErrorEvent(err);
        setSessionState(SessionState::Closed);
        return opened;
    }

    setSessionState(SessionState::Connected);
    CommRecord conn;
    conn.sessionId = config_.sessionId;
    conn.sequence = ++sequence_;
    conn.kind = RecordKind::ConnectionEvent;
    conn.status = RecordStatus::Observed;
    conn.direction = Direction::System;
    conn.wallTime = QDateTime::currentDateTimeUtc();
    conn.monotonicNs = monotonicNsNow();
    // 按传输就绪语义写摘要（协议解析与是否连通无关）
    if (activeKind_ == TransportKind::TcpServer)
        conn.summary = QStringLiteral("已监听 %1").arg(endpointSummary());
    else if (activeKind_ == TransportKind::Udp)
        conn.summary = QStringLiteral("已绑定 %1").arg(endpointSummary());
    else if (activeKind_ == TransportKind::Serial)
        conn.summary = QStringLiteral("串口已打开 %1").arg(endpointSummary());
    else
        conn.summary = QStringLiteral("已连接 %1").arg(endpointSummary());
    conn.localEndpoint.address = endpointSummary();
    emit recordReceived(conn);
    return Result::success();
}

Result NativeSession::close()
{
    if (state_ == SessionState::Closed || state_ == SessionState::Created)
        return Result::success();
    if (tearingDown_)
        return Result::success();

    tearingDown_ = true;
    setSessionState(SessionState::Closing);
    closeActiveTransport();
    if (manager_)
        manager_->release(config_.sessionId);
    setSessionState(SessionState::Closed);
    emitClosedEvent();
    tearingDown_ = false;
    return Result::success();
}

Result NativeSession::send(const SendRequest& request)
{
    if (state_ != SessionState::Connected)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("会话未连接"));
    if (request.requestId.isNull())
        return Result::fail(QStringLiteral("invalid_request_id"), QStringLiteral("缺少 requestId"));
    // UDP 允许空 Datagram；其它传输仍拒绝空载荷
    if (request.payload.isEmpty() && activeKind_ != TransportKind::Udp)
        return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("载荷为空"));

    emit recordReceived(makeTxRecord(request, RecordStatus::Queued));

    Result submitted;
    if (activeKind_ == TransportKind::TcpServer) {
        if (request.broadcast || request.channelId.isEmpty())
            submitted = tcpServer_.send(request.payload);
        else
            submitted = tcpServer_.sendToChannel(request.channelId, request.payload);
    } else {
        submitted = activeTransport()->send(request.payload);
    }

    if (!submitted.ok) {
        CommRecord failed = makeTxRecord(request, RecordStatus::Failed);
        failed.errorCode = submitted.code;
        failed.errorMessage = submitted.message;
        emit recordReceived(failed);
        return submitted;
    }

    emit recordReceived(makeTxRecord(request, RecordStatus::Submitted));
    return Result::success();
}

QVector<TcpChannelInfo> NativeSession::tcpChannels() const
{
    if (activeKind_ != TransportKind::TcpServer)
        return QVector<TcpChannelInfo>();
    return tcpServer_.channels();
}

Result NativeSession::disconnectTcpChannel(const QString& channelId)
{
    if (activeKind_ != TransportKind::TcpServer || state_ != SessionState::Connected)
        return Result::fail(QStringLiteral("invalid_state"), QStringLiteral("仅 TCP 服务端已连接时可断开客户端"));
    return tcpServer_.disconnectChannel(channelId);
}

QString NativeSession::endpointSummary() const
{
    if (activeKind_ == TransportKind::TcpClient)
        return QStringLiteral("%1:%2").arg(claim_.remoteAddress).arg(claim_.remotePort);
    if (activeKind_ == TransportKind::TcpServer || activeKind_ == TransportKind::Udp) {
        const QString a = claim_.localAddress.isEmpty() ? QStringLiteral("0.0.0.0") : claim_.localAddress;
        return QStringLiteral("%1:%2").arg(a).arg(claim_.localPort);
    }
    return claim_.serialPort;
}

void NativeSession::onBytesReceived(const QByteArray& bytes)
{
    // 服务端以 channelBytesReceived 为准，忽略汇总 bytesReceived，避免双记
    if (activeKind_ == TransportKind::TcpServer)
        return;

    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::RawChunk;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = bytes;
    rec.summary = QStringLiteral("接收 %1 字节").arg(bytes.size());
    rec.localEndpoint.address = endpointSummary();
    emit recordReceived(rec);
}

void NativeSession::onChannelBytesReceived(const QString& channelId, const QByteArray& bytes)
{
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.channelId = channelId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::RawChunk;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = bytes;
    rec.summary = QStringLiteral("接收 %1 字节（通道 %2）").arg(bytes.size()).arg(channelId);
    rec.localEndpoint.address = endpointSummary();
    emit recordReceived(rec);
}

void NativeSession::onDatagramReceived(const QByteArray& bytes, const QString& peerAddress, quint16 peerPort)
{
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::UdpDatagram;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = bytes;
    rec.summary = QStringLiteral("UDP 收包 %1 字节 来自 %2:%3")
                      .arg(bytes.size())
                      .arg(peerAddress)
                      .arg(peerPort);
    rec.localEndpoint.address = claim_.localAddress.isEmpty() ? QStringLiteral("0.0.0.0") : claim_.localAddress;
    rec.localEndpoint.port = claim_.localPort;
    rec.remoteEndpoint.address = peerAddress;
    rec.remoteEndpoint.port = peerPort;
    emit recordReceived(rec);
}

void NativeSession::onTransportError(const ca::CommError& error)
{
    CommError err = error;
    err.sessionId = config_.sessionId.toString(QUuid::WithoutBraces);
    tearDownAfterFault(&err);
}

void NativeSession::onTransportStateChanged(ca::TransportState transportState)
{
    if (tearingDown_)
        return;
    if (state_ != SessionState::Connected)
        return;
    if (transportState != TransportState::Closed)
        return;

    CommError err;
    err.code = QStringLiteral("transport_closed");
    err.message = QStringLiteral("传输意外关闭");
    err.sessionId = config_.sessionId.toString(QUuid::WithoutBraces);
    tearDownAfterFault(&err);
}

void NativeSession::onServerChannelsChanged()
{
    emit channelsChanged();
}

void NativeSession::setSessionState(SessionState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit stateChanged(state_);
}

void NativeSession::emitErrorEvent(const CommError& error)
{
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::ErrorEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::System;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.errorCode = error.code;
    rec.errorMessage = error.message;
    rec.summary = error.message;
    rec.requestId = error.requestId;
    emit recordReceived(rec);
    emit errorOccurred(error);
}

void NativeSession::emitClosedEvent()
{
    CommRecord conn;
    conn.sessionId = config_.sessionId;
    conn.sequence = ++sequence_;
    conn.kind = RecordKind::ConnectionEvent;
    conn.status = RecordStatus::Observed;
    conn.direction = Direction::System;
    conn.wallTime = QDateTime::currentDateTimeUtc();
    conn.monotonicNs = monotonicNsNow();
    conn.summary = QStringLiteral("已断开连接");
    emit recordReceived(conn);
}

void NativeSession::tearDownAfterFault(const CommError* errorOrNull)
{
    if (tearingDown_)
        return;
    if (state_ == SessionState::Closed || state_ == SessionState::Created || state_ == SessionState::Closing)
        return;

    tearingDown_ = true;
    if (errorOrNull)
        emitErrorEvent(*errorOrNull);

    setSessionState(SessionState::Faulted);
    closeActiveTransport();
    if (manager_)
        manager_->release(config_.sessionId);
    setSessionState(SessionState::Closed);
    emitClosedEvent();
    tearingDown_ = false;
}

CommRecord NativeSession::makeTxRecord(const SendRequest& request, RecordStatus status)
{
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.channelId = request.channelId;
    rec.sequence = ++sequence_;
    rec.requestId = request.requestId;
    rec.taskId = request.taskId;
    rec.kind = (activeKind_ == TransportKind::Udp) ? RecordKind::UdpDatagram : RecordKind::RawChunk;
    rec.status = status;
    rec.direction = Direction::Tx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = request.payload;
    if (request.broadcast || (activeKind_ == TransportKind::TcpServer && request.channelId.isEmpty()))
        rec.summary = QStringLiteral("广播发送 %1 字节（%2）")
                          .arg(request.payload.size())
                          .arg(recordStatusDisplayName(status));
    else if (activeKind_ == TransportKind::Udp)
        rec.summary = QStringLiteral("UDP 发包 %1 字节（%2）")
                          .arg(request.payload.size())
                          .arg(recordStatusDisplayName(status));
    else
        rec.summary = QStringLiteral("发送 %1 字节（%2）")
                          .arg(request.payload.size())
                          .arg(recordStatusDisplayName(status));
    rec.localEndpoint.address = endpointSummary();
    if (activeKind_ == TransportKind::Udp) {
        rec.localEndpoint.address = claim_.localAddress.isEmpty() ? QStringLiteral("0.0.0.0") : claim_.localAddress;
        rec.localEndpoint.port = claim_.localPort;
        rec.remoteEndpoint.address = claim_.remoteAddress;
        rec.remoteEndpoint.port = claim_.remotePort;
    }
    rec.attributes = request.attributes;
    return rec;
}

} // namespace ca