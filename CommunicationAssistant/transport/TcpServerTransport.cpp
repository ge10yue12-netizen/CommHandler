#include "TcpServerTransport.h"

#include <QDateTime>
#include <QHostAddress>

namespace ca {

TcpServerTransport::TcpServerTransport(QObject* parent)
    : ITransport(parent)
{
    connect(&server_, &QTcpServer::newConnection, this, &TcpServerTransport::onNewConnection);
}

TcpServerTransport::~TcpServerTransport()
{
    close();
}

Result TcpServerTransport::open(const TransportConfig& config)
{
    if (config.kind != TransportKind::TcpServer)
        return Result::fail(QStringLiteral("invalid_kind"), QStringLiteral("TcpServerTransport 仅支持 TCP 服务端"));
    if (state_ == TransportState::Open || state_ == TransportState::Opening)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("服务端已在监听"));

    if (server_.isListening())
        server_.close();

    QString addr = config.tcpServer.localAddress.trimmed();
    if (addr.isEmpty())
        addr = QStringLiteral("0.0.0.0");
    const quint16 port = config.tcpServer.localPort;
    if (port == 0)
        return Result::fail(QStringLiteral("invalid_tcp_server"), QStringLiteral("需要本地端口"));

    setState(TransportState::Opening);
    const QHostAddress host(addr);
    if (host.isNull() && addr != QStringLiteral("0.0.0.0")) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_address"), QStringLiteral("监听地址无效"));
    }

    if (!server_.listen(host, port) || !server_.isListening()) {
        const QString msg = server_.errorString();
        server_.close();
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("tcp_listen_failed"), msg.isEmpty() ? QStringLiteral("监听失败") : msg);
    }

    setState(TransportState::Open);
    return Result::success();
}

void TcpServerTransport::close()
{
    if (state_ == TransportState::Closed && !server_.isListening() && channels_.isEmpty())
        return;
    setState(TransportState::Closing);

    const auto socks = channels_.keys();
    for (QTcpSocket* s : socks) {
        if (!s)
            continue;
        s->disconnect(this);
        s->abort();
        s->deleteLater();
    }
    channels_.clear();
    emit channelsChanged();

    server_.close();
    setState(TransportState::Closed);
}

Result TcpServerTransport::send(const QByteArray& bytes)
{
    if (state_ != TransportState::Open)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("服务端未监听"));
    if (bytes.isEmpty())
        return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("载荷为空"));
    if (channels_.isEmpty())
        return Result::fail(QStringLiteral("no_clients"), QStringLiteral("当前无客户端连接"));

    Result lastFail = Result::success();
    int okCount = 0;
    for (auto it = channels_.begin(); it != channels_.end(); ++it) {
        const Result r = writeSocket(it.key(), bytes, &it.value());
        if (r.ok)
            ++okCount;
        else
            lastFail = r;
    }
    if (okCount == 0)
        return lastFail;
    return Result::success();
}

Result TcpServerTransport::sendToChannel(const QString& channelId, const QByteArray& bytes)
{
    if (state_ != TransportState::Open)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("服务端未监听"));
    if (bytes.isEmpty())
        return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("载荷为空"));
    if (channelId.isEmpty())
        return Result::fail(QStringLiteral("invalid_channel"), QStringLiteral("缺少 channelId"));

    for (auto it = channels_.begin(); it != channels_.end(); ++it) {
        if (it.value().channelId == channelId)
            return writeSocket(it.key(), bytes, &it.value());
    }
    return Result::fail(QStringLiteral("channel_not_found"), QStringLiteral("客户端通道不存在"));
}

Result TcpServerTransport::disconnectChannel(const QString& channelId)
{
    for (auto it = channels_.begin(); it != channels_.end(); ++it) {
        if (it.value().channelId != channelId)
            continue;
        QTcpSocket* s = it.key();
        removeChannel(s, true);
        if (s) {
            s->disconnect(this);
            s->abort();
            s->deleteLater();
        }
        return Result::success();
    }
    return Result::fail(QStringLiteral("channel_not_found"), QStringLiteral("客户端通道不存在"));
}

