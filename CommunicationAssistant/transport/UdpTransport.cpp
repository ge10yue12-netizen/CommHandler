#include "UdpTransport.h"

namespace ca {

UdpTransport::UdpTransport(QObject* parent)
    : ITransport(parent)
{
    connect(&socket_, &QUdpSocket::readyRead, this, &UdpTransport::onReadyRead);
}

UdpTransport::~UdpTransport()
{
    close();
}

Result UdpTransport::open(const TransportConfig& config)
{
    if (config.kind != TransportKind::Udp)
        return Result::fail(QStringLiteral("invalid_kind"), QStringLiteral("UdpTransport 仅支持 UDP"));
    if (state_ == TransportState::Open || state_ == TransportState::Opening)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("UDP 已绑定"));

    if (socket_.state() != QAbstractSocket::UnconnectedState)
        socket_.close();

    QString addr = config.udp.localAddress.trimmed();
    if (addr.isEmpty())
        addr = QStringLiteral("0.0.0.0");
    const quint16 port = config.udp.localPort;
    if (port == 0)
        return Result::fail(QStringLiteral("invalid_udp"), QStringLiteral("需要本地端口"));

    setState(TransportState::Opening);

    QUdpSocket::BindMode mode = QUdpSocket::DefaultForPlatform;
    if (config.udp.addressReuse)
        mode |= QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint;

    const QHostAddress host(addr);
    if (!socket_.bind(host, port, mode)
        || socket_.state() != QAbstractSocket::BoundState) {
        const QString msg = socket_.errorString();
        socket_.close();
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("udp_bind_failed"),
                            msg.isEmpty() ? QStringLiteral("UDP 绑定失败") : msg);
    }

    remoteHost_ = QHostAddress();
    remotePort_ = 0;
    hasRemote_ = false;
    const QString remote = config.udp.remoteAddress.trimmed();
    if (!remote.isEmpty() && config.udp.remotePort != 0) {
        remoteHost_ = QHostAddress(remote);
        if (remoteHost_.isNull()) {
            socket_.close();
            setState(TransportState::Closed);
            return Result::fail(QStringLiteral("invalid_remote"), QStringLiteral("远端地址无效"));
        }
        remotePort_ = config.udp.remotePort;
        hasRemote_ = true;
    }

    setState(TransportState::Open);
    return Result::success();
}

void UdpTransport::close()
{
    if (state_ == TransportState::Closed && socket_.state() == QAbstractSocket::UnconnectedState)
        return;
    setState(TransportState::Closing);
    socket_.close();
    hasRemote_ = false;
    setState(TransportState::Closed);
}

Result UdpTransport::send(const QByteArray& bytes)
{
    if (state_ != TransportState::Open)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("UDP 未绑定"));
    if (!hasRemote_)
        return Result::fail(QStringLiteral("no_remote"), QStringLiteral("未配置远端地址/端口，无法发送"));
    // 允许空包：基线要求覆盖空 Datagram；空包仍 writeDatagram
    const qint64 written = socket_.writeDatagram(bytes, remoteHost_, remotePort_);
    if (written < 0)
        return Result::fail(QStringLiteral("udp_write_failed"), socket_.errorString());
    if (written != bytes.size())
        return Result::fail(QStringLiteral("udp_short_write"),
                            QStringLiteral("短写：已写 %1 / 共 %2").arg(written).arg(bytes.size()));
    return Result::success();
}

void UdpTransport::onReadyRead()
{
    while (socket_.hasPendingDatagrams()) {
        QByteArray buffer;
        buffer.resize(static_cast<int>(socket_.pendingDatagramSize()));
        QHostAddress from;
        quint16 fromPort = 0;
        const qint64 n = socket_.readDatagram(buffer.data(), buffer.size(), &from, &fromPort);
        if (n < 0)
            continue;
        buffer.resize(static_cast<int>(n));
        emit datagramReceived(buffer, from.toString(), fromPort);
        // 兼容层：不经 bytesReceived，避免 Session 记成 RawChunk
    }
}

void UdpTransport::setState(TransportState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit transportStateChanged(state_);
}

} // namespace ca