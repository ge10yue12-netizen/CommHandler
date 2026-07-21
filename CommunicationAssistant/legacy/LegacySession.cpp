#include "LegacySession.h"

#include "ClaimFor.h"
#include "UIDef.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QRegularExpression>
#include <QVariantList>

namespace ca {

namespace {
qint64 monotonicNsNow()
{
    static QElapsedTimer timer;
    if (!timer.isValid())
        timer.start();
    return timer.nsecsElapsed();
}

// 将 W_CUSTOM_* 事件码转为可读指令名（与 CommLab 对齐）
QString legacyControlMsgName(int msg)
{
    switch (msg) {
    case W_CUSTOM_COMM_STARTCALC: return QStringLiteral("开始");
    case W_CUSTOM_COMM_STOPCALC: return QStringLiteral("停止");
    case W_CUSTOM_COMM_EXITPROG: return QStringLiteral("退出");
    case W_CUSTOM_COMM_AUTO_SAVE_IMAGE: return QStringLiteral("存图");
    case W_CUSTOM_ZERO_CLEARING: return QStringLiteral("清零");
    case W_CUSTOM_COMM_SINGALTRIIMAGESAVE: return QStringLiteral("触发存图");
    case W_CUSTOM_COMM_RESET_LVE_LENGTH: return QStringLiteral("重置标距");
    case W_CUSTOM_COMM_CALC_LVE_LENGTH: return QStringLiteral("识别线条标距");
    case W_CUSTOM_COMM_DATASTREAMING: return QStringLiteral("数据流控制");
    case W_CUSTOM_COMM_DATAPOLLING: return QStringLiteral("轮询请求");
    case W_CUSTOM_COMM_UPDATEPULSECTRL: return QStringLiteral("刷新脉冲");
    case W_CUSTOM_COMM_PULSECALIDONE: return QStringLiteral("脉冲标定完成");
    case W_CUSTOM_COMM_PULSEBUTTON: return QStringLiteral("刷新脉冲按钮");
    default: return QStringLiteral("0x%1").arg(msg, 0, 16);
    }
}
} // namespace

LegacySession::LegacySession(SessionManager* manager, QObject* parent)
    : ICommSession(parent)
    , manager_(manager)
    , watchdog_(this)
{
    worker_ = new LegacyWorker();
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);

    connect(worker_, &LegacyWorker::initializeFinished, this, &LegacySession::onInitializeFinished);
    connect(worker_, &LegacyWorker::connectFinished, this, &LegacySession::onConnectFinished);
    connect(worker_, &LegacyWorker::disconnectFinished, this, &LegacySession::onDisconnectFinished);
    connect(worker_, &LegacyWorker::sendFinished, this, &LegacySession::onSendFinished);
    connect(worker_, &LegacyWorker::valuesReceived, this, &LegacySession::onValuesReceived, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::controlEvent, this, &LegacySession::onControlEvent, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::parameterEvent, this, &LegacySession::onParameterEvent, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::unparsedRx, this, &LegacySession::onUnparsedRx, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::backendError, this, &LegacySession::onBackendError, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::disconnected, this, &LegacySession::onBackendDisconnected, Qt::QueuedConnection);
    connect(worker_, &LegacyWorker::shutdownFinished, this, &LegacySession::onWorkerShutdownFinished);

    connect(&watchdog_, &LegacyWatchdog::timedOut, this, &LegacySession::onWatchdogTimeout);
    watchdog_.setTimeoutMs(8000);

    workerThread_.start();
}

