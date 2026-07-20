#pragma once

#include "ITransport.h"

#include <QSerialPort>

namespace ca {

/**
 * @brief QSerialPort 字节传输实现（M1 Serial 路径）。
 *
 * @thread 须在应用线程创建与调用；readyRead/errorOccurred 在同线程回调。
 */
class SerialTransport : public ITransport
{
    Q_OBJECT

public:
    explicit SerialTransport(QObject* parent = nullptr);
    ~SerialTransport() override;

    Result open(const TransportConfig& config) override;
    void close() override;
    Result send(const QByteArray& bytes) override;
    TransportState state() const override { return state_; }
    TransportKind kind() const override { return TransportKind::Serial; }

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    void setState(TransportState state);
    static QSerialPort::DataBits toDataBits(int bits);
    static QSerialPort::Parity toParity(const QString& parity);
    static QSerialPort::StopBits toStopBits(const QString& stopBits);

    QSerialPort port_;
    TransportState state_ = TransportState::Closed;
};

} // namespace ca