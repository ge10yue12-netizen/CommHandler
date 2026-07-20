#pragma once

#include "CommRecord.h"
#include "Result.h"

#include <QAtomicInt>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QThread>
#include <QUuid>
#include <QWaitCondition>

namespace ca {

/**
 * @brief 单 Session 抓包写入器：有界队列 + 后台线程顺序追加 JSONL。
 * @thread enqueue 可从应用线程调用；实际写盘在内部工作线程。
 */
class CaptureWriter : public QObject
{
    Q_OBJECT

public:
    explicit CaptureWriter(QObject* parent = nullptr);
    ~CaptureWriter() override;

    /**
     * @brief 打开文件并写文件头，启动工作线程。
     * maxQueue 为有界队列容量；写失败通过 writeFailed 报告。
     */
    Result open(const QString& filePath, const QUuid& sessionId, int maxQueue = 1024);

    /** 入队一条记录；队列满返回 capture_queue_full，不阻塞。 */
    Result enqueue(const CommRecord& record);

    /** 停止接收、排空队列并关闭文件。 */
    void close();

    bool isOpen() const { return open_.load() != 0; }
    QString filePath() const;
    QUuid sessionId() const { return sessionId_; }
    int queuedCount() const;

signals:
    void writeFailed(const QString& code, const QString& message);

private:
    void workerMain();

    mutable QMutex mutex_;
    QWaitCondition cond_;
    QQueue<CommRecord> queue_;
    int maxQueue_ = 1024;
    QAtomicInt open_;
    QAtomicInt stopping_;
    QThread* thread_ = nullptr;
    QString filePath_;
    QUuid sessionId_;
    bool headerWritten_ = false;
};

} // namespace ca