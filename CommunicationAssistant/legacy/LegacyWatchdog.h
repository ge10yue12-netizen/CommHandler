#pragma once

#include <QObject>
#include <QTimer>
#include <QUuid>

namespace ca {

/**
 * @brief Worker 外看门狗：超时仅标记 Unresponsive，不 terminate 线程、不向阻塞 Worker 排 close。
 * @thread 应用/UI 线程。
 */
class LegacyWatchdog : public QObject
{
    Q_OBJECT
public:
    explicit LegacyWatchdog(QObject* parent = nullptr);

    void setTimeoutMs(int ms);
    void arm(const QUuid& opId);
    void disarm(const QUuid& opId);
    void disarmAll();
    bool isArmed() const { return armed_; }

signals:
    void timedOut(const QUuid& opId);

private slots:
    void onTimeout();

private:
    QTimer timer_;
    QUuid currentOp_;
    bool armed_ = false;
    int timeoutMs_ = 5000;
};

} // namespace ca