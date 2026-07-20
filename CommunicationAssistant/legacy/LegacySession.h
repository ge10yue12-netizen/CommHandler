#pragma once

#include "ICommSession.h"
#include "LegacyCapability.h"
#include "LegacyWatchdog.h"
#include "LegacyWorker.h"
#include "SessionManager.h"

#include <QThread>
#include <QUuid>
#include <QVector>

namespace ca {

/**
 * @brief Legacy DLL 会话：UI 仅依赖 ICommSession；CommHandler 仅存在于 Worker 线程。
 * @thread 虚函数在应用线程调用；经 QueuedConnection 投递 Worker。
 */
class LegacySession : public ICommSession
{
    Q_OBJECT
public:
    explicit LegacySession(SessionManager* manager, QObject* parent = nullptr);
    ~LegacySession() override;

    Result configure(const SessionConfig& config) override;
    Result open() override;
    Result close() override;
    Result send(const SendRequest& request) override;

    SessionState state() const override { return state_; }
    QUuid sessionId() const override { return config_.sessionId; }

    const LegacyCapabilityProfile& capabilityProfile() const { return profile_; }
    bool useMockBackend() const { return config_.transport.legacy.useMockBackend; }

    // 测试访问：注入 Mock 事件（仅 Mock 后端）
    LegacyWorker* workerForTest() { return worker_; }

signals:
    void unresponsive(const QString& message);

private slots:
    void onInitializeFinished(bool ok, const QString& code, const QString& message);
    void onConnectFinished(bool ok, const QString& code, const QString& message);
    void onDisconnectFinished(bool ok, const QString& code, const QString& message);
    void onSendFinished(bool ok, const QString& code, const QString& message, const QUuid& requestId);
    void onValuesReceived(const QVector<double>& values, int type);
    void onControlEvent(int ctrlCmd, int viewId, int msg);
    void onParameterEvent(int ctrlCmd, int viewId, int msg, const QVariantMap& extra);
    void onBackendError(const QString& code, const QString& message);
    void onBackendDisconnected();
    void onWatchdogTimeout(const QUuid& opId);
    void onWorkerShutdownFinished();

private:
    void setSessionState(SessionState state);
    void emitError(const QString& code, const QString& message, const QUuid& requestId = QUuid());
    void emitTxRecord(const SendRequest& request, RecordStatus status, const QString& code = QString(),
                      const QString& message = QString());
    void tearDown(bool fromFault);
    Result validateSendAgainstCapability(const SendRequest& request, QVector<double>* outValues,
                                         QString* outText, bool* isText) const;

    SessionManager* manager_ = nullptr;
    SessionConfig config_;
    LegacyCapabilityProfile profile_;
    SessionState state_ = SessionState::Created;
    bool configured_ = false;
    bool tearingDown_ = false;
    bool pendingClose_ = false;
    bool cancelOpen_ = false; // Opening 期间 close：禁止继续 connectDevice
    quint64 sequence_ = 0;

    QThread workerThread_;
    LegacyWorker* worker_ = nullptr;
    LegacyWatchdog watchdog_;
    QUuid pendingOpenOp_;
    QUuid pendingSendOp_;
    SendRequest pendingSendRequest_;
};

} // namespace ca