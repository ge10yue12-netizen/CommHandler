#pragma once

#include "Result.h"
#include "Types.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QVariantMap>

namespace ca {

// UI/Session 提交的发送请求；requestId 用于关联 Queued/Submitted/Failed 记录
struct SendRequest
{
    QUuid requestId;
    QUuid taskId;
    QUuid sessionId;
    QString channelId;
    QByteArray payload;
    bool broadcast = false;
    QVariantMap attributes;
};

/**
 * @brief 统一通信记录：RX/TX/连接/错误均通过此结构上报 UI 与 Capture。
 *
 * monotonicNs 为进程内单调时钟，不表示 UTC；wallTime 为 UTC 墙钟。
 * [DECISION] status 语义见开发基线 RecordStatus。
 */
struct CommRecord
{
    QUuid sessionId;
    QString channelId;
    quint64 sequence = 0;

    QUuid requestId;
    QUuid taskId;

    RecordKind kind = RecordKind::RawChunk;
    RecordStatus status = RecordStatus::Observed;
    Direction direction = Direction::Rx;

    QDateTime wallTime;
    qint64 monotonicNs = 0;

    QByteArray bytes;
    QVariantMap attributes;
    QString summary;

    Endpoint localEndpoint;
    Endpoint remoteEndpoint;

    QString errorCode;
    QString errorMessage;
};

} // namespace ca