#pragma once

#include "ILegacyBackend.h"
#include "Result.h"
#include "SessionConfig.h"

#include <QObject>
#include <QString>
#include <QVector>

namespace ca {

/**
 * @brief Legacy Worker：驻留独立线程；在此线程创建/销毁后端（含 CommHandler）。
 * @thread 禁止在 UI 线程创建 CommHandler。
 */
class LegacyWorker : public QObject
{
    Q_OBJECT
public:
    explicit LegacyWorker(QObject* parent = nullptr);
    ~LegacyWorker() override;

public slots:
    void initialize(const LegacyConfig& config);
    void connectDevice();
    void disconnectDevice();
    void sendValues(const QVector<double>& values);
    void sendText(const QString& text);
    void shutdown();

signals:
    void initializeFinished(bool ok, const QString& code, const QString& message);
    void connectFinished(bool ok, const QString& code, const QString& message);
    void disconnectFinished(bool ok, const QString& code, const QString& message);
    void sendFinished(bool ok, const QString& code, const QString& message, const QUuid& requestId);
    void valuesReceived(const QVector<double>& values, int type);
    void controlEvent(int ctrlCmd, int viewId, int msg);
    void parameterEvent(int ctrlCmd, int viewId, int msg, const QVariantMap& extra);
    void backendError(const QString& code, const QString& message);
    void disconnected();
    void shutdownFinished();

    // 携带 requestId 的内部发送请求
    void sendValuesWithId(const QVector<double>& values, const QUuid& requestId);
    void sendTextWithId(const QString& text, const QUuid& requestId);

public slots:
    void onSendValuesWithId(const QVector<double>& values, const QUuid& requestId);
    void onSendTextWithId(const QString& text, const QUuid& requestId);

private:
    void bindBackendSignals();
    void destroyBackend();

    ILegacyBackend* backend_ = nullptr;
    LegacyConfig config_;
};

} // namespace ca