#include "SendScheduler.h"

namespace ca {

SendScheduler::SendScheduler(QObject* parent)
    : QObject(parent)
{
}

SendScheduler::~SendScheduler()
{
    stopAll();
    unbindSessionSignals();
}

void SendScheduler::setSession(ICommSession* session)
{
    if (session_ == session)
        return;
    stopAll();
    unbindSessionSignals();
    session_ = session;
    bindSessionSignals();
}

void SendScheduler::bindSessionSignals()
{
    if (!session_)
        return;
    connect(session_, &ICommSession::recordReceived, this, &SendScheduler::onRecordReceived);
    connect(session_, &ICommSession::stateChanged, this, &SendScheduler::onSessionStateChanged);
}

void SendScheduler::unbindSessionSignals()
{
    if (!session_)
        return;
    disconnect(session_, nullptr, this, nullptr);
}

bool SendScheduler::isPaused(const QUuid& taskId) const
{
    const auto it = tasks_.constFind(taskId);
    if (it == tasks_.constEnd())
        return false;
    return it->phase == Phase::Paused;
}

Result SendScheduler::startTask(const ScheduleTaskSpec& spec)
{
    if (!session_)
        return Result::fail(QStringLiteral("no_session"), QStringLiteral("未绑定会话"));
    if (spec.payloads.isEmpty())
        return Result::fail(QStringLiteral("invalid_payloads"), QStringLiteral("载荷列表为空"));
    if (spec.mode != ScheduleMode::Once && spec.intervalMs < 0)
        return Result::fail(QStringLiteral("invalid_interval"), QStringLiteral("间隔非法"));
    if (spec.mode == ScheduleMode::Counted && spec.maxCount <= 0)
        return Result::fail(QStringLiteral("invalid_count"), QStringLiteral("指定次数须大于 0"));
    if (spec.mode == ScheduleMode::RoundRobin && spec.payloads.size() < 1)
        return Result::fail(QStringLiteral("invalid_payloads"), QStringLiteral("轮询载荷为空"));

    ScheduleTaskSpec normalized = spec;
    if (normalized.taskId.isNull())
        normalized.taskId = QUuid::createUuid();
    if (tasks_.contains(normalized.taskId))
        return Result::fail(QStringLiteral("duplicate_task"), QStringLiteral("任务已存在"));

    if (normalized.mode == ScheduleMode::Once)
        normalized.maxCount = 1;

    TaskRuntime runtime;
    runtime.spec = normalized;
    runtime.phase = Phase::Idle;
    runtime.timer = new QTimer(this);
    runtime.timer->setSingleShot(true);
    const QUuid taskId = normalized.taskId;
    connect(runtime.timer, &QTimer::timeout, this, [this, taskId]() { onIntervalTimeout(taskId); });

    tasks_.insert(taskId, runtime);
    emit taskStarted(taskId);
    fireNextSend(&tasks_[taskId]);
    return Result::success();
}

Result SendScheduler::pauseTask(const QUuid& taskId)
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end())
        return Result::fail(QStringLiteral("unknown_task"), QStringLiteral("任务不存在"));
    if (it->phase == Phase::Paused)
        return Result::success();

    if (it->timer)
        it->timer->stop();

    it->phase = Phase::Paused;
    return Result::success();
}

Result SendScheduler::resumeTask(const QUuid& taskId)
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end())
        return Result::fail(QStringLiteral("unknown_task"), QStringLiteral("任务不存在"));
    if (it->phase != Phase::Paused)
        return Result::fail(QStringLiteral("not_paused"), QStringLiteral("任务未暂停"));

    // 暂停期间已收到 Submitted：继续走间隔；仍有 pending：回到等待；否则立即发送
    if (it->resumePending) {
        it->resumePending = false;
        scheduleInterval(&(*it));
        return Result::success();
    }
    if (!it->pendingRequestId.isNull()) {
        it->phase = Phase::AwaitingSubmitted;
        return Result::success();
    }
    it->phase = Phase::Idle;
    fireNextSend(&(*it));
    return Result::success();
}

Result SendScheduler::stopTask(const QUuid& taskId)
{
    if (!tasks_.contains(taskId))
        return Result::fail(QStringLiteral("unknown_task"), QStringLiteral("任务不存在"));
    finishTask(taskId, QStringLiteral("stopped"), false);
    return Result::success();
}

void SendScheduler::stopAll()
{
    const QList<QUuid> ids = tasks_.keys();
    for (const QUuid& id : ids)
        finishTask(id, QStringLiteral("stopped_all"), false);
}

QByteArray SendScheduler::currentPayload(const TaskRuntime& task) const
{
    if (task.spec.payloads.isEmpty())
        return QByteArray();
    if (task.spec.mode == ScheduleMode::RoundRobin) {
        const int idx = task.payloadIndex % task.spec.payloads.size();
        return task.spec.payloads.at(idx);
    }
    return task.spec.payloads.first();
}

