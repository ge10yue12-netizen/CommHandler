#pragma once

#include "Types.h"

#include <QString>
#include <QUuid>

namespace ca {

// 网络端点描述；Serial 场景 address 存放规范化 COM 名
struct Endpoint
{
    QString address;
    quint16 port = 0;
};

/**
 * @brief 同步调用点的接受结果：ok 仅表示本地校验/提交成功，不含异步完成语义。
 *
 * code 为稳定机器可读键；message 为人类可读说明（可英文，日志/UI 自行本地化）。
 */
struct Result
{
    bool ok = false;
    QString code;
    QString message;

    static Result success()
    {
        Result r;
        r.ok = true;
        return r;
    }

    static Result fail(const QString& code, const QString& message)
    {
        Result r;
        r.ok = false;
        r.code = code;
        r.message = message;
        return r;
    }
};

// 错误信号与 ErrorEvent 记录共用载荷
struct CommError
{
    QString code;
    QString message;
    QString sessionId;
    QString channelId;
    QUuid requestId;
};

} // namespace ca