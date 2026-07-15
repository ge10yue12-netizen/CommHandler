#include "WireCodec.h"

#include <QStringList>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace WireCodec {
namespace {

// 按小数位数 n 做四舍五入截断，供三思 ASCII 组包使用。
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

// 将单个浮点写入三思段：首字节正负号，其余为 ASCII 数字 / 小数点。
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

// 组串口三思数值帧：每值 segSize 字节，帧末 0x0D；非法参数返回空。
QByteArray packSansiSerial(const QVector<double>& values, int segSize)
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

// 组 UDP 三思测量包：力 / 位移 / 应变三个 double，共 24 字节。
QByteArray packSansiUdp(double force, double displacement, double strain)
{
    QByteArray packet(24, Qt::Uninitialized);
    double fields[3] = {force, displacement, strain};
    std::memcpy(packet.data(), fields, sizeof(fields));
    return packet;
}

// 从累计缓冲按 0x0D 切出完整帧，并从前端移除已消费字节。
QVector<QByteArray> takeFramesEndingCr(QByteArray* buffer)
{
    QVector<QByteArray> frames;
    if (!buffer || buffer->isEmpty()) {
        return frames;
    }
    int start = 0;
    for (int i = 0; i < buffer->size(); ++i) {
        if (static_cast<unsigned char>(buffer->at(i)) != 0x0D) {
            continue;
        }
        frames.push_back(buffer->mid(start, i - start + 1));
        start = i + 1;
    }
    if (start > 0) {
        buffer->remove(0, start);
    }
    return frames;
}

// 识别是否为二字节控制指令；否则返回 Unknown。
SerialCommand classifySerialCommand(const QByteArray& frame)
{
    if (frame.size() != 2) {
        return SerialCommand::Unknown;
    }
    if (static_cast<unsigned char>(frame.at(1)) != 0x0D) {
        return SerialCommand::Unknown;
    }
    switch (static_cast<unsigned char>(frame.at(0))) {
    case 0x3D:
        return SerialCommand::StartCalc;
    case 0x3E:
        return SerialCommand::StopCalc;
    case 0x6B:
        return SerialCommand::SaveImage;
    case 0x5A:
        return SerialCommand::ZeroClear;
    default:
        return SerialCommand::Unknown;
    }
}

// 控制指令的中文名称。
QString serialCommandName(SerialCommand cmd)
{
    switch (cmd) {
    case SerialCommand::StartCalc:
        return QStringLiteral("开始计算");
    case SerialCommand::StopCalc:
        return QStringLiteral("停止计算");
    case SerialCommand::SaveImage:
        return QStringLiteral("自动存图");
    case SerialCommand::ZeroClear:
        return QStringLiteral("清零");
    default:
        return QStringLiteral("未知指令");
    }
}

// 组控制指令原始字节（命令码 + 0x0D）。
QByteArray packSerialCommand(SerialCommand cmd)
{
    char code = 0;
    switch (cmd) {
    case SerialCommand::StartCalc:
        code = 0x3D;
        break;
    case SerialCommand::StopCalc:
        code = 0x3E;
        break;
    case SerialCommand::SaveImage:
        code = 0x6B;
        break;
    case SerialCommand::ZeroClear:
        code = 0x5A;
        break;
    default:
        return {};
    }
    QByteArray out;
    out.append(code);
    out.append(static_cast<char>(0x0D));
    return out;
}

// 字节序列转空格分隔大写十六进制文本。
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

} // namespace WireCodec
