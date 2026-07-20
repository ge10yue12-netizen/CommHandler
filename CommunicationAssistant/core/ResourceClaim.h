#pragma once

#include "Types.h"

#include <QString>

namespace ca {

/**
 * @brief 由 SessionConfig 推导的资源占用声明；UI 不得手工构造。
 *
 * exclusive 表示进程内互斥（M1 仅 Serial 口由 SessionManager 强制执行）。
 */
struct ResourceClaim
{
    TransportKind transport = TransportKind::Serial;
    QString serialPort;
    QString localAddress;
    quint16 localPort = 0;
    QString remoteAddress;
    quint16 remotePort = 0;
    bool exclusive = true;
    bool addressReuse = false;
};

// Windows COM 名规范化：统一大写 COM 前缀，剥离 \\.\ 前缀以便互斥键一致
inline QString normalizeSerialPortName(const QString& raw)
{
    QString s = raw.trimmed();
    if (s.startsWith(QStringLiteral("\\\\.\\"), Qt::CaseInsensitive))
        s = s.mid(4);
    s = s.toUpper();
    if (!s.isEmpty() && !s.startsWith(QStringLiteral("COM")) && s.at(0).isDigit())
        s = QStringLiteral("COM") + s;
    return s;
}

} // namespace ca