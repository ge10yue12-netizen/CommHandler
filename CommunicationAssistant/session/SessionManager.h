#pragma once

#include "ResourceClaim.h"
#include "Result.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

namespace ca {

/**
 * @brief 进程内资源互斥：Serial；TCP Server / UDP 本地绑定；Legacy 单会话。
 * @thread 应用线程。
 * [DECISION] TCP 与 UDP 同端口号可并存（分属不同协议占用表项）。
 */
class SessionManager : public QObject
{
public:
    explicit SessionManager(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    Result acquire(const QUuid& sessionId, const ResourceClaim& claim)
    {
        if (sessionId.isNull())
            return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));

        if (claim.transport == TransportKind::Serial) {
            const QString key = claim.serialPort;
            if (key.isEmpty())
                return Result::fail(QStringLiteral("invalid_serial"), QStringLiteral("串口占用标识为空"));

            if (sessionSerial_.contains(sessionId)) {
                const QString oldKey = sessionSerial_.value(sessionId);
                if (oldKey != key && serialOwners_.value(oldKey) == sessionId)
                    serialOwners_.remove(oldKey);
            }

            if (serialOwners_.contains(key) && serialOwners_.value(key) != sessionId)
                return Result::fail(QStringLiteral("resource_busy"),
                                   QStringLiteral("串口 %1 已被占用").arg(key));
            serialOwners_.insert(key, sessionId);
            sessionSerial_.insert(sessionId, key);
            return Result::success();
        }

        if (claim.transport == TransportKind::TcpServer || claim.transport == TransportKind::Udp) {
            const QString addr = normalizeListenAddress(claim.localAddress);
            if (claim.localPort == 0) {
                return Result::fail(claim.transport == TransportKind::Udp
                                        ? QStringLiteral("invalid_udp")
                                        : QStringLiteral("invalid_tcp_server"),
                                    QStringLiteral("需要本地端口"));
            }

            clearSessionListen(sessionId);

            for (const ListenClaim& existing : listens_) {
                if (existing.sessionId == sessionId)
                    continue;
                if (existing.kind != claim.transport)
                    continue;
                if (listenConflicts(existing.address, existing.port, addr, claim.localPort)) {
                    return Result::fail(QStringLiteral("resource_busy"),
                                       QStringLiteral("绑定 %1:%2 与已有占用冲突")
                                           .arg(addr)
                                           .arg(claim.localPort));
                }
            }

            ListenClaim lc;
            lc.sessionId = sessionId;
            lc.kind = claim.transport;
            lc.address = addr;
            lc.port = claim.localPort;
            listens_.push_back(lc);
            sessionListen_.insert(sessionId, true);
            return Result::success();
        }

        // TcpClient：不因远端互斥
        return Result::success();
    }

    void release(const QUuid& sessionId)
    {
        if (sessionSerial_.contains(sessionId)) {
            const QString key = sessionSerial_.take(sessionId);
            if (serialOwners_.value(key) == sessionId)
                serialOwners_.remove(key);
        }
        clearSessionListen(sessionId);
        releaseLegacy(sessionId);
    }

    Result acquireLegacy(const QUuid& sessionId)
    {
        if (sessionId.isNull())
            return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));
        if (hasActiveLegacySession_ && activeLegacySessionId_ != sessionId)
            return Result::fail(QStringLiteral("legacy_busy"), QStringLiteral("已有活动 LegacySession"));
        hasActiveLegacySession_ = true;
        activeLegacySessionId_ = sessionId;
        return Result::success();
    }

    void releaseLegacy(const QUuid& sessionId)
    {
        if (!hasActiveLegacySession_)
            return;
        if (activeLegacySessionId_ != sessionId)
            return;
        hasActiveLegacySession_ = false;
        activeLegacySessionId_ = QUuid();
    }

    bool hasActiveLegacySession() const { return hasActiveLegacySession_; }

    bool isSerialClaimed(const QString& port) const { return serialOwners_.contains(port); }
    int serialClaimCount() const { return serialOwners_.size(); }
    int listenClaimCount() const { return listens_.size(); }

private:
    struct ListenClaim
    {
        QUuid sessionId;
        TransportKind kind = TransportKind::TcpServer;
        QString address;
        quint16 port = 0;
    };

    void clearSessionListen(const QUuid& sessionId)
    {
        if (!sessionListen_.contains(sessionId))
            return;
        sessionListen_.remove(sessionId);
        const int idx = indexOfListen(sessionId);
        if (idx >= 0)
            listens_.removeAt(idx);
    }

    static QString normalizeListenAddress(const QString& raw)
    {
        const QString a = raw.trimmed();
        if (a.isEmpty() || a == QStringLiteral("*") || a == QStringLiteral("::"))
            return QStringLiteral("0.0.0.0");
        return a.toLower();
    }

    static bool isWildcard(const QString& addr)
    {
        return addr == QStringLiteral("0.0.0.0") || addr == QStringLiteral("::")
            || addr == QStringLiteral("::0");
    }

    static bool listenConflicts(const QString& a1, quint16 p1, const QString& a2, quint16 p2)
    {
        if (p1 != p2)
            return false;
        if (isWildcard(a1) || isWildcard(a2))
            return true;
        return a1 == a2;
    }

    int indexOfListen(const QUuid& sessionId) const
    {
        for (int i = 0; i < listens_.size(); ++i) {
            if (listens_.at(i).sessionId == sessionId)
                return i;
        }
        return -1;
    }

    QHash<QString, QUuid> serialOwners_;
    QHash<QUuid, QString> sessionSerial_;
    QVector<ListenClaim> listens_;
    QHash<QUuid, bool> sessionListen_;
    bool hasActiveLegacySession_ = false;
    QUuid activeLegacySessionId_;
};

} // namespace ca