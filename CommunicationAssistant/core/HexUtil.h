#pragma once

#include <QByteArray>
#include <QString>

namespace ca {

/**
 * @brief 严格 HEX 解析：仅允许 0-9A-Fa-f 与空白，拒绝 0x 前缀，长度必须为偶数。
 *
 * @param error 失败时写入中文原因，供 UI 日志拼接。
 * @thread 无状态，任意线程可调用。
 */
inline QByteArray parseHexPayloadStrict(const QString& text, QString* error)
{
    QString hex;
    hex.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c.isSpace())
            continue;
        if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
              || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
              || (c >= QLatin1Char('A') && c <= QLatin1Char('F')))) {
            if (error)
                *error = QStringLiteral("含非法十六进制字符");
            return QByteArray();
        }
        hex.append(c);
    }
    if (hex.isEmpty()) {
        if (error)
            *error = QStringLiteral("HEX 为空");
        return QByteArray();
    }
    if ((hex.size() % 2) != 0) {
        if (error)
            *error = QStringLiteral("HEX 长度必须为偶数");
        return QByteArray();
    }
    const QByteArray out = QByteArray::fromHex(hex.toLatin1());
    if (out.size() != hex.size() / 2) {
        if (error)
            *error = QStringLiteral("HEX 解码失败");
        return QByteArray();
    }
    return out;
}

} // namespace ca