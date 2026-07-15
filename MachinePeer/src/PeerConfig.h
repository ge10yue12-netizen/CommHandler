#pragma once

#include <QChar>
#include <QString>

class QSerialPort;

// 试验机运行参数；仅在找不到 peerconfig.ini 时使用结构体内缺省值。
struct PeerConfig {
    QString bindIp = QStringLiteral("127.0.0.1"); // UDP 本机绑定地址
    int bindPort = 19106;                         // UDP 本机绑定端口
    QString peerIp = QStringLiteral("127.0.0.1");  // UDP 对端地址
    int peerPort = 19105;                         // UDP 对端端口

    int serialPortIndex = 4;                      // COM 序号（0 起；4=COM5）
    int baudRate = 115200;                        // 波特率
    int dataBits = 8;                             // 数据位：5/6/8
    QChar parity = QLatin1Char('N');              // 校验：N/E/O/S/M
    QString stopBits = QStringLiteral("1");       // 停止位：1/1.5/2
    int serialSegSize = 8;                        // 三思段长：5/6/8

    QString sourcePath;                           // 实际加载的 ini 路径
    bool fromFile = false;                        // 是否来自文件（否则为内置默认）
};

// 按「当前目录 → exe 目录」查找并加载 peerconfig.ini；始终返回可用配置。
PeerConfig LoadPeerConfig();

// 将线格式写入 QSerialPort（不 Open）；参数非法或驱动拒绝时返回 false。
bool ApplySerialLineFormat(QSerialPort* port,
                           int baudRate,
                           int dataBits,
                           QChar parity,
                           const QString& stopBits);

// 生成线格式摘要文本，例如「115200 8N1」。
QString SerialLineFormatText(int baudRate, int dataBits, QChar parity, const QString& stopBits);
