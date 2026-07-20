#pragma once

#include "ICommSession.h"
#include "Result.h"

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QUuid>
#include <QVector>

namespace ca {

/** 调度模式：固定延迟模型（M1）。 */
enum class ScheduleMode
{
    Once,       // 发送一次后结束
    Counted,    // 固定次数
    Infinite,   // 无限周期
    RoundRobin  // 多载荷顺序轮询（可配次数或无限）
};

/**
 * @brief 调度任务规格；payloads 至少一项（RoundRobin 可多项）。
 * @thread 由 UI/应用线程构造后交给 SendScheduler。
 */
struct ScheduleTaskSpec
{
    QUuid taskId; // 空则由 Scheduler 分配
    ScheduleMode mode = ScheduleMode::Once;
    QVector<QByteArray> payloads;
    int intervalMs = 1000; // Submitted 之后到下一次发送的固定延迟
    int maxCount = 0;      // Counted/RoundRobin：>0 为次数；Once 忽略；Infinite 忽略
    QString channelId;
    bool broadcast = false;
};

/**
 * @brief 应用层发送调度：仅调用 ICommSession::send()，不持有 Transport。
 *
 * [DECISION] 周期锚点为 RecordStatus::Submitted；Failed 默认停止该任务且不重试。
 * 每任务一个 singleShot 定时器；不为每任务建线程。
 *
 * @thread 与 session 同线程（通常为 UI/应用线程）。
 */
class SendScheduler : public QObject
{
    Q_OBJECT

public:
    explicit SendScheduler(QObject* parent = nullptr);
    ~SendScheduler() override;

    /** 绑定会话（非拥有）；关闭或切换会话前应 stopAll。 */
    void setSession(ICommSession* session);

    ICommSession* session() const { return session_; }

    /**
     * @brief 启动任务。
     * 返回成功表示任务已登记并开始首次发送尝试。
     * 主要错误：no_session、invalid_payloads、invalid_interval、duplicate_task。
     */
    Result startTask(const ScheduleTaskSpec& spec);

    Result pauseTask(const QUuid& taskId);
    Result resumeTask(const QUuid& taskId);
    Result stopTask(const QUuid& taskId);
    void stopAll();

    int activeTaskCount() const { return tasks_.size(); }
    bool hasTask(const QUuid& taskId) const { return tasks_.contains(taskId); }
    bool isPaused(const QUuid& taskId) const;

signals:
    void taskStarted(const QUuid& taskId);
    void taskStopped(const QUuid& taskId, const QString& reason);
    void taskFailed(const QUuid& taskId, const QString& code, const QString& message);

private slots:
    void onRecordReceived(const ca::CommRecord& record);
    void onSessionStateChanged(ca::SessionState state);
    void onIntervalTimeout(const QUuid& taskId);

private:
    enum class Phase
    {
        Idle,
        AwaitingSubmitted,
        WaitingInterval,
        Paused
    };

    struct TaskRuntime
    {
        ScheduleTaskSpec spec;
        Phase phase = Phase::Idle;
        int completedSends = 0;
        int payloadIndex = 0;
        QUuid pendingRequestId;
        QTimer* timer = nullptr;
        bool resumePending = false;
    };

    void bindSessionSignals();
    void unbindSessionSignals();
    void fireNextSend(TaskRuntime* task);
    void scheduleInterval(TaskRuntime* task);
    void finishTask(const QUuid& taskId, const QString& reason, bool failed,
                    const QString& code = QString(), const QString& message = QString());
    bool shouldContinueAfterSubmitted(const TaskRuntime& task) const;
    QByteArray currentPayload(const TaskRuntime& task) const;

    ICommSession* session_ = nullptr;
    QHash<QUuid, TaskRuntime> tasks_;
};

} // namespace ca