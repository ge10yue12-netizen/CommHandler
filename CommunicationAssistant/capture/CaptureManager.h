#pragma once

#include "CaptureWriter.h"
#include "CommRecord.h"
#include "Result.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QUuid>

namespace ca {

/**
 * @brief 应用层抓包管理：每活动 Session 一个 CaptureWriter / 独立文件 / 有界队列。
 * [DECISION] 只消费 recordReceived；不持有 Transport。
 * @thread 应用/UI 线程调用 start/stop/enqueue；写盘在 Writer 后台线程。
 */
class CaptureManager : public QObject
{
    Q_OBJECT

public:
    explicit CaptureManager(QObject* parent = nullptr);
    ~CaptureManager() override;

    void setOutputDirectory(const QString& dir);
    QString outputDirectory() const { return outputDir_; }
    void setMaxQueue(int maxQueue) { maxQueue_ = maxQueue; }

    /**
     * @brief 为 Session 启动 Writer；同 sessionId 重复启动先停止旧 Writer。
     * 文件名：{sanitizedName}_{utc}_{sessionId}.clog.jsonl
     */
    Result startSession(const QUuid& sessionId, const QString& sessionName);

    void stopSession(const QUuid& sessionId);
    void stopAll();

    /** 按 record.sessionId 路由入队；无对应 Writer 时忽略（非错误）。 */
    Result enqueue(const CommRecord& record);

    bool hasSession(const QUuid& sessionId) const;
    QString filePathFor(const QUuid& sessionId) const;
    int activeWriterCount() const { return writers_.size(); }

    static QString sanitizeFileToken(const QString& raw);

signals:
    void captureFailed(const QUuid& sessionId, const QString& code, const QString& message);
    void captureStarted(const QUuid& sessionId, const QString& filePath);
    void captureStopped(const QUuid& sessionId);

private slots:
    void onWriterFailed(const QString& code, const QString& message);

private:
    QString outputDir_;
    int maxQueue_ = 1024;
    QHash<QUuid, CaptureWriter*> writers_;
};

} // namespace ca