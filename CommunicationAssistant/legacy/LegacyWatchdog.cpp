#include "LegacyWatchdog.h"

namespace ca {

LegacyWatchdog::LegacyWatchdog(QObject* parent)
    : QObject(parent)
{
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &LegacyWatchdog::onTimeout);
}

void LegacyWatchdog::setTimeoutMs(int ms)
{
    timeoutMs_ = qMax(100, ms);
}

void LegacyWatchdog::arm(const QUuid& opId)
{
    currentOp_ = opId;
    armed_ = true;
    timer_.start(timeoutMs_);
}

void LegacyWatchdog::disarm(const QUuid& opId)
{
    if (!armed_)
        return;
    if (!opId.isNull() && opId != currentOp_)
        return;
    armed_ = false;
    timer_.stop();
    currentOp_ = QUuid();
}

void LegacyWatchdog::disarmAll()
{
    armed_ = false;
    timer_.stop();
    currentOp_ = QUuid();
}

void LegacyWatchdog::onTimeout()
{
    if (!armed_)
        return;
    armed_ = false;
    const QUuid id = currentOp_;
    currentOp_ = QUuid();
    emit timedOut(id);
}

} // namespace ca