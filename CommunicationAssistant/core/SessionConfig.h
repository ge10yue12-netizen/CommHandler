#pragma once

#include "Types.h"

#include <QMetaType>
#include <QString>
#include <QUuid>

namespace ca {

// 串口参数快照；parity/stopBits 使用文本以便 UI 与配置文件对齐
struct SerialConfig
{
    QString portName;
    int baudRate = 115200;
    int dataBits = 8;
    QString parity = QStringLiteral("none");
    QString stopBits = QStringLiteral("1");
};

struct TcpClientConfig
{
    QString remoteAddress;
    quint16 remotePort = 0;
    QString localAddress;
    quint16 localPort = 0;
};

struct TcpServerConfig
{
    QString localAddress = QStringLiteral("0.0.0.0");
    quint16 localPort = 0;
    bool addressReuse = false;
};

struct UdpConfig
{
    QString localAddress = QStringLiteral("0.0.0.0");
    quint16 localPort = 0;
    QString remoteAddress;
    quint16 remotePort = 0;
    bool addressReuse = false;
};

// M2：Legacy DLL 配置；commType: 0=Network, 1=Serial（Voltage/Pulse 不开放）
// 字段语义对齐 CommLab NetworkProtocol / SerialProtocol → CommHandler setParameter 键名
struct LegacyConfig
{
    int commType = 0;
    int protocolIndex = 0;

    // 串口：UI 用 portName/baudRate；后端映射为 iComPort / iComBaud 索引
    QString portName;
    int baudRate = 115200;
    int dataBits = 8;
    QString parity;   // none/even/odd
    QString stopBits; // 1 / 1.5 / 2

    // 网口：对齐 SocketComm（iTransferType / iModel / sLocalIP / sDestIP …）
    int transferType = 0; // 0=TCP 1=UDP
    int model = 1;        // 0=服务端 1=客户端（助手默认客户端）
    QString remoteAddress;
    quint16 remotePort = 0;
    QString localAddress;
    quint16 localPort = 0;

    bool useMockBackend = false; // true=Mock；false 且 CA_LINK_COMMHANDLER=1 时用真实 DLL
};

// 强类型传输配置联合体（C++14 无 std::variant，以 kind 判别）
struct TransportConfig
{
    TransportKind kind = TransportKind::Serial;
    SerialConfig serial;
    TcpClientConfig tcpClient;
    TcpServerConfig tcpServer;
    UdpConfig udp;
    LegacyConfig legacy;
};

/**
 * @brief 会话级不可变配置快照；configure 成功后 Session 内部持有副本。
 *
 * sessionId 由调用方分配，须在进程内唯一以便 SessionManager 跟踪占用。
 */
struct SessionConfig
{
    QUuid sessionId;
    QString name;
    WorkMode mode = WorkMode::Native;
    SessionRole role = SessionRole::Tool;
    TransportConfig transport;
};

} // namespace ca

Q_DECLARE_METATYPE(ca::LegacyConfig)
