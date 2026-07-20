#include "CaptureWriter.h"

#include "CaptureJson.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>

namespace ca {

CaptureWriter::CaptureWriter(QObject* parent)
    : QObject(parent)
{
    open_.store(0);
    stopping_.store(0);
}

CaptureWriter::~CaptureWriter()
{
    close();
}

QString CaptureWriter::filePath() const
{
    QMutexLocker lock(&mutex_);
    return filePath_;
}

int CaptureWriter::queuedCount() const
{
    QMutexLocker lock(&mutex_);
    return queue_.size();
}

Result CaptureWriter::open(const QString& filePath, const QUuid& sessionId, int maxQueue)
{
    if (filePath.isEmpty())
        return Result::fail(QStringLiteral("invalid_path"), QStringLiteral("抓包路径为空"));
    if (sessionId.isNull())
        return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));
    if (maxQueue <= 0)
        return Result::fail(QStringLiteral("invalid_queue"), QStringLiteral("队列容量非法"));
    if (open_.load() != 0)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("抓包写入器已打开"));

    {
        QMutexLocker lock(&mutex_);
        filePath_ = filePath;
        sessionId_ = sessionId;
        maxQueue_ = maxQueue;
        queue_.clear();
        headerWritten_ = false;
        stopping_.store(0);
    }

    // 先写文件头，确保文件可创建；随后由工作线程追加记录
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return Result::fail(QStringLiteral("capture_open_failed"),
                            file.errorString().isEmpty() ? QStringLiteral("无法创建抓包文件")
                                                         : file.errorString());
    }
    const QByteArray header =
        serializeCaptureFileHeader(sessionId, QDateTime::currentDateTimeUtc()) + '\n';
    if (file.write(header) != header.size()) {
        file.close();
        return Result::fail(QStringLiteral("capture_write_failed"), QStringLiteral("写入文件头失败"));
    }
    file.flush();
    file.close();

    {
        QMutexLocker lock(&mutex_);
        headerWritten_ = true;
    }

    open_.store(1);
    thread_ = QThread::create([this]() { workerMain(); });
    thread_->setObjectName(QStringLiteral("CaptureWriter"));
    thread_->start();
    return Result::success();
}

Result CaptureWriter::enqueue(const CommRecord& record)
{
    if (open_.load() == 0)
        return Result::fail(QStringLiteral("not_open"), QStringLiteral("抓包未打开"));
    if (stopping_.load() != 0)
        return Result::fail(QStringLiteral("stopping"), QStringLiteral("抓包正在关闭"));

    QMutexLocker lock(&mutex_);
    if (queue_.size() >= maxQueue_)
        return Result::fail(QStringLiteral("capture_queue_full"), QStringLiteral("抓包队列已满"));
    queue_.enqueue(record);
    cond_.wakeOne();
    return Result::success();
}

void CaptureWriter::close()
{
    if (open_.load() == 0 && !thread_)
        return;

    // stopping_ 与 wait 谓词必须在同一把锁下更新，避免 lost-wakeup
    {
        QMutexLocker lock(&mutex_);
        stopping_.store(1);
        cond_.wakeAll();
    }

    if (thread_) {
        if (!thread_->wait(15000)) {
            // 超时仍不退出：避免对存活线程 delete；标记停用后泄漏线程对象直至进程退出
            open_.store(0);
            thread_ = nullptr;
            return;
        }
        delete thread_;
        thread_ = nullptr;
    }

    {
        QMutexLocker lock(&mutex_);
        queue_.clear();
        open_.store(0);
        stopping_.store(0);
        headerWritten_ = false;
    }
}

void CaptureWriter::workerMain()
{
    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        emit writeFailed(QStringLiteral("capture_open_failed"),
                         file.errorString().isEmpty() ? QStringLiteral("工作线程无法打开抓包文件")
                                                      : file.errorString());
        open_.store(0);
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    for (;;) {
        CommRecord item;
        bool hasItem = false;
        {
            QMutexLocker lock(&mutex_);
            while (queue_.isEmpty() && stopping_.load() == 0)
                cond_.wait(&mutex_);

            if (!queue_.isEmpty()) {
                item = queue_.dequeue();
                hasItem = true;
            } else if (stopping_.load() != 0) {
                break;
            }
        }

        if (!hasItem)
            continue;

        const QByteArray line = serializeCaptureRecord(item) + '\n';
        if (file.write(line) != line.size()) {
            emit writeFailed(QStringLiteral("capture_write_failed"),
                             file.errorString().isEmpty() ? QStringLiteral("抓包写入失败")
                                                          : file.errorString());
            open_.store(0);
            break;
        }
        file.flush();
    }

    file.close();
    open_.store(0);
}

} // namespace ca