LegacySession::~LegacySession()
{
    watchdog_.disarmAll();
    // 析构禁止 BlockingQueuedConnection：若 Worker 正经 Queued 回投主线程，易死锁
    if (worker_ && workerThread_.isRunning()) {
        if (state_ != SessionState::Closed && state_ != SessionState::Created
            && state_ != SessionState::Unresponsive) {
            pendingClose_ = true;
            setSessionState(SessionState::Closing);
            QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(worker_, "shutdown", Qt::QueuedConnection);
        workerThread_.quit();
        workerThread_.wait(8000);
    } else {
        workerThread_.quit();
        workerThread_.wait(1000);
    }
    if (manager_ && !config_.sessionId.isNull()) {
        manager_->releaseLegacy(config_.sessionId);
        manager_->release(config_.sessionId);
    }
    worker_ = nullptr;
}

Result LegacySession::configure(const SessionConfig& config)
{
    if (state_ != SessionState::Created && state_ != SessionState::Closed)
        return Result::fail(QStringLiteral("invalid_state"), QStringLiteral("仅允许在 Created/Closed 下配置"));
    if (config.mode != WorkMode::LegacyDll)
        return Result::fail(QStringLiteral("unsupported_mode"), QStringLiteral("LegacySession 仅支持 LegacyDll"));
    if (config.role != SessionRole::Tool)
        return Result::fail(QStringLiteral("unsupported_role"), QStringLiteral("Legacy 角色必须为 Tool"));
    if (config.sessionId.isNull())
        return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("缺少 sessionId"));
    if (config.transport.legacy.commType != 0 && config.transport.legacy.commType != 1)
        return Result::fail(QStringLiteral("unsupported_legacy_comm"),
                            QStringLiteral("仅开放 Network/Serial；Voltage/Pulse 未验证"));

    ResourceClaim claim;
    const Result claimResult = claimFor(config, &claim);
    if (!claimResult.ok)
        return claimResult;

    config_ = config;
    const LegacyCommKind kind =
        (config.transport.legacy.commType == 1) ? LegacyCommKind::Serial : LegacyCommKind::Network;
    profile_ = legacyCapabilityFor(kind, config.transport.legacy.protocolIndex);
    configured_ = true;
    if (state_ == SessionState::Closed)
        setSessionState(SessionState::Created);
    return Result::success();
}

Result LegacySession::open()
{
    if (!configured_)
        return Result::fail(QStringLiteral("not_configured"), QStringLiteral("请先调用 configure"));
    if (state_ == SessionState::Connected || state_ == SessionState::Opening
        || state_ == SessionState::Unresponsive)
        return Result::fail(QStringLiteral("already_open"), QStringLiteral("会话已在打开或不可响应"));
    if (!manager_)
        return Result::fail(QStringLiteral("no_manager"), QStringLiteral("缺少 SessionManager"));

    ResourceClaim claim;
    const Result claimResult = claimFor(config_, &claim);
    if (!claimResult.ok)
        return claimResult;

    const Result acquired = manager_->acquire(config_.sessionId, claim);
    if (!acquired.ok)
        return acquired;
    const Result legacyLock = manager_->acquireLegacy(config_.sessionId);
    if (!legacyLock.ok) {
        manager_->release(config_.sessionId);
        return legacyLock;
    }

    setSessionState(SessionState::Opening);
    pendingClose_ = false;
    cancelOpen_ = false;
    pendingOpenOp_ = QUuid::createUuid();
    watchdog_.arm(pendingOpenOp_);
    const bool queued = QMetaObject::invokeMethod(
        worker_, "initialize", Qt::QueuedConnection,
        QGenericArgument("LegacyConfig", static_cast<const void*>(&config_.transport.legacy)));
    if (!queued) {
        watchdog_.disarm(pendingOpenOp_);
        manager_->releaseLegacy(config_.sessionId);
        manager_->release(config_.sessionId);
        setSessionState(SessionState::Closed);
        return Result::fail(QStringLiteral("legacy_invoke_failed"),
                            QStringLiteral("无法投递兼容通道初始化（请检查元类型注册）"));
    }
    return Result::success();
}

Result LegacySession::close()
{
    if (state_ == SessionState::Closed || state_ == SessionState::Created)
        return Result::success();
    if (state_ == SessionState::Unresponsive) {
        // 基线：不向可能阻塞的 Worker 排 close；仅提示侧已禁新命令
        return Result::fail(QStringLiteral("unresponsive"),
                            QStringLiteral("会话无响应，请重启应用程序以回收工作线程"));
    }
    if (tearingDown_)
        return Result::success();

    // Opening 中关闭：取消后续 connect，避免 disconnect 后再被 connect 抢回
    if (state_ == SessionState::Opening)
        cancelOpen_ = true;

    pendingClose_ = true;
    setSessionState(SessionState::Closing);
    watchdog_.disarmAll();
    QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
    return Result::success();
}

