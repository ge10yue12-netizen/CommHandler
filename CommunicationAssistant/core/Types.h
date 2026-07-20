#pragma once

#include <QString>
#include <QtGlobal>

namespace ca {

enum class TransportKind
{
    Serial = 0,
    TcpClient,
    TcpServer,
    Udp
};

enum class WorkMode
{
    Native = 0,
    LegacyDll,
    Simulator
};

enum class SessionRole
{
    Tool = 0,
    DeviceSimulator
};

enum class Direction
{
    Tx = 0,
    Rx,
    System
};

// 会话状态机；UI 通过 stateChanged 观察，不得自行推断中间态
enum class SessionState
{
    Created = 0,
    Opening,
    Connected,
    Closing,
    Closed,
    Reconnecting,
    Unresponsive,
    Faulted
};

enum class RecordKind
{
    RawChunk = 0,
    UdpDatagram,
    ProtocolFrame,
    LegacyValueEvent,
    LegacyControlEvent,
    LegacyParameterEvent,
    ConnectionEvent,
    ErrorEvent
};

// [DECISION] 与开发基线 RecordStatus 一致；Submitted 不等于远端 ACK
enum class RecordStatus
{
    Observed = 0,
    Queued,
    Submitted,
    Completed,
    Failed,
    Cancelled
};

// 日志与 UI 展示用英文枚举名，避免翻译导致测试 Golden 漂移
inline QString recordKindName(RecordKind kind)
{
    switch (kind) {
    case RecordKind::RawChunk: return QStringLiteral("RawChunk");
    case RecordKind::UdpDatagram: return QStringLiteral("UdpDatagram");
    case RecordKind::ProtocolFrame: return QStringLiteral("ProtocolFrame");
    case RecordKind::LegacyValueEvent: return QStringLiteral("LegacyValueEvent");
    case RecordKind::LegacyControlEvent: return QStringLiteral("LegacyControlEvent");
    case RecordKind::LegacyParameterEvent: return QStringLiteral("LegacyParameterEvent");
    case RecordKind::ConnectionEvent: return QStringLiteral("ConnectionEvent");
    case RecordKind::ErrorEvent: return QStringLiteral("ErrorEvent");
    }
    return QStringLiteral("UnknownKind");
}

inline QString recordStatusName(RecordStatus status)
{
    switch (status) {
    case RecordStatus::Observed: return QStringLiteral("Observed");
    case RecordStatus::Queued: return QStringLiteral("Queued");
    case RecordStatus::Submitted: return QStringLiteral("Submitted");
    case RecordStatus::Completed: return QStringLiteral("Completed");
    case RecordStatus::Failed: return QStringLiteral("Failed");
    case RecordStatus::Cancelled: return QStringLiteral("Cancelled");
    }
    return QStringLiteral("UnknownStatus");
}

} // namespace ca