QVector<TcpChannelInfo> TcpServerTransport::channels() const
{
    QVector<TcpChannelInfo> out;
    out.reserve(channels_.size());
    for (auto it = channels_.constBegin(); it != channels_.constEnd(); ++it) {
        TcpChannelInfo info;
        info.channelId = it.value().channelId;
        info.remoteAddress = it.value().remoteAddress;
        info.remotePort = it.value().remotePort;
        info.connectedAtMs = it.value().connectedAtMs;
        info.rxBytes = it.value().rxBytes;
        info.txBytes = it.value().txBytes;
        out.push_back(info);
    }
    return out;
}

void TcpServerTransport::onNewConnection()
{
    while (server_.hasPendingConnections()) {
        QTcpSocket* socket = server_.nextPendingConnection();
        if (!socket)
            continue;
        Channel ch;
        ch.channelId = makeChannelId(socket);
        ch.socket = socket;
        ch.remoteAddress = socket->peerAddress().toString();
        ch.remotePort = socket->peerPort();
        ch.connectedAtMs = QDateTime::currentMSecsSinceEpoch();
        channels_.insert(socket, ch);

        connect(socket, &QTcpSocket::readyRead, this, &TcpServerTransport::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &TcpServerTransport::onClientDisconnected);
        connect(socket,
                static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                this,
                &TcpServerTransport::onClientError);
    }
    emit channelsChanged();
}

void TcpServerTransport::onClientReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !channels_.contains(socket))
        return;
    const QByteArray chunk = socket->readAll();
    if (chunk.isEmpty())
        return;
    Channel& ch = channels_[socket];
    ch.rxBytes += static_cast<quint64>(chunk.size());
    emit channelBytesReceived(ch.channelId, chunk);
    // 兼容 ITransport：无 channel 的会话也可收到汇总信号（本路径 Session 主要用 channelBytesReceived）
    emit bytesReceived(chunk);
}

void TcpServerTransport::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    removeChannel(socket, true);
    socket->deleteLater();
}

void TcpServerTransport::onClientError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        return;
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    // 单客户端错误不拖垮监听；仅移除该 Channel
    removeChannel(socket, true);
    socket->abort();
    socket->deleteLater();
}

void TcpServerTransport::setState(TransportState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit transportStateChanged(state_);
}

void TcpServerTransport::removeChannel(QTcpSocket* socket, bool emitChanged)
{
    if (!socket || !channels_.contains(socket))
        return;
    channels_.remove(socket);
    if (emitChanged)
        emit channelsChanged();
}

QString TcpServerTransport::makeChannelId(QTcpSocket* socket) const
{
    // 稳定可读：远端 endpoint + 描述符
    return QStringLiteral("%1:%2#%3")
        .arg(socket->peerAddress().toString())
        .arg(socket->peerPort())
        .arg(static_cast<qintptr>(socket->socketDescriptor()));
}

Result TcpServerTransport::writeSocket(QTcpSocket* socket, const QByteArray& bytes, Channel* stats)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("客户端未连接"));
    const qint64 written = socket->write(bytes);
    if (written < 0)
        return Result::fail(QStringLiteral("tcp_write_failed"), socket->errorString());
    if (!socket->waitForBytesWritten(1000)) {
        return Result::fail(QStringLiteral("tcp_write_timeout"),
                            socket->errorString().isEmpty() ? QStringLiteral("等待写入完成超时")
                                                            : socket->errorString());
    }
    if (written != bytes.size())
        return Result::fail(QStringLiteral("tcp_short_write"),
                            QStringLiteral("短写：已写 %1 / 共 %2").arg(written).arg(bytes.size()));
    if (stats)
        stats->txBytes += static_cast<quint64>(written);
    return Result::success();
}

} // namespace ca