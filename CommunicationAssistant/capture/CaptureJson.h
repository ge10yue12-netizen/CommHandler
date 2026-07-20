#pragma once

#include "CommRecord.h"
#include "Types.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QUuid>
#include <QJsonObject>
#include <QVariantMap>

namespace ca {

/** Direction 稳定英文名（JSONL / Golden）。 */
inline QString directionName(Direction direction)
{
    switch (direction) {
    case Direction::Tx: return QStringLiteral("Tx");
    case Direction::Rx: return QStringLiteral("Rx");
    case Direction::System: return QStringLiteral("System");
    }
    return QStringLiteral("UnknownDirection");
}

/** UUID → 小写、无花括号；空 UUID → 空串。 */
inline QString uuidToCaptureString(const QUuid& id)
{
    if (id.isNull())
        return QString();
    return id.toString(QUuid::WithoutBraces).toLower();
}

/** 64 位整数 → 十进制字符串，避免 JSON 双精度丢精度。 */
inline QString int64ToDecimalString(qint64 value)
{
    return QString::number(value);
}

inline QString uint64ToDecimalString(quint64 value)
{
    return QString::number(value);
}

/**
 * @brief 序列化文件头（JSON Lines 首行）。
 * @thread 任意；纯函数。
 */
inline QByteArray serializeCaptureFileHeader(const QUuid& sessionId, const QDateTime& createdAtUtc)
{
    QJsonObject o;
    o.insert(QStringLiteral("recordType"), QStringLiteral("fileHeader"));
    o.insert(QStringLiteral("formatVersion"), QStringLiteral("1"));
    o.insert(QStringLiteral("sessionId"), uuidToCaptureString(sessionId));
    o.insert(QStringLiteral("createdAtUtc"),
             createdAtUtc.toUTC().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzZ")));
    o.insert(QStringLiteral("magic"), QStringLiteral("CAREC01"));
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

/**
 * @brief 序列化一条 CommRecord 为 JSONL 行（不含换行符）。
 * attributes 仅含非保留键；端点与 summary 放入 attributes。
 */
inline QByteArray serializeCaptureRecord(const CommRecord& record)
{
    QJsonObject o;
    o.insert(QStringLiteral("recordType"), QStringLiteral("record"));
    o.insert(QStringLiteral("formatVersion"), QStringLiteral("1"));
    o.insert(QStringLiteral("sessionId"), uuidToCaptureString(record.sessionId));
    o.insert(QStringLiteral("sequence"), uint64ToDecimalString(record.sequence));
    o.insert(QStringLiteral("kind"), recordKindName(record.kind));
    o.insert(QStringLiteral("direction"), directionName(record.direction));
    o.insert(QStringLiteral("status"), recordStatusName(record.status));
    o.insert(QStringLiteral("wallTimeUtc"),
             record.wallTime.toUTC().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzZ")));
    o.insert(QStringLiteral("monotonicNs"), int64ToDecimalString(record.monotonicNs));
    o.insert(QStringLiteral("channelId"), record.channelId);
    o.insert(QStringLiteral("requestId"), uuidToCaptureString(record.requestId));
    o.insert(QStringLiteral("taskId"), uuidToCaptureString(record.taskId));
    o.insert(QStringLiteral("bytesBase64"), QString::fromLatin1(record.bytes.toBase64()));
    o.insert(QStringLiteral("errorCode"), record.errorCode);
    o.insert(QStringLiteral("errorMessage"), record.errorMessage);

    QJsonObject attrs = QJsonObject::fromVariantMap(record.attributes);
    // 禁止 attributes 覆盖保留顶层字段名：剔除同名键
    static const char* reserved[] = {
        "recordType", "formatVersion", "sessionId", "sequence", "kind", "direction", "status",
        "wallTimeUtc", "monotonicNs", "channelId", "requestId", "taskId", "bytesBase64",
        "attributes", "errorCode", "errorMessage"};
    for (const char* key : reserved)
        attrs.remove(QString::fromLatin1(key));

    if (!record.summary.isEmpty())
        attrs.insert(QStringLiteral("summary"), record.summary);
    if (!record.localEndpoint.address.isEmpty() || record.localEndpoint.port != 0) {
        QJsonObject ep;
        ep.insert(QStringLiteral("address"), record.localEndpoint.address);
        ep.insert(QStringLiteral("port"), static_cast<int>(record.localEndpoint.port));
        attrs.insert(QStringLiteral("localEndpoint"), ep);
    }
    if (!record.remoteEndpoint.address.isEmpty() || record.remoteEndpoint.port != 0) {
        QJsonObject ep;
        ep.insert(QStringLiteral("address"), record.remoteEndpoint.address);
        ep.insert(QStringLiteral("port"), static_cast<int>(record.remoteEndpoint.port));
        attrs.insert(QStringLiteral("remoteEndpoint"), ep);
    }
    o.insert(QStringLiteral("attributes"), attrs);

    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

/** 从文件名或首行识别异常/非本格式文件。 */
inline bool isCaptureMagicOk(const QJsonObject& header)
{
    return header.value(QStringLiteral("magic")).toString() == QStringLiteral("CAREC01")
        && header.value(QStringLiteral("recordType")).toString() == QStringLiteral("fileHeader")
        && header.value(QStringLiteral("formatVersion")).toString() == QStringLiteral("1");
}

} // namespace ca