Result LegacySession::send(const SendRequest& request)
{
    if (state_ == SessionState::Unresponsive)
        return Result::fail(QStringLiteral("unresponsive"), QStringLiteral("会话不可响应，禁止新命令"));
    if (state_ != SessionState::Connected)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("会话未连接"));
    if (request.requestId.isNull())
        return Result::fail(QStringLiteral("invalid_request_id"), QStringLiteral("缺少 requestId"));

    QVector<double> values;
    QString text;
    bool isText = false;
    const Result validated = validateSendAgainstCapability(request, &values, &text, &isText);
    if (!validated.ok)
        return validated;

    emitTxRecord(request, RecordStatus::Queued);
    pendingSendOp_ = request.requestId;
    pendingSendRequest_ = request;
    watchdog_.arm(pendingSendOp_);

    if (isText) {
        emit worker_->sendTextWithId(text, request.requestId);
    } else {
        emit worker_->sendValuesWithId(values, request.requestId);
    }
    return Result::success();
}

Result LegacySession::validateSendAgainstCapability(const SendRequest& request, QVector<double>* outValues,
                                                    QString* outText, bool* isText) const
{
    const QString mode = request.attributes.value(QStringLiteral("legacySend")).toString();

    // 显式文本
    if (mode == QStringLiteral("text")) {
        if (!profile_.supports(LegacyCapability::SendTransparentText))
            return Result::fail(QStringLiteral("capability_denied"), QStringLiteral("当前协议不支持透明文本发送"));
        *isText = true;
        *outText = QString::fromUtf8(request.payload);
        if (outText->isEmpty())
            *outText = request.attributes.value(QStringLiteral("text")).toString();
        if (outText->isEmpty())
            return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("文本为空"));
        return Result::success();
    }

    // 数值：显式 values，或未标注时优先尝试 CSV（调度器仅填 payload）
    const bool tryValues = (mode == QStringLiteral("values") || mode.isEmpty());
    if (tryValues && profile_.supports(LegacyCapability::SendEncodedValues)) {
        QVariantList list = request.attributes.value(QStringLiteral("values")).toList();
        if (list.isEmpty() && !request.payload.isEmpty()) {
            const QString csv = QString::fromUtf8(request.payload);
            const QStringList parts = csv.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
            bool allOk = !parts.isEmpty();
            for (const QString& p : parts) {
                bool ok = false;
                const double d = p.trimmed().toDouble(&ok);
                if (!ok) {
                    allOk = false;
                    break;
                }
                list.push_back(d);
            }
            if (!allOk)
                list.clear();
        }
        if (!list.isEmpty()) {
            QVector<double> values;
            values.reserve(list.size());
            for (const QVariant& v : list)
                values.push_back(v.toDouble());

            const QString lim =
                profile_.entries.value(static_cast<int>(LegacyCapability::SendEncodedValues)).limitation;
            if (lim.contains(QStringLiteral("恰好 4")) && values.size() != 4)
                return Result::fail(QStringLiteral("invalid_value_count"), QStringLiteral("需要恰好 4 个数值"));
            if (lim.contains(QStringLiteral("5")) && values.size() < 5)
                return Result::fail(QStringLiteral("invalid_value_count"), QStringLiteral("至少需要 5 个数值"));
            if (lim.contains(QStringLiteral("2")) && values.size() < 2)
                return Result::fail(QStringLiteral("invalid_value_count"), QStringLiteral("至少需要 2 个数值"));

            *isText = false;
            *outValues = values;
            return Result::success();
        }
    }

    // 回落透明文本（未标注且数值解析失败时）
    if (mode.isEmpty() && profile_.supports(LegacyCapability::SendTransparentText)) {
        *isText = true;
        *outText = QString::fromUtf8(request.payload);
        if (outText->isEmpty())
            return Result::fail(QStringLiteral("empty_payload"), QStringLiteral("文本为空"));
        return Result::success();
    }

    if (mode == QStringLiteral("values"))
        return Result::fail(QStringLiteral("capability_denied"),
                            QStringLiteral("当前协议不支持协议数值发送，或数值格式/个数不符合要求"));
    if (!profile_.supports(LegacyCapability::SendEncodedValues)
        && !profile_.supports(LegacyCapability::SendTransparentText))
        return Result::fail(QStringLiteral("capability_denied"),
                            QStringLiteral("当前协议无可用发送路径（发数值与发文本均不支持）"));
    return Result::fail(QStringLiteral("capability_denied"),
                        QStringLiteral("载荷无法解析为合法数值，且当前协议不支持透明文本"));
}

