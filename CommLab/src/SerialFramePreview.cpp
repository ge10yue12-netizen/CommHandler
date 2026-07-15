#include "SerialFramePreview.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <cmath>
#include <cstdint>
#include <string>

namespace SerialFramePreview {
namespace {

// 按小数位 n 做四舍五入截断（与 SerialPortComm 三思组包一致）。
float getNFromData(float val, int n)
{
    float temp1 = val;
    const int t1 = static_cast<int>(static_cast<uint32_t>(temp1 * std::pow(10.0f, n + 1)) % 10);
    if (t1 > 4) {
        temp1 = (static_cast<int>(temp1 * std::pow(10.0f, n)) + 1) / std::pow(10.0f, n);
    } else {
        temp1 = (static_cast<int>(temp1 * std::pow(10.0f, n))) / std::pow(10.0f, n);
    }
    return temp1;
}

// 将单个浮点写入三思段：首字节正负号，其余 ASCII 数字/小数点。
void floatToHex(unsigned char* str, int row, float fValue, int segSize)
{
    str[row * segSize] = fValue >= 0 ? 0x2B : 0x2D;
    int n = segSize - 3;
    for (int i = 1; i < 4; ++i) {
        if (std::fabs(fValue) >= std::pow(10.0, i)) {
            n = segSize - (i + 3);
        }
    }
    const float val = getNFromData(std::fabs(fValue), n);
    const std::string data = std::to_string(val);
    for (int i = 0; i < segSize - 1; ++i) {
        const int value1 = (i < static_cast<int>(data.size()))
                               ? static_cast<int>(data[static_cast<size_t>(i)])
                               : 0;
        unsigned char out = 0x2A;
        switch (value1) {
        case 46: out = 0x2E; break;
        case 48: out = 0x30; break;
        case 49: out = 0x31; break;
        case 50: out = 0x32; break;
        case 51: out = 0x33; break;
        case 52: out = 0x34; break;
        case 53: out = 0x35; break;
        case 54: out = 0x36; break;
        case 55: out = 0x37; break;
        case 56: out = 0x38; break;
        case 57: out = 0x39; break;
        default: out = 0x2A; break;
        }
        str[row * segSize + i + 1] = out;
    }
}

} // namespace

// 组串口三思数值帧：每值 segSize 字节，帧末 0x0D。
QByteArray packSansiSsck(const QVector<double>& values, int segSize)
{
    if (values.isEmpty() || segSize <= 1) {
        return {};
    }
    const int size = segSize * values.size() + 1;
    QByteArray out(size, '\0');
    auto* p = reinterpret_cast<unsigned char*>(out.data());
    for (int i = 0; i < values.size(); ++i) {
        floatToHex(p, i, static_cast<float>(values.at(i)), segSize);
    }
    p[segSize * values.size()] = 0x0D;
    return out;
}

// 字节转空格分隔大写十六进制。
QString toHexSpaced(const QByteArray& bytes)
{
    QStringList parts;
    parts.reserve(bytes.size());
    for (int i = 0; i < bytes.size(); ++i) {
        const auto c = static_cast<unsigned char>(bytes.at(i));
        parts << QStringLiteral("%1").arg(c, 2, 16, QLatin1Char('0')).toUpper();
    }
    return parts.join(QLatin1Char(' '));
}

// 预览组包自检；通过返回空串。
QString selfCheckOrEmpty()
{
    const QByteArray packed8 = packSansiSsck(QVector<double>{1.0}, 8);
    if (packed8.isEmpty()) {
        return QStringLiteral("预览为空");
    }
    if (packed8.size() != 9) {
        return QStringLiteral("segSize=8 期望长度 9，实际 %1").arg(packed8.size());
    }
    if (static_cast<unsigned char>(packed8.at(packed8.size() - 1)) != 0x0D) {
        return QStringLiteral("末字节不是 0D");
    }
    if (static_cast<unsigned char>(packed8.at(0)) != 0x2B) {
        return QStringLiteral("正数首字节不是 2B(+)");
    }

    const QByteArray packed5 = packSansiSsck(QVector<double>{1.0}, 5);
    if (packed5.size() != 6) {
        return QStringLiteral("segSize=5 期望长度 6，实际 %1").arg(packed5.size());
    }
    if (packed5.size() >= packed8.size()) {
        return QStringLiteral("segSize=5 帧应短于 segSize=8");
    }

    const QByteArray packed6 = packSansiSsck(QVector<double>{1.0}, 6);
    if (packed6.size() != 7) {
        return QStringLiteral("segSize=6 期望长度 7，实际 %1").arg(packed6.size());
    }

    if (!packSansiSsck(QVector<double>{}, 8).isEmpty()) {
        return QStringLiteral("空数值应返回空包");
    }
    if (!packSansiSsck(QVector<double>{1.0}, 1).isEmpty()) {
        return QStringLiteral("非法 segSize=1 应返回空包");
    }

    // 上限策略自检：32 个数值可组包；长度应等于 32*segSize+1。
    QVector<double> many(32, 1.0);
    const QByteArray packedMany = packSansiSsck(many, 8);
    if (packedMany.size() != 32 * 8 + 1) {
        return QStringLiteral("32 值组包长度异常：%1").arg(packedMany.size());
    }

    return {};
}

} // namespace SerialFramePreview