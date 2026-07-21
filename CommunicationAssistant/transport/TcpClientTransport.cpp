#include "TcpClientTransport.h"

namespace ca {

TcpClientTransport::TcpClientTransport(QObject* parent)
    : ITransport(parent)
{
    connect(&socket_, &QTcpSocket::readyRead, this, &TcpClientTransport::onReadyRead);
    connect(&socket_,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this,
            &TcpClientTransport::onSocketError);
    connect(&socket_, &QTcpSocket::disconnected, this, &TcpClientTransport::onDisconnected);
}

TcpClientTransport::~TcpClientTransport()
{
    close();
}

// 连接失败仅返回 Result，不 emit transportError，由 Session 统一 ErrorEvent
Result TcpClientTransport::open(const TransportConfig& config)
{
    if (config.kind != TransportKind::TcpClient)
        return Result::fail(QStringLiteral("invalid_kind"), QStringLiteral("TcpClientTransport 仅支持 TCP 客户端"));
    if (state_ == TransportState::Open || state_ == TransportState::Opening)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("TCP 已连接或正在连接"));

    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.abort();

    const QString host = config.tcpClient.remoteAddress.trimmed();
    const quint16 port = config.tcpClient.remotePort;
    if (host.isEmpty() || port == 0) {
        return Result::fail(QStringLiteral("invalid_tcp_client"), QStringLiteral("需要远端地址与端口"));
    }

    setState(TransportState::Opening);
    socket_.connectToHost(host, port);
    // [DECISION] 有界等待 5s，禁止 waitForConnected(-1)
    if (!socket_.waitForConnected(5000)
        || socket_.state() != QAbstractSocket::ConnectedState) {
        const QString msg = socket_.errorString();
        socket_.abort();
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("tcp_connect_failed"),
                            msg.isEmpty() ? QStringLiteral("连接超时或对端拒绝") : msg);
    }

    setState(TransportState::Open);
    return Result::success();
}

void TcpClientTransport::close()
{
    if (state_ == TransportState::Closed
        && socket_.state() == QAbstractSocket::UnconnectedState)
        return;
    setState(TransportState::Closing);
    socket_.disconnectFromHost();
    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.waitForDisconnected(1000);
    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.abort();
    setState(TransportState::Closed);
}

Result TcpClientTransport::send(const QByteArray& bytes)
{
    if (state_ != TransportState::Open || socket_.state() != QAbstractSocket::ConnectedState)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("TCP 未连接"));
    if (bytes.isEmpty())
        return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("载荷为空"));

    const qint64 written = socket_.write(bytes);
    if (written < 0)
        return Result::fail(QStringLiteral("tcp_write_failed"), socket_.errorString());
    if (!socket_.waitForBytesWritten(1000)) {
        const QString msg = (socket_.error() != QAbstractSocket::UnknownSocketError
                             && socket_.error() != QAbstractSocket::SocketTimeoutError)
                                ? socket_.errorString()
                                : QStringLiteral("等待写入完成超时");
        return Result::fail(QStringLiteral("tcp_write_timeout"), msg);
    }
    if (written != bytes.size())
        return Result::fail(QStringLiteral("tcp_short_write"),
                            QStringLiteral("短写：已写 %1 / 共 %2").arg(written).arg(bytes.size()));
    return Result::success();
}

void TcpClientTransport::onReadyRead()
{
    const QByteArray chunk = socket_.readAll();
    if (!chunk.isEmpty())
        emit bytesReceived(chunk);
}

void TcpClientTransport::onSocketError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        return; // 由 disconnected 统一收尾
    if (state_ == TransportState::Closed || state_ == TransportState::Closing || state_ == TransportState::Opening)
        return;
    CommError err;
    err.code = QStringLiteral("tcp_error");
    err.message = socket_.errorString();
    emit transportError(err);
    if (state_ != TransportState::Closed && state_ != TransportState::Closing) {
        setState(TransportState::Faulted);
        close();
    }
}

void TcpClientTransport::onDisconnected()
{
    if (state_ == TransportState::Closed || state_ == TransportState::Closing || state_ == TransportState::Opening)
        return;
    CommError err;
    err.code = QStringLiteral("tcp_disconnected");
    err.message = QStringLiteral("对端已断开连接");
    emit transportError(err);
    setState(TransportState::Faulted);
    close();
}

void TcpClientTransport::setState(TransportState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit transportStateChanged(state_);
}

} // namespace ca