void LegacySession::onInitializeFinished(bool ok, const QString& code, const QString& message)
{
    if (cancelOpen_ || state_ != SessionState::Opening) {
        // 打开已取消：仅在仍 Opening 时不应再 connect；Closing 时由 disconnect 收尾
        if (cancelOpen_ && state_ == SessionState::Opening) {
            watchdog_.disarm(pendingOpenOp_);
            QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
        }
        return;
    }
    if (!ok) {
        watchdog_.disarm(pendingOpenOp_);
        emitError(code, message);
        tearDown(true);
        return;
    }
    QMetaObject::invokeMethod(worker_, "connectDevice", Qt::QueuedConnection);
}

void LegacySession::onConnectFinished(bool ok, const QString& code, const QString& message)
{
    if (cancelOpen_ || state_ != SessionState::Opening) {
        // 迟到的 connect 成功：立即断开，避免关闭后仍占着后端连接
        if (ok)
            QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
        return;
    }
    watchdog_.disarm(pendingOpenOp_);
    if (!ok) {
        emitError(code, message);
        tearDown(true);
        return;
    }
    setSessionState(SessionState::Connected);
    CommRecord conn;
    conn.sessionId = config_.sessionId;
    conn.sequence = ++sequence_;
    conn.kind = RecordKind::ConnectionEvent;
    conn.status = RecordStatus::Observed;
    conn.direction = Direction::System;
    conn.wallTime = QDateTime::currentDateTimeUtc();
    conn.monotonicNs = monotonicNsNow();
    // 协议 index 不改变连通语义；按串口/TCP/UDP 角色写摘要
    if (config_.transport.legacy.commType == 1) {
        conn.summary = QStringLiteral("Legacy 串口已打开");
    } else if (config_.transport.legacy.transferType == 1) {
        conn.summary = QStringLiteral("Legacy UDP 已绑定");
    } else if (config_.transport.legacy.model == 0) {
        conn.summary = QStringLiteral("Legacy TCP 已监听（等待客户端）");
    } else {
        conn.summary = QStringLiteral("Legacy TCP 已连接");
    }
    emit recordReceived(conn);
}

void LegacySession::onDisconnectFinished(bool ok, const QString& code, const QString& message)
{
    Q_UNUSED(ok);
    Q_UNUSED(code);
    Q_UNUSED(message);
    if (pendingClose_ || state_ == SessionState::Closing || state_ == SessionState::Faulted)
        tearDown(false);
}

void LegacySession::onSendFinished(bool ok, const QString& code, const QString& message, const QUuid& requestId)
{
    watchdog_.disarm(requestId.isNull() ? pendingSendOp_ : requestId);
    if (ok) {
        emitTxRecord(pendingSendRequest_, RecordStatus::Submitted);
    } else {
        emitTxRecord(pendingSendRequest_, RecordStatus::Failed, code, message);
    }
    pendingSendOp_ = QUuid();
}

void LegacySession::onValuesReceived(const QVector<double>& values, int type)
{
    if (!profile_.supports(LegacyCapability::ReceiveValues)) {
        emitRxCapDrop(QStringLiteral("收数值✗"), values.size());
        return;
    }
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::LegacyValueEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    QVariantList list;
    for (double d : values)
        list.push_back(d);
    rec.attributes.insert(QStringLiteral("values"), list);
    rec.attributes.insert(QStringLiteral("legacyType"), type);
    rec.attributes.insert(QStringLiteral("protocolLabel"), protocolObjectLabel());
    rec.summary = QStringLiteral("Legacy 数值 %1 个").arg(values.size());
    emit recordReceived(rec);
}

void LegacySession::onControlEvent(int ctrlCmd, int viewId, int msg)
{
    if (!profile_.supports(LegacyCapability::ReceiveControlEvents)) {
        emitRxCapDrop(QStringLiteral("收控制✗"), 1);
        return;
    }
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::LegacyControlEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.attributes.insert(QStringLiteral("ctrlCmd"), ctrlCmd);
    rec.attributes.insert(QStringLiteral("viewId"), viewId);
    rec.attributes.insert(QStringLiteral("msg"), msg);
    rec.attributes.insert(QStringLiteral("msgName"), legacyControlMsgName(msg));
    rec.summary = QStringLiteral("Legacy 控制 %1 | ctrlCmd=%2 viewId=%3 msg=%4")
                      .arg(legacyControlMsgName(msg))
                      .arg(ctrlCmd)
                      .arg(viewId)
                      .arg(msg);
    emit recordReceived(rec);
}

