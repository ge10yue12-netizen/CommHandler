#include "LegacyWorker.h"

#include <QUuid>

namespace ca {

LegacyWorker::LegacyWorker(QObject* parent)
    : QObject(parent)
{
    connect(this, &LegacyWorker::sendValuesWithId, this, &LegacyWorker::onSendValuesWithId);
    connect(this, &LegacyWorker::sendTextWithId, this, &LegacyWorker::onSendTextWithId);
}

LegacyWorker::~LegacyWorker()
{
    destroyBackend();
}

void LegacyWorker::destroyBackend()
{
    if (!backend_)
        return;
    backend_->disconnectDevice();
    delete backend_;
    backend_ = nullptr;
}

void LegacyWorker::bindBackendSignals()
{
    connect(backend_, &ILegacyBackend::valuesReceived, this, &LegacyWorker::valuesReceived);
    connect(backend_, &ILegacyBackend::controlEvent, this, &LegacyWorker::controlEvent);
    connect(backend_, &ILegacyBackend::parameterEvent, this, &LegacyWorker::parameterEvent);
    connect(backend_, &ILegacyBackend::backendError, this, &LegacyWorker::backendError);
    connect(backend_, &ILegacyBackend::disconnected, this, &LegacyWorker::disconnected);
}

void LegacyWorker::initialize(const LegacyConfig& config)
{
    destroyBackend();
    config_ = config;
    if (config.useMockBackend)
        backend_ = new MockLegacyBackend(this);
    else
        backend_ = new CommHandlerBackend(this);
    bindBackendSignals();
    const Result r = backend_->configure(config);
    emit initializeFinished(r.ok, r.code, r.message);
}

void LegacyWorker::connectDevice()
{
    if (!backend_) {
        emit connectFinished(false, QStringLiteral("not_initialized"), QStringLiteral("Worker 未初始化"));
        return;
    }
    const Result r = backend_->connectDevice();
    emit connectFinished(r.ok, r.code, r.message);
}

void LegacyWorker::disconnectDevice()
{
    if (!backend_) {
        emit disconnectFinished(true, QString(), QString());
        return;
    }
    const Result r = backend_->disconnectDevice();
    emit disconnectFinished(r.ok, r.code, r.message);
}

void LegacyWorker::sendValues(const QVector<double>& values)
{
    onSendValuesWithId(values, QUuid());
}

void LegacyWorker::sendText(const QString& text)
{
    onSendTextWithId(text, QUuid());
}

void LegacyWorker::onSendValuesWithId(const QVector<double>& values, const QUuid& requestId)
{
    if (!backend_) {
        emit sendFinished(false, QStringLiteral("not_initialized"), QStringLiteral("Worker 未初始化"), requestId);
        return;
    }
    const Result r = backend_->sendValues(values);
    emit sendFinished(r.ok, r.code, r.message, requestId);
}

void LegacyWorker::onSendTextWithId(const QString& text, const QUuid& requestId)
{
    if (!backend_) {
        emit sendFinished(false, QStringLiteral("not_initialized"), QStringLiteral("Worker 未初始化"), requestId);
        return;
    }
    const Result r = backend_->sendText(text);
    emit sendFinished(r.ok, r.code, r.message, requestId);
}

void LegacyWorker::shutdown()
{
    destroyBackend();
    emit shutdownFinished();
}

} // namespace ca