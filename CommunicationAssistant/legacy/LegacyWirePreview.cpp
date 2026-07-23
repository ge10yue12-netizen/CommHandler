#include "LegacyWirePreview.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace ca {
namespace {

// 对齐 SerialPortComm::GetNFromData
float getNFromData(float val, int n)
{
    float temp1 = val;
    const int t1 = static_cast<int>(static_cast<uint32_t>(temp1 * std::pow(10.0f, n + 1)) % 10);
    if (t1 > 4)
        temp1 = (static_cast<int>(temp1 * std::pow(10.0f, n)) + 1) / std::pow(10.0f, n);
    else
        temp1 = static_cast<int>(temp1 * std::pow(10.0f, n)) / std::pow(10.0f, n);
    return temp1;
}

// 对齐 SerialPortComm::FloatToHex（三思定宽 ASCII 段）
void floatToHexSeg(unsigned char* str, int row, float fValue, int segSize)
{
    if (!str || segSize < 2)
        return;
    str[row * segSize] = (fValue >= 0.0f) ? 0x2B : 0x2D;
    int n = segSize - 3;
    for (int i = 1; i < 4; ++i) {
        if (std::fabs(fValue) >= std::pow(10.0, i))
            n = segSize - (i + 3);
    }
    if (n < 0)
        n = 0;
    const float val = getNFromData(std::fabs(fValue), n);
    const std::string data = std::to_string(val);
    for (int i = 0; i < segSize - 1; ++i) {
        const int value1 = (i < static_cast<int>(data.size())) ? static_cast<int>(data[static_cast<size_t>(i)]) : -1;
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

// 对齐 SerialPortComm::FixedDoubleToHex（科新 14 字节）
QByteArray encodeKexin14(double dValue)
{
    unsigned char str[14] = {};
    str[0] = 0x02;
    str[1] = 0x4C;
    const QString sData = QString::number(dValue, 'f', 5);
    const int iIndex = sData.indexOf(QLatin1Char('.'));
    if (iIndex <= 0) {
        for (int i = 2; i < 13; ++i) {
            if (i == 7)
                str[i] = 0x2E;
            else
                str[i] = 0x30;
        }
    } else {
        int iStartPos = 0;
        if (iIndex > 4)
            iStartPos = iIndex - 5;
        QString sInteger = sData.mid(iStartPos, iIndex - iStartPos);
        if (dValue < 0)
            sInteger = sInteger.mid(1);
        QString sDecimal = (iIndex + 1 < sData.size()) ? sData.mid(iIndex + 1) : QString();
        while (sInteger.size() < 5)
            sInteger.prepend(QLatin1Char('0'));
        while (sDecimal.size() < 5)
            sDecimal.append(QLatin1Char('0'));
        for (int i = 2; i < 7; ++i) {
            if (dValue < 0 && i == 2)
                str[i] = 0x2D;
            else
                str[i] = static_cast<unsigned char>(sInteger.at(i - 2).toLatin1());
        }
        str[7] = 0x2E;
        for (int i = 8; i < 13; ++i)
            str[i] = static_cast<unsigned char>(sDecimal.at(i - 8).toLatin1());
    }
    str[13] = 0x23;
    return QByteArray(reinterpret_cast<const char*>(str), 14);
}

// 对齐 SerialPortComm::formatValue（冠腾）
QString formatGuantengValue(double value)
{
    const QChar sign = (value >= 0.0) ? QLatin1Char('+') : QLatin1Char('-');
    const double absVal = std::fabs(value);
    // 宽度 8、小数 4、左补 0；内容超宽时 Qt 不截断（与库一致）
    const QString numPart = QStringLiteral("%1").arg(absVal, 8, 'f', 4, QLatin1Char('0'));
    return QStringLiteral("%1%2").arg(sign).arg(numPart);
}

#pragma pack(push, 1)
struct ZhongjiServerData
{
    quint8 msgType;
    double valueX;
    double valueY;
};
#pragma pack(pop)

QByteArray encodeZhongji(double x, double y)
{
    ZhongjiServerData d{};
    d.msgType = 0x04;
    d.valueX = x;
    d.valueY = y;
    return QByteArray(reinterpret_cast<const char*>(&d), static_cast<int>(sizeof(d)));
}

// CRC16-XMODEM（联恒串口/网口共用算法）
quint16 crc16Xmodem(const quint8* data, size_t length)
{
    quint16 crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<quint16>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? static_cast<quint16>((crc << 1) ^ 0x1021) : static_cast<quint16>(crc << 1);
    }
    return crc;
}

void appendU16LE(std::vector<quint8>& buf, quint16 v)
{
    buf.push_back(static_cast<quint8>(v & 0xFF));
    buf.push_back(static_cast<quint8>((v >> 8) & 0xFF));
}

void appendU16BE(std::vector<quint8>& buf, quint16 v)
{
    buf.push_back(static_cast<quint8>((v >> 8) & 0xFF));
    buf.push_back(static_cast<quint8>(v & 0xFF));
}

void appendDoubleLE(std::vector<quint8>& buf, double d)
{
    quint64 raw = 0;
    std::memcpy(&raw, &d, sizeof(d));
    for (int i = 0; i < 8; ++i)
        buf.push_back(static_cast<quint8>((raw >> (i * 8)) & 0xFF));
}

void appendDoubleBE(std::vector<quint8>& buf, double d)
{
    quint64 raw = 0;
    std::memcpy(&raw, &d, sizeof(d));
    for (int i = 7; i >= 0; --i)
        buf.push_back(static_cast<quint8>((raw >> (i * 8)) & 0xFF));
}

QByteArray toQByteArray(const std::vector<quint8>& v)
{
    return QByteArray(reinterpret_cast<const char*>(v.data()), static_cast<int>(v.size()));
}

// 对齐 SerLhgkProtocol::buildExternalDataFrame（串口，长度=payload 字节数，LE）
QByteArray encodeSerLhgk(double force, double temp)
{
    std::vector<quint8> payload;
    payload.push_back(0x06);
    payload.push_back(0x0C);
    appendDoubleLE(payload, force);
    payload.push_back(0x06);
    payload.push_back(0x0D);
    appendDoubleLE(payload, temp);

    std::vector<quint8> buf = {0xAA, 0x55, 0x01};
    appendU16LE(buf, static_cast<quint16>(payload.size()));
    buf.insert(buf.end(), payload.begin(), payload.end());
    const quint16 crc = crc16Xmodem(buf.data(), buf.size());
    buf.push_back(static_cast<quint8>(crc & 0xFF));
    buf.push_back(static_cast<quint8>((crc >> 8) & 0xFF));
    buf.push_back(0x0D);
    buf.push_back(0x0A);
    return toQByteArray(buf);
}

// 对齐 NetLhgkProtocol::buildExternalDataFrame（网口 TX：段计数 + 0x0B/0x0C，BE）
QByteArray encodeNetLhgk(double force, double temp)
{
    std::vector<quint8> payload;
    payload.push_back(0x02);
    payload.push_back(0x06);
    payload.push_back(0x0B);
    appendDoubleBE(payload, force);
    payload.push_back(0x06);
    payload.push_back(0x0C);
    appendDoubleBE(payload, temp);

    std::vector<quint8> buf = {0xAA, 0x55, 0x01};
    appendU16BE(buf, static_cast<quint16>(payload.size()));
    buf.insert(buf.end(), payload.begin(), payload.end());
    const quint16 crc = crc16Xmodem(buf.data(), buf.size());
    buf.push_back(static_cast<quint8>((crc >> 8) & 0xFF));
    buf.push_back(static_cast<quint8>(crc & 0xFF));
    buf.push_back(0x0D);
    buf.push_back(0x0A);
    return toQByteArray(buf);
}

} // namespace

LegacyWirePreview previewLegacySendWire(LegacyCommKind kind, int protocolIndex,
                                        const QVector<double>& values, int dataBits,
                                        const QString& textPayload)
{
    LegacyWirePreview out;
    if (!textPayload.isEmpty() && values.isEmpty()) {
        out.bytes = textPayload.toUtf8();
        out.binary = false;
        out.note = QStringLiteral("透明文本：按 UTF-8 原样写出");
        return out;
    }
    if (values.isEmpty()) {
        out.note = QStringLiteral("无有效数值入参，未生成发送字节");
        return out;
    }

    if (kind == LegacyCommKind::Serial) {
        switch (protocolIndex) {
        case 0: {
            const int seg = qBound(5, dataBits > 0 ? dataBits : 8, 8);
            const int size = seg * values.size() + 1;
            QByteArray buf(size, '\0');
            auto* p = reinterpret_cast<unsigned char*>(buf.data());
            for (int i = 0; i < values.size(); ++i)
                floatToHexSeg(p, i, static_cast<float>(values.at(i)), seg);
            p[seg * values.size()] = 0x0D;
            out.bytes = buf;
            out.binary = false;
            out.note = QStringLiteral("串口三思：定宽 ASCII 线帧");
            return out;
        }
        case 1: {
            out.bytes = encodeKexin14(values.at(0));
            out.binary = true;
            out.note = QStringLiteral("串口科新：14 字节定长帧（仅第 1 路）");
            return out;
        }
        case 2: {
            out.note = QStringLiteral("串口时代新材：本协议不支持数值发送，链路无写出");
            return out;
        }
        case 3: {
            out.binary = true;
            out.note = QStringLiteral("串口 IEEE：写出 56 字节二进制包（界面不重建逐字节预览）");
            return out;
        }
        case 4: {
            if (values.size() < 2) {
                out.note = QStringLiteral("串口冠腾：入参不足 2 路，未生成发送字节");
                return out;
            }
            const QString frame = QStringLiteral("R%1#N%2#")
                                      .arg(formatGuantengValue(values.at(0)))
                                      .arg(formatGuantengValue(values.at(1)));
            out.bytes = frame.toLatin1();
            out.binary = false;
            out.note = QStringLiteral("串口冠腾：ASCII 线帧 R…#N…#");
            return out;
        }
        case 5: {
            if (values.size() < 2) {
                out.note = QStringLiteral("串口联恒：入参不足 2 路（力、温度），未生成发送字节");
                return out;
            }
            out.bytes = encodeSerLhgk(values.at(0), values.at(1));
            out.binary = true;
            out.note = QStringLiteral("串口联恒：AA55 二进制帧（CRC16-XMODEM，小端）");
            return out;
        }
        default:
            out.note = QStringLiteral("未知串口协议索引，未生成发送字节");
            return out;
        }
    }

    switch (protocolIndex) {
    case 0: {
        QJsonObject obj;
        QJsonArray arr;
        for (double v : values)
            arr.append(QString::number(v, 'f', 12).toDouble());
        obj.insert(QStringLiteral("code"), 0);
        obj.insert(QStringLiteral("values"), arr);
        obj.insert(QStringLiteral("tn"), 2);
        out.bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        out.binary = false;
        out.note = QStringLiteral("网口 JSON：数值包 tn=2");
        return out;
    }
    case 1: {
        QStringList parts;
        for (double v : values)
            parts << QString::number(v);
        out.bytes = parts.join(QLatin1Char(',')).toUtf8();
        out.binary = false;
        out.note = QStringLiteral("网口万测：分隔符数值文本（默认逗号）");
        return out;
    }
    case 2: {
        if (values.size() != 2) {
            out.note = QStringLiteral("网口中机：入参须恰好 2 路，未生成发送字节");
            return out;
        }
        out.bytes = encodeZhongji(values.at(0), values.at(1));
        out.binary = true;
        out.note = QStringLiteral("网口中机：17 字节定长二进制");
        return out;
    }
    case 3: {
        out.note = QStringLiteral("网口三思：本协议不支持数值发送，链路无写出");
        return out;
    }
    case 4:
        out.note = QStringLiteral("网口触发存图：无数值发送分支，链路无写出");
        return out;
    case 5:
        out.note = QStringLiteral("网口福建威盛：无数值发送分支，链路无写出");
        return out;
    case 6: {
        QJsonObject obj;
        QJsonArray arr;
        for (double v : values)
            arr.append(v);
        obj.insert(QStringLiteral("values"), arr);
        obj.insert(QStringLiteral("name"), QStringLiteral("LVEComputedValues"));
        out.bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        out.binary = false;
        out.note = QStringLiteral("网口纳百川自动化：JSON 数值包");
        return out;
    }
    case 7: {
        if (values.size() != 4) {
            out.note = QStringLiteral("网口纳百川线条：入参须恰好 4 路，未生成发送字节");
            return out;
        }
        QJsonObject hsm;
        hsm.insert(QStringLiteral("L2"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("b2"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("Le2"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("bo2"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("bu"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("val"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("YMaxval"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("XMaxval"), QStringLiteral("0"));
        hsm.insert(QStringLiteral("L1"), QString::number(values.at(0)));
        hsm.insert(QStringLiteral("b1"), QString::number(values.at(1)));
        hsm.insert(QStringLiteral("Le1"), QString::number(values.at(2)));
        hsm.insert(QStringLiteral("bo1"), QString::number(values.at(3)));
        QJsonObject root;
        root.insert(QStringLiteral("hsm"), hsm);
        out.bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
        out.binary = false;
        out.note = QStringLiteral("网口纳百川线条：JSON（L1/b1/Le1/bo1）");
        return out;
    }
    case 8: {
        if (values.size() < 2) {
            out.note = QStringLiteral("网口联恒：入参不足 2 路（力、温度），未生成发送字节");
            return out;
        }
        out.bytes = encodeNetLhgk(values.at(0), values.at(1));
        out.binary = true;
        out.note = QStringLiteral("网口联恒：AA55 二进制帧（CRC16-XMODEM，大端）");
        return out;
    }
    default:
        out.note = QStringLiteral("未知网口协议索引，未生成发送字节");
        return out;
    }
}

QStringList legacyDataBoundaryTips(LegacyCommKind kind, int protocolIndex, int dataBits)
{
    QStringList tips;
    const int seg = qBound(5, dataBits > 0 ? dataBits : 8, 8);

    if (kind == LegacyCommKind::Serial) {
        switch (protocolIndex) {
        case 0:
            tips << QStringLiteral("发送：每路定宽 %1 字节 ASCII（首字节符号「+/−」，其后 %2 字节为数字/小数点），全帧末尾附加 0x0D（CR）")
                        .arg(seg)
                        .arg(seg - 1);
            tips << QStringLiteral("数值：按 float 精度编码；有效数字约 7 位，超长整数截断（如 12321239 → +1232123）");
            tips << QStringLiteral("接收：按 0x0D 定界拼帧后解析为业务数值");
            tips << QStringLiteral("入参：逗号分隔十进制数，路数 ≥ 1");
            tips << QStringLiteral("示例：12.3,45.6 → +12.3000…\\r（数据位=%1 时每路 %2 字节定宽拼接，末尾 CR）")
                        .arg(seg)
                        .arg(seg);
            break;
        case 1:
            tips << QStringLiteral("发送：定长 14 字节（02 4C + 11 字节 ASCII 数值 + 23「#」）；仅编码第 1 个数值");
            tips << QStringLiteral("接收：同构 14 字节力值；控制字 {QLI[1]} 开始 / {QLI[2]} 停止");
            tips << QStringLiteral("入参：至少 1 个数值；其后数值忽略");
            tips << QStringLiteral("示例：12.3 → 02 4C 30 30 30 31 32 2E 33 30 30 30 30 23（可见段 L00012.30000#）");
            break;
        case 2:
            tips << QStringLiteral("发送：不支持数值写出；发起发送将被拒绝，链路无字节写出");
            tips << QStringLiteral("接收：文本行 value,num,TYPE,flag\\r\\n；取第 1 段为测量值");
            tips << QStringLiteral("示例（接收）：12.3,1,TYPE,0\\r\\n → 测量值 12.3");
            break;
        case 3:
            tips << QStringLiteral("发送：56 字节二进制打包帧");
            tips << QStringLiteral("入参：不少于 5 个数值");
            tips << QStringLiteral("接收：以控制字为主（如 24 00 0D 0A / 24 FF 0D 0A）");
            tips << QStringLiteral("示例：1,2,3,4,5 → 56 字节二进制帧（数据显示区不重建逐字节预览，请以对端 HEX 核对）");
            break;
        case 4:
            tips << QStringLiteral("发送：ASCII 帧格式 R{符号+数值}#N{符号+数值}#");
            tips << QStringLiteral("数值字段：符号 1 字节 + 十进制体（固定小数 4 位、最小宽度 8、不足左补 0；整数位过长不截断）");
            tips << QStringLiteral("入参：不少于 2 个数值，依次对应 R、N");
            tips << QStringLiteral("示例：12.3,45.6 → R+012.3000#N+045.6000#");
            tips << QStringLiteral("接收：本协议无专用数值回包解析；回显可能记为未解析");
            break;
        case 5:
            tips << QStringLiteral("发送/接收：帧头 AA 55，类型 01，长度=负载字节数（小端），负载含力(0x0C)/温(0x0D)，CRC16-XMODEM（小端），尾 0D 0A");
            tips << QStringLiteral("入参：不少于 2 个数值（第 1 路力、第 2 路温度）");
            tips << QStringLiteral("边界：完整坏帧记未解析；断帧等待，不误报");
            tips << QStringLiteral("示例：12.3,45.6 → AA 55 01 14 00 … 0D 0A（二进制；数据显示以十六进制展示）");
            break;
        default:
            tips << QStringLiteral("未登记该串口协议索引的收发边界");
            break;
        }
        return tips;
    }

    switch (protocolIndex) {
    case 0:
        tips << QStringLiteral("发送：UTF-8 JSON，字段 code=0、values=[…]、tn=2；长度随路数变化");
        tips << QStringLiteral("接收：控制类 JSON（如 tn=1/3）；非数值流");
        tips << QStringLiteral("入参：逗号分隔数值；亦支持透明文本（能力允许时）");
        tips << QStringLiteral("示例：12.3,45.6 → {\"code\":0,\"values\":[12.3,45.6],\"tn\":2}");
        break;
    case 1:
        tips << QStringLiteral("发送：十进制数值以分隔符拼接的文本（默认逗号）");
        tips << QStringLiteral("接收：控制字为主");
        tips << QStringLiteral("入参：逗号分隔数值；亦支持透明文本（能力允许时）");
        tips << QStringLiteral("示例：12.3,45.6 → 12.3,45.6");
        break;
    case 2:
        tips << QStringLiteral("发送：定长 17 字节（1 字节类型 0x04 + 2×IEEE754 小端 double）");
        tips << QStringLiteral("接收：定长结构（力值等）；长度不符记未解析");
        tips << QStringLiteral("入参：恰好 2 个数值");
        tips << QStringLiteral("示例：12.3,45.6 → 04 + 小端 double(12.3) + 小端 double(45.6)（共 17 字节）");
        break;
    case 3:
        tips << QStringLiteral("发送：不支持数值写出；发起发送将被拒绝，链路无字节写出");
        tips << QStringLiteral("接收：定长 Data 结构");
        tips << QStringLiteral("示例：本协议仅接收，无发送线帧");
        break;
    case 4:
        tips << QStringLiteral("发送：不支持数值写出；发起发送将被拒绝");
        tips << QStringLiteral("接收：控制/参数事件（行尾 \\r\\n）");
        tips << QStringLiteral("示例（接收）：控制/参数文本行…\\r\\n");
        break;
    case 5:
        tips << QStringLiteral("发送：不支持数值写出；发起发送将被拒绝");
        tips << QStringLiteral("接收：威盛文本解析（如 T/S…L…）");
        tips << QStringLiteral("示例（接收）：T/S…L… 文本行");
        break;
    case 6:
        tips << QStringLiteral("发送：JSON {\"values\":[…],\"name\":\"LVEComputedValues\"}");
        tips << QStringLiteral("接收：控制/参数事件");
        tips << QStringLiteral("入参：逗号分隔数值");
        tips << QStringLiteral("示例：12.3,45.6 → {\"values\":[12.3,45.6],\"name\":\"LVEComputedValues\"}");
        break;
    case 7:
        tips << QStringLiteral("发送：JSON 对象 hsm，写入 L1/b1/Le1/bo1");
        tips << QStringLiteral("入参：恰好 4 个数值，依次对应 L1、b1、Le1、bo1");
        tips << QStringLiteral("接收：控制事件（calcStart / calcEnd）");
        tips << QStringLiteral("示例：12.3,45.6,1,2 → {\"hsm\":{\"L1\":\"12.3\",\"b1\":\"45.6\",\"Le1\":\"1\",\"bo1\":\"2\",…}}");
        break;
    case 8:
        tips << QStringLiteral("发送：帧头 AA 55，类型 01，长度=负载字节数（大端），负载含段计数与力(0x0B)/温(0x0C)，CRC16-XMODEM（大端），尾 0D 0A");
        tips << QStringLiteral("入参：不少于 2 个数值（第 1 路力、第 2 路温度）");
        tips << QStringLiteral("注意：发送与接收字段布局不同，请勿将发送帧直接环回作为接收正例");
        tips << QStringLiteral("示例：12.3,45.6 → AA 55 01 … 0D 0A（二进制；数据显示以十六进制展示）");
        break;
    default:
        tips << QStringLiteral("未登记该网口协议索引的收发边界");
        break;
    }
    return tips;
}

} // namespace ca