void LegacySession::onParameterEvent(int ctrlCmd, int viewId, int msg, const QVariantMap& extra)
{
    if (!profile_.supports(LegacyCapability::ReceiveParameterEvents)) {
        emitRxCapDrop(QStringLiteral("收参数✗"), 1);
        return;
    }
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::LegacyParameterEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.attributes.insert(QStringLiteral("ctrlCmd"), ctrlCmd);
    rec.attributes.insert(QStringLiteral("viewId"), viewId);
    rec.attributes.insert(QStringLiteral("msg"), msg);
    rec.attributes.insert(QStringLiteral("msgName"), legacyControlMsgName(msg));
    rec.attributes.insert(QStringLiteral("extra"), extra);
    rec.summary = QStringLiteral("Legacy 参数 %1 | ctrlCmd=%2 viewId=%3 msg=%4")
                      .arg(legacyControlMsgName(msg))
                      .arg(ctrlCmd)
                      .arg(viewId)
                      .arg(msg);
    emit recordReceived(rec);
}

void LegacySession::onUnparsedRx(const QByteArray& raw)
{
    if (raw.isEmpty())
        return;

    // 单包上限：截断后仍可观测，避免 Queued/UI 被巨包打爆
    QByteArray body = raw;
    bool truncated = false;
    if (body.size() > kMaxUnparsedBytes) {
        body = body.left(kMaxUnparsedBytes);
        truncated = true;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastUnparsedEmitMs_ > 0 && (now - lastUnparsedEmitMs_) < kUnparsedMinIntervalMs) {
        ++suppressedUnparsedCount_;
        return;
    }

    const int suppressed = suppressedUnparsedCount_;
    suppressedUnparsedCount_ = 0;
    lastUnparsedEmitMs_ = now;

    // 进数据窗：有线帧字节；summary 供日志短码
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::RawChunk;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::Rx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = body;
    rec.attributes.insert(QStringLiteral("legacyUnparsed"), true);
    rec.attributes.insert(QStringLiteral("protocolLabel"), protocolObjectLabel());
    if (truncated)
        rec.attributes.insert(QStringLiteral("truncated"), true);
    if (raw.size() > body.size())
        rec.attributes.insert(QStringLiteral("rawSize"), raw.size());
    rec.summary = QStringLiteral("接收 | %1 | 协议解析失败").arg(protocolObjectLabel());
    emit recordReceived(rec);

    CommRecord tip;
    tip.sessionId = config_.sessionId;
    tip.sequence = ++sequence_;
    tip.kind = RecordKind::ErrorEvent;
    tip.status = RecordStatus::Observed;
    tip.direction = Direction::System;
    tip.wallTime = QDateTime::currentDateTimeUtc();
    tip.monotonicNs = monotonicNsNow();
    tip.errorCode = QStringLiteral("rx_unparsed");
    if (suppressed > 0) {
        tip.summary = QStringLiteral("接收 | %1 | 协议解析失败 %2 字节（已抑制重复 %3 次）")
                          .arg(protocolObjectLabel())
                          .arg(raw.size())
                          .arg(suppressed);
    } else {
        tip.summary = QStringLiteral("接收 | %1 | 协议解析失败 %2 字节")
                          .arg(protocolObjectLabel())
                          .arg(raw.size());
    }
    emit recordReceived(tip);
}

void LegacySession::onBackendError(const QString& code, const QString& message)
{
    emitError(code, message);
    if (state_ == SessionState::Connected || state_ == SessionState::Opening)
        tearDown(true);
}

void LegacySession::onBackendDisconnected()
{
    // Opening 期间对端拒绝/瞬时断开：必须失败收尾，禁止随后误标「已连接」
    if (state_ == SessionState::Opening) {
        watchdog_.disarm(pendingOpenOp_);
        cancelOpen_ = true;
        QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
        emitError(QStringLiteral("legacy_connect_rejected"),
                  QStringLiteral("连接未完成：对端拒绝或连接过程中已断开"));
        tearDown(true);
        return;
    }
    if (state_ == SessionState::Connected) {
        QMetaObject::invokeMethod(worker_, "disconnectDevice", Qt::QueuedConnection);
        tearDown(true);
    }
}

