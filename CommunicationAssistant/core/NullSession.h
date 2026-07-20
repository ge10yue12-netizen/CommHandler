#pragma once

#include "ClaimFor.h"
#include "ICommSession.h"

namespace ca {

/**
 * @brief M1 占位会话：仅验证 configure/claim 链路，不执行实际 I/O。
 *
 * [DEFERRED] open/send 在 CA-M1-SERIAL/TCP/UDP 任务中由 NativeSession 替代。
 * @thread UI/应用线程。
 */
class NullSession : public ICommSession
{
    Q_OBJECT

public:
    explicit NullSession(QObject* parent = nullptr)
        : ICommSession(parent)
    {
    }

    Result configure(const SessionConfig& config) override
    {
        if (state_ != SessionState::Created && state_ != SessionState::Closed)
            return Result::fail(QStringLiteral("invalid_state"), QStringLiteral("configure only in Created/Closed"));
        if (config.mode != WorkMode::Native)
            return Result::fail(QStringLiteral("unsupported_mode"), QStringLiteral("M1 NullSession is Native only"));
        if (config.role != SessionRole::Tool)
            return Result::fail(QStringLiteral("unsupported_role"), QStringLiteral("M1 role must be Tool"));
        if (config.sessionId.isNull())
            return Result::fail(QStringLiteral("invalid_session_id"), QStringLiteral("sessionId required"));

        ResourceClaim claim;
        const Result claimResult = claimFor(config, &claim);
        if (!claimResult.ok)
            return claimResult;

        config_ = config;
        claim_ = claim;
        configured_ = true;
        return Result::success();
    }

    Result open() override
    {
        if (!configured_)
            return Result::fail(QStringLiteral("not_configured"), QStringLiteral("call configure first"));
        return Result::fail(QStringLiteral("not_implemented"),
                            QStringLiteral("Transport open deferred to CA-M1-SERIAL/TCP/UDP"));
    }

    Result close() override
    {
        if (state_ == SessionState::Closed || state_ == SessionState::Created)
            return Result::success();
        state_ = SessionState::Closed;
        emit stateChanged(state_);
        return Result::success();
    }

    Result send(const SendRequest&) override
    {
        return Result::fail(QStringLiteral("not_implemented"), QStringLiteral("send deferred to later M1 tasks"));
    }

    SessionState state() const override { return state_; }
    QUuid sessionId() const override { return config_.sessionId; }

    bool isConfigured() const { return configured_; }
    const ResourceClaim& claim() const { return claim_; }

private:
    SessionConfig config_;
    ResourceClaim claim_;
    SessionState state_ = SessionState::Created;
    bool configured_ = false;
};

} // namespace ca