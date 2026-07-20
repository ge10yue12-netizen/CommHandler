#pragma once

#include "Result.h"
#include "SessionConfig.h"
#include "Types.h"

#include <QByteArray>
#include <QObject>

namespace ca {

// 传输层状态；Open 表示底层 I/O 已就绪，Faulted 表示运行时错误待上层回收
enum class TransportState
{
    Closed = 0,
    Opening,
    Open,
    Closing,
    Faulted
};

/**
 * @brief 字节传输抽象：不含协议编解码，仅负责 open/close/send 与原始字节流。
 *
 * @thread 须在创建对象的线程调用；信号在同线程同步发出（M1 均在应用线程）。
 */
class ITransport : public QObject
{
    Q_OBJECT

public:
    explicit ITransport(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~ITransport() override = default;

    /**
     * @brief 按 TransportConfig 打开底层 I/O。
     * @thread 应用线程；Serial 路径可能短暂阻塞于 QSerialPort::open。
     * 返回失败时不保证 emit transportError（由 Session 映射为单次 ErrorEvent）。
     * 主要错误：invalid_kind、参数非法、设备占用、open 失败。
     */
    virtual Result open(const TransportConfig& config) = 0;

    /**
     * @brief 关闭传输；重复调用安全。
     * @thread 应用线程。
     */
    virtual void close() = 0;

    /**
     * @brief 写入原始字节。
     * @thread 应用线程；Serial 使用有界 waitForBytesWritten，超时恒为失败。
     * 返回成功表示字节已提交至 OS/驱动缓冲区，不等于对端已收到。
     */
    virtual Result send(const QByteArray& bytes) = 0;

    virtual TransportState state() const = 0;
    virtual TransportKind kind() const = 0;

signals:
    void bytesReceived(const QByteArray& bytes);
    void transportStateChanged(ca::TransportState state);
    void transportError(const ca::CommError& error);
};

} // namespace ca

Q_DECLARE_METATYPE(ca::TransportState)