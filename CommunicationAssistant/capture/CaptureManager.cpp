#include "CaptureManager.h"

#include <QDateTime>
#include <QDir>

namespace ca {

CaptureManager::CaptureManager(QObject* parent)
    : QObject(parent)
{
    outputDir_ = QDir::current().filePath(QStringLiteral("captures"));
}

CaptureManager::~CaptureManager()
{
    stopAll();
}

void CaptureManager::setOutputDirectory(const QString& dir)
{
    if (!dir.trimmed().isEmpty())
        outputDir_ = dir.trimmed();
}

QString CaptureManager::sanitizeFileToken(const QString& raw)
{
    QString s = raw.trimmed();
    if (s.isEmpty())
        s = QStringLiteral("session");
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_'))
            continue;
        s[i] = QLatin1Char('_');
    }
    if (s.size() > 64)
        s = s.left(64);
    return s;
}

Result CaptureManager::startSession(const QUuid& sessionId, const QString& sessionName)
{
    if (sessionId.isNull())
        return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));

    stopSession(sessionId);

    QDir dir(outputDir_);
    if (!dir.exists() && !QDir().mkpath(outputDir_))
        return Result::fail(QStringLiteral("capture_dir_failed"), QStringLiteral("无法创建抓包目录"));

    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmss"));
    const QString sid = sessionId.toString(QUuid::WithoutBraces).toLower();
    const QString realName =
        QStringLiteral("%1_%2_%3.clog.jsonl").arg(sanitizeFileToken(sessionName), stamp, sid);
    const QString path = dir.filePath(realName);

    CaptureWriter* writer = new CaptureWriter(this);
    connect(writer, &CaptureWriter::writeFailed, this, &CaptureManager::onWriterFailed);
    const Result opened = writer->open(path, sessionId, maxQueue_);
    if (!opened.ok) {
        delete writer;
        return opened;
    }

    writers_.insert(sessionId, writer);
    emit captureStarted(sessionId, path);
    return Result::success();
}

void CaptureManager::stopSession(const QUuid& sessionId)
{
    CaptureWriter* writer = writers_.take(sessionId);
    if (!writer)
        return;
    writer->close();
    writer->deleteLater();
    emit captureStopped(sessionId);
}

void CaptureManager::stopAll()
{
    const QList<QUuid> ids = writers_.keys();
    for (const QUuid& id : ids)
        stopSession(id);
}

Result CaptureManager::enqueue(const CommRecord& record)
{
    CaptureWriter* writer = writers_.value(record.sessionId, nullptr);
    if (!writer)
        return Result::success(); // 未开抓包：静默跳过
    const Result r = writer->enqueue(record);
    if (!r.ok && r.code == QStringLiteral("capture_queue_full"))
        emit captureFailed(record.sessionId, r.code, r.message);
    return r;
}

bool CaptureManager::hasSession(const QUuid& sessionId) const
{
    return writers_.contains(sessionId);
}

QString CaptureManager::filePathFor(const QUuid& sessionId) const
{
    CaptureWriter* writer = writers_.value(sessionId, nullptr);
    if (!writer)
        return QString();
    return writer->filePath();
}

void CaptureManager::onWriterFailed(const QString& code, const QString& message)
{
    CaptureWriter* writer = qobject_cast<CaptureWriter*>(sender());
    QUuid sid;
    if (writer)
        sid = writer->sessionId();
    emit captureFailed(sid, code, message);
}

} // namespace ca