bool SendScheduler::shouldContinueAfterSubmitted(const TaskRuntime& task) const
{
    switch (task.spec.mode) {
    case ScheduleMode::Once:
        return false;
    case ScheduleMode::Counted:
        return task.completedSends < task.spec.maxCount;
    case ScheduleMode::Infinite:
        return true;
    case ScheduleMode::RoundRobin:
        if (task.spec.maxCount > 0)
            return task.completedSends < task.spec.maxCount;
        return true;
    }
    return false;
}

void SendScheduler::fireNextSend(TaskRuntime* task)
{
    if (!task || !session_)
        return;
    if (task->phase == Phase::Paused)
        return;

    SendRequest req;
    req.requestId = QUuid::createUuid();
    req.taskId = task->spec.taskId;
    req.sessionId = session_->sessionId();
    req.channelId = task->spec.channelId;
    req.broadcast = task->spec.broadcast;
    req.payload = currentPayload(*task);
    if (task->spec.payloadAttributes.size() == task->spec.payloads.size()
        && task->payloadIndex >= 0 && task->payloadIndex < task->spec.payloadAttributes.size()) {
        req.attributes = task->spec.payloadAttributes.at(task->payloadIndex);
    }

    task->pendingRequestId = req.requestId;
    task->phase = Phase::AwaitingSubmitted;

    const Result submitted = session_->send(req);
    if (!submitted.ok) {
        const QUuid id = task->spec.taskId;
        finishTask(id, QStringLiteral("send_rejected"), true, submitted.code, submitted.message);
        return;
    }
    // 同线程 DirectConnection 下 Submitted 可能已在 send 内到达；若仍等待则保持 AwaitingSubmitted
}

void SendScheduler::scheduleInterval(TaskRuntime* task)
{
    if (!task || !task->timer)
        return;
    task->phase = Phase::WaitingInterval;
    int ms = qMax(0, task->spec.intervalMs);
    // payloadIndex 已指向下一条；间隔取「刚发完」那一行
    if (task->spec.payloadIntervals.size() == task->spec.payloads.size()
        && !task->spec.payloads.isEmpty()) {
        int prev = task->payloadIndex - 1;
        if (task->spec.mode == ScheduleMode::RoundRobin) {
            if (prev < 0)
                prev = task->spec.payloads.size() - 1;
        } else {
            prev = qBound(0, prev, task->spec.payloads.size() - 1);
        }
        if (prev >= 0 && prev < task->spec.payloadIntervals.size()) {
            const int rowMs = task->spec.payloadIntervals.at(prev);
            if (rowMs >= 0)
                ms = rowMs;
        }
    }
    task->timer->start(ms);
}

void SendScheduler::onIntervalTimeout(const QUuid& taskId)
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end())
        return;
    if (it->phase == Phase::Paused)
        return;
    if (it->phase != Phase::WaitingInterval)
        return;
    fireNextSend(&(*it));
}

void SendScheduler::onRecordReceived(const ca::CommRecord& record)
{
    if (record.requestId.isNull() || record.taskId.isNull())
        return;
    if (record.direction != Direction::Tx)
        return;

    auto it = tasks_.find(record.taskId);
    if (it == tasks_.end())
        return;
    if (record.requestId != it->pendingRequestId)
        return;

    if (record.status == RecordStatus::Failed) {
        const QUuid id = it->spec.taskId;
        finishTask(id, QStringLiteral("failed"), true, record.errorCode, record.errorMessage);
        return;
    }

    if (record.status != RecordStatus::Submitted)
        return;

    it->pendingRequestId = QUuid();
    ++it->completedSends;
    if (it->spec.mode == ScheduleMode::RoundRobin)
        it->payloadIndex = (it->payloadIndex + 1) % qMax(1, it->spec.payloads.size());

    if (!shouldContinueAfterSubmitted(*it)) {
        const QUuid id = it->spec.taskId;
        finishTask(id, QStringLiteral("completed"), false);
        return;
    }

    if (it->phase == Phase::Paused) {
        it->resumePending = true;
        return;
    }

    scheduleInterval(&(*it));
}

void SendScheduler::onSessionStateChanged(ca::SessionState state)
{
    if (state == SessionState::Connected)
        return;
    // 会话离开 Connected：停止全部调度任务
    if (state == SessionState::Closing || state == SessionState::Closed || state == SessionState::Faulted
        || state == SessionState::Created) {
        stopAll();
    }
}

void SendScheduler::finishTask(const QUuid& taskId, const QString& reason, bool failed,
                               const QString& code, const QString& message)
{
    auto it = tasks_.find(taskId);
    if (it == tasks_.end())
        return;

    if (it->timer) {
        it->timer->stop();
        it->timer->deleteLater();
        it->timer = nullptr;
    }
    tasks_.remove(taskId);

    if (failed)
        emit taskFailed(taskId, code, message);
    emit taskStopped(taskId, reason);
}

} // namespace ca