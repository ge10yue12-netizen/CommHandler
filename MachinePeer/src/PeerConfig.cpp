#include "PeerConfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSerialPort>
#include <QSettings>
#include <QStringList>

namespace {

// 返回 peerconfig.ini 候选路径列表（当前目录、exe 目录）。
QStringList candidatePaths()
{
    QStringList paths;
    const QString name = QStringLiteral("peerconfig.ini");
    paths << QDir::current().absoluteFilePath(name);
    if (!QCoreApplication::applicationDirPath().isEmpty()) {
        paths << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(name);
    }
    paths.removeDuplicates();
    return paths;
}

// 将端口钳位到 1..65535，否则返回 fallback。
int clampPort(int v, int fallback)
{
    return (v >= 1 && v <= 65535) ? v : fallback;
}

// 将波特率钳位到合理区间，否则返回 fallback。
int clampBaud(int v, int fallback)
{
    return (v >= 300 && v <= 4000000) ? v : fallback;
}

// 仅允许 5/6/8 数据位。
int clampDataBits(int v, int fallback)
{
    return (v == 5 || v == 6 || v == 8) ? v : fallback;
}

// 仅允许 5/6/8 三思段长。
int clampSegSize(int v, int fallback)
{
    return (v == 5 || v == 6 || v == 8) ? v : fallback;
}

// 解析校验字母；非法则返回 fallback。
QChar clampParity(QString raw, QChar fallback)
{
    raw = raw.trimmed().toUpper();
    if (raw.isEmpty()) {
        return fallback;
    }
    const QChar c = raw.at(0);
    if (c == QLatin1Char('N') || c == QLatin1Char('E') || c == QLatin1Char('O')
        || c == QLatin1Char('S') || c == QLatin1Char('M')) {
        return c;
    }
    return fallback;
}

// 解析停止位文本；非法则返回 fallback。
QString clampStopBits(QString raw, const QString& fallback)
{
    raw = raw.trimmed();
    if (raw == QLatin1String("1") || raw == QLatin1String("1.5")
        || raw == QLatin1String("2")) {
        return raw;
    }
    return fallback;
}

} // namespace

// 按「当前目录 → exe 目录」查找并加载 peerconfig.ini；始终返回可用配置。
PeerConfig LoadPeerConfig()
{
    PeerConfig cfg;
    for (const QString& path : candidatePaths()) {
        if (!QFileInfo::exists(path)) {
            continue;
        }
        QSettings ini(path, QSettings::IniFormat);
        ini.setIniCodec("UTF-8");

        ini.beginGroup(QStringLiteral("network"));
        const QString bindIp =
            ini.value(QStringLiteral("bind_ip"), cfg.bindIp).toString().trimmed();
        const QString peerIp =
            ini.value(QStringLiteral("peer_ip"), cfg.peerIp).toString().trimmed();
        const int bindPort =
            clampPort(ini.value(QStringLiteral("bind_port"), cfg.bindPort).toInt(),
                      cfg.bindPort);
        const int peerPort =
            clampPort(ini.value(QStringLiteral("peer_port"), cfg.peerPort).toInt(),
                      cfg.peerPort);
        ini.endGroup();

        ini.beginGroup(QStringLiteral("serial"));
        const int portIndex =
            ini.value(QStringLiteral("port_index"), cfg.serialPortIndex).toInt();
        const int baud =
            clampBaud(ini.value(QStringLiteral("baud"), cfg.baudRate).toInt(), cfg.baudRate);
        const int dataBits = clampDataBits(
            ini.value(QStringLiteral("data_bits"), cfg.dataBits).toInt(), cfg.dataBits);
        const QChar parity =
            clampParity(ini.value(QStringLiteral("parity"), QString(cfg.parity)).toString(),
                        cfg.parity);
        const QString stopBits = clampStopBits(
            ini.value(QStringLiteral("stop_bits"), cfg.stopBits).toString(), cfg.stopBits);
        const int segSize = clampSegSize(
            ini.value(QStringLiteral("seg_size"), cfg.serialSegSize).toInt(),
            cfg.serialSegSize);
        ini.endGroup();

        if (!bindIp.isEmpty()) {
            cfg.bindIp = bindIp;
        }
        if (!peerIp.isEmpty()) {
            cfg.peerIp = peerIp;
        }
        cfg.bindPort = bindPort;
        cfg.peerPort = peerPort;
        cfg.serialPortIndex = portIndex;
        cfg.baudRate = baud;
        cfg.dataBits = dataBits;
        cfg.parity = parity;
        cfg.stopBits = stopBits;
        cfg.serialSegSize = segSize;
        cfg.sourcePath = QDir::toNativeSeparators(path);
        cfg.fromFile = true;
        return cfg;
    }
    return cfg;
}

// 将线格式写入 QSerialPort（不 Open）；参数非法或驱动拒绝时返回 false。
bool ApplySerialLineFormat(QSerialPort* port,
                           int baudRate,
                           int dataBits,
                           QChar parity,
                           const QString& stopBits)
{
    if (!port) {
        return false;
    }
    if (!port->setBaudRate(baudRate)) {
        return false;
    }

    QSerialPort::DataBits db = QSerialPort::Data8;
    switch (dataBits) {
    case 5:
        db = QSerialPort::Data5;
        break;
    case 6:
        db = QSerialPort::Data6;
        break;
    case 8:
        db = QSerialPort::Data8;
        break;
    default:
        return false;
    }
    if (!port->setDataBits(db)) {
        return false;
    }

    QSerialPort::Parity py = QSerialPort::NoParity;
    switch (parity.toUpper().toLatin1()) {
    case 'E':
        py = QSerialPort::EvenParity;
        break;
    case 'O':
        py = QSerialPort::OddParity;
        break;
    case 'S':
        py = QSerialPort::SpaceParity;
        break;
    case 'M':
        py = QSerialPort::MarkParity;
        break;
    case 'N':
        py = QSerialPort::NoParity;
        break;
    default:
        return false;
    }
    if (!port->setParity(py)) {
        return false;
    }

    QSerialPort::StopBits sb = QSerialPort::OneStop;
    if (stopBits == QLatin1String("1.5")) {
        sb = QSerialPort::OneAndHalfStop;
    } else if (stopBits == QLatin1String("2")) {
        sb = QSerialPort::TwoStop;
    } else if (stopBits != QLatin1String("1")) {
        return false;
    }
    if (!port->setStopBits(sb)) {
        return false;
    }

    port->setFlowControl(QSerialPort::NoFlowControl);
    return true;
}

// 生成线格式摘要文本，例如「115200 8N1」。
QString SerialLineFormatText(int baudRate, int dataBits, QChar parity, const QString& stopBits)
{
    return QStringLiteral("%1 %2%3%4")
        .arg(baudRate)
        .arg(dataBits)
        .arg(parity.toUpper())
        .arg(stopBits);
}
