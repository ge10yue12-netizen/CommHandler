#pragma once

#include "CommRecord.h"
#include "Result.h"
#include "SessionConfig.h"
#include "Types.h"

#include <QObject>
#include <QUuid>

namespace ca {

/**
 * @brief UI 侧会话契约：configure/open/close/send 均为异步接受语义。
 *
 * 返回 Result::ok 仅表示请求被本地接受，最终 Connected/Closed 等状态通过 stateChanged 报告。
 * CommRecord 生命周期（Queued/Submitted/Completed 等）通过 recordReceived 报告。
 * [DECISION] 见开发基线 Queued/Submitted 与 SessionState 状态机。
 *
 * @thread 须在创建本对象的线程（通常为 UI/应用线程）调用虚函数与连接信号。
 */
class ICommSession : public QObject
{
    Q_OBJECT

public:
    explicit ICommSession(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~ICommSession() override = default;

    /**
     * @brief 写入不可变 SessionConfig 快照。
     * @thread UI/应用线程。
     * 返回成功表示配置被接受；仅 Created/Closed 可调用。
     * 主要错误：invalid_state、unsupported_mode、invalid_session_id、claim 推导失败。
     */
    virtual Result configure(const SessionConfig& config) = 0;

    /**
     * @brief 请求打开传输与会话。
     * @thread UI/应用线程；不阻塞等待远端就绪。
     * 返回成功表示 open 请求已提交；Connected 通过 stateChanged 到达。
     * 主要错误：not_configured、already_open、resource_busy、传输 open 失败。
     */
    virtual Result open() = 0;

    /**
     * @brief 请求关闭会话并释放资源占用。
     * @thread UI/应用线程。
     * 返回成功表示 close 请求已接受；Closed 通过 stateChanged 到达。
     */
    virtual Result close() = 0;

    /**
     * @brief 提交发送请求。
     * @thread UI/应用线程。
     * 返回成功仅表示 payload 已进入本地提交流程（Queued→Submitted），不等于远端确认。
     * [DECISION] 见 BASELINE RecordStatus::Queued/Submitted。
     * 主要错误：not_connected、invalid_request_id、empty_payload、传输 write 失败。
     */
    virtual Result send(const SendRequest& request) = 0;

    virtual SessionState state() const = 0;
    virtual QUuid sessionId() const = 0;

signals:
    void recordReceived(const ca::CommRecord& record);
    void stateChanged(ca::SessionState state);
    void errorOccurred(const ca::CommError& error);
};

} // namespace ca

Q_DECLARE_METATYPE(ca::CommRecord)
Q_DECLARE_METATYPE(ca::CommError)
Q_DECLARE_METATYPE(ca::SessionState)