void LegacySession::onWatchdogTimeout(const QUuid& opId)
{
    Q_UNUSED(opId);
    setSessionState(SessionState::Unresponsive);
    emitError(QStringLiteral("legacy_unresponsive"),
              QStringLiteral("兼容通道工作线程超时（无响应）；已禁止新操作，请重启应用程序"));
    emit unresponsive(QStringLiteral("兼容通道工作线程无响应"));
    // 不 terminate、不向 Worker 排 close
}

void LegacySession::onWorkerShutdownFinished()
{
}

void LegacySession::setSessionState(SessionState state)
{
    if (state_ == state)
        return;
    state_ = state;
    emit stateChanged(state_);
}

void LegacySession::emitError(const QString& code, const QString& message, const QUuid& requestId)
{
    CommError err;
    err.code = code;
    err.message = message;
    err.sessionId = config_.sessionId.toString(QUuid::WithoutBraces);
    err.requestId = requestId;

    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.requestId = requestId;
    rec.kind = RecordKind::ErrorEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::System;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.errorCode = code;
    rec.errorMessage = message;
    rec.summary = message;
    emit recordReceived(rec);
    emit errorOccurred(err);
}

QString LegacySession::protocolObjectLabel() const
{
    const bool serial = (config_.transport.legacy.commType == 1);
    return QStringLiteral("%1%2")
        .arg(serial ? QStringLiteral("串口") : QStringLiteral("网口"))
        .arg(config_.transport.legacy.protocolIndex);
}

void LegacySession::emitRxCapDrop(const QString& reasonCode, int droppedCount)
{
    // 仅 recordReceived，不走 errorOccurred，避免再刷一条「异常」
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.kind = RecordKind::ErrorEvent;
    rec.status = RecordStatus::Observed;
    rec.direction = Direction::System;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.errorCode = QStringLiteral("rx_cap");
    rec.summary = QStringLiteral("接收 | %1 | 能力下降：%2，丢弃 %3")
                      .arg(protocolObjectLabel(), reasonCode)
                      .arg(droppedCount);
    emit recordReceived(rec);
}

void LegacySession::emitTxRecord(const SendRequest& request, RecordStatus status, const QString& code,
                                 const QString& message)
{
    CommRecord rec;
    rec.sessionId = config_.sessionId;
    rec.sequence = ++sequence_;
    rec.requestId = request.requestId;
    rec.taskId = request.taskId;
    rec.kind = RecordKind::LegacyValueEvent;
    rec.status = status;
    rec.direction = Direction::Tx;
    rec.wallTime = QDateTime::currentDateTimeUtc();
    rec.monotonicNs = monotonicNsNow();
    rec.bytes = request.payload;
    rec.attributes = request.attributes;
    rec.errorCode = code;
    rec.errorMessage = message;
    rec.summary = QStringLiteral("Legacy 发送（%1）").arg(recordStatusDisplayName(status));
    emit recordReceived(rec);
}

void LegacySession::tearDown(bool fromFault)
{
    if (tearingDown_)
        return;
    tearingDown_ = true;
    pendingClose_ = false;
    cancelOpen_ = false;
    watchdog_.disarmAll();
    if (fromFault)
        setSessionState(SessionState::Faulted);
    if (manager_) {
        manager_->releaseLegacy(config_.sessionId);
        manager_->release(config_.sessionId);
    }
    setSessionState(SessionState::Closed);
    CommRecord conn;
    conn.sessionId = config_.sessionId;
    conn.sequence = ++sequence_;
    conn.kind = RecordKind::ConnectionEvent;
    conn.status = RecordStatus::Observed;
    conn.direction = Direction::System;
    conn.wallTime = QDateTime::currentDateTimeUtc();
    conn.monotonicNs = monotonicNsNow();
    conn.summary = fromFault ? QStringLiteral("Legacy 连接中断（故障断开）")
                             : QStringLiteral("Legacy 已断开");
    emit recordReceived(conn);
    tearingDown_ = false;
}

} // namespace ca