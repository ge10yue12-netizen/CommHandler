#include "SerialTransport.h"

#include "ResourceClaim.h"

namespace ca {

// 成员 port_ 无 QObject 父对象，由本类生命周期管理
SerialTransport::SerialTransport(QObject* parent)
    : ITransport(parent)
{
    connect(&port_, &QSerialPort::readyRead, this, &SerialTransport::onReadyRead);
    connect(&port_, &QSerialPort::errorOccurred, this, &SerialTransport::onSerialError);
}

SerialTransport::~SerialTransport()
{
    close();
}

// open 失败仅返回 Result，不 emit transportError，避免 Session 重复 ErrorEvent
Result SerialTransport::open(const TransportConfig& config)
{
    if (config.kind != TransportKind::Serial)
        return Result::fail(QStringLiteral("invalid_kind"), QStringLiteral("SerialTransport 仅支持 Serial 类型"));
    if (state_ == TransportState::Open || state_ == TransportState::Opening)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("串口已打开"));

    if (port_.isOpen())
        port_.close();
    port_.clearError();

    setState(TransportState::Opening);

    const QString portName = normalizeSerialPortName(config.serial.portName);
    if (portName.isEmpty()) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_serial"), QStringLiteral("串口名为空"));
    }

    port_.setPortName(portName);
    if (!port_.setBaudRate(config.serial.baudRate)) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_baud"), port_.errorString());
    }
    const QSerialPort::DataBits dataBits = toDataBits(config.serial.dataBits);
    const QSerialPort::Parity parity = toParity(config.serial.parity);
    const QSerialPort::StopBits stopBits = toStopBits(config.serial.stopBits);
    if (dataBits == QSerialPort::UnknownDataBits) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_data_bits"), QStringLiteral("不支持的数据位"));
    }
    if (parity == QSerialPort::UnknownParity) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_parity"), QStringLiteral("不支持的校验位"));
    }
    if (stopBits == QSerialPort::UnknownStopBits) {
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("invalid_stop_bits"), QStringLiteral("不支持的停止位"));
    }
    port_.setDataBits(dataBits);
    port_.setParity(parity);
    port_.setStopBits(stopBits);
    port_.setFlowControl(QSerialPort::NoFlowControl);

    if (!port_.open(QIODevice::ReadWrite)) {
        const QString msg = port_.errorString();
        port_.clearError();
        setState(TransportState::Closed);
        return Result::fail(QStringLiteral("serial_open_failed"), msg);
    }

    setState(TransportState::Open);
    return Result::success();
}

void SerialTransport::close()
{
    if (state_ == TransportState::Closed && !port_.isOpen())
        return;
    setState(TransportState::Closing);
    if (port_.isOpen())
        port_.close();
    port_.clearError();
    setState(TransportState::Closed);
}

// [DECISION] waitForBytesWritten 固定 1000ms；超时或短写恒为失败，禁止无限等待
Result SerialTransport::send(const QByteArray& bytes)
{
    if (state_ != TransportState::Open || !port_.isOpen())
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("串口未打开"));
    if (bytes.isEmpty())
        return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("载荷为空"));

    const qint64 written = port_.write(bytes);
    if (written < 0)
        return Result::fail(QStringLiteral("serial_write_failed"), port_.errorString());

    if (!port_.waitForBytesWritten(1000)) {
        const QString msg = (port_.error() != QSerialPort::NoError)
                                ? port_.errorString()
                                : QStringLiteral("等待写入完成超时");
        return Result::fail(QStringLiteral("serial_write_timeout"), msg);
    }
    if (written != bytes.size())
        return Result::fail(QStringLiteral("serial_short_write"),
                            QStringLiteral("短写：已写 %1 / 共 %2").arg(written).arg(bytes.size()));
    return Result::success();
}

void SerialTransport::onReadyRead()
{
    const QByteArray chunk = port_.readAll();
    if (!chunk.isEmpty())
        emit bytesReceived(chunk);
}

// 运行时故障：先 transportError 再 Faulted+close，保证 Session tearDown 看到错误码
void SerialTransport::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    if (state_ == TransportState::Closed || state_ == TransportState::Closing || state_ == TransportState::Opening)
        return;

    CommError err;
    err.code = QStringLiteral("serial_error");
    err.message = port_.errorString();
    emit transportError(err);
    if (state_ != TransportState::Closed && state_ != TransportState::Closing) {
        setState(TransportState::Faulted);
        close();
    }
}

void SerialTransport::setState(TransportState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit transportStateChanged(state_);
}

QSerialPort::DataBits SerialTransport::toDataBits(int bits)
{
    switch (bits) {
    case 5: return QSerialPort::Data5;
    case 6: return QSerialPort::Data6;
    case 7: return QSerialPort::Data7;
    case 8: return QSerialPort::Data8;
    default: return QSerialPort::UnknownDataBits;
    }
}

QSerialPort::Parity SerialTransport::toParity(const QString& parity)
{
    const QString p = parity.trimmed().toLower();
    if (p == QStringLiteral("even") || p == QStringLiteral("e"))
        return QSerialPort::EvenParity;
    if (p == QStringLiteral("odd") || p == QStringLiteral("o"))
        return QSerialPort::OddParity;
    if (p == QStringLiteral("space") || p == QStringLiteral("s"))
        return QSerialPort::SpaceParity;
    if (p == QStringLiteral("mark") || p == QStringLiteral("m"))
        return QSerialPort::MarkParity;
    if (p == QStringLiteral("none") || p == QStringLiteral("n") || p.isEmpty())
        return QSerialPort::NoParity;
    return QSerialPort::UnknownParity;
}

QSerialPort::StopBits SerialTransport::toStopBits(const QString& stopBits)
{
    const QString s = stopBits.trimmed();
    if (s == QStringLiteral("1.5"))
        return QSerialPort::OneAndHalfStop;
    if (s == QStringLiteral("2"))
        return QSerialPort::TwoStop;
    if (s == QStringLiteral("1") || s.isEmpty())
        return QSerialPort::OneStop;
    return QSerialPort::UnknownStopBits;
}

} // namespace ca