#include "ILegacyBackend.h"

#include <QMetaObject>
#include <cstring>

#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
#include "CommHandler.h"
#include "UIDef.h"
#include <array>
#include <vector>
#endif

namespace ca {

namespace {

#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
// COM3 → iComPort=2（库内端口名为 COM{iComPort+1}）
int comPortIndexFromName(const QString& portName)
{
    QString n = portName.trimmed().toUpper();
    n.remove(QStringLiteral("\\\\.\\"));
    if (n.startsWith(QStringLiteral("COM")))
        n = n.mid(3);
    bool ok = false;
    const int num = n.toInt(&ok);
    if (!ok || num <= 0)
        return -1;
    return num - 1;
}

// 实际波特率 → iComBaud 索引（与 ProtoGuide / SerialPortComm switch 对齐）
int baudIndexFromRate(int baud)
{
    switch (baud) {
    case 4800: return 0;
    case 9600: return 1;
    case 19200: return 2;
    case 38400: return 3;
    case 57600: return 4;
    case 115200: return 5;
    default: return 5;
    }
}

// 数据位 → iDataBits：0=5 1=6 2=7 3=8（与 SerialPortComm / UIDef 一致）
int dataBitsIndexFromValue(int bits)
{
    switch (bits) {
    case 5: return 0;
    case 6: return 1;
    case 7: return 2;
    case 8: return 3;
    default: return 3;
    }
}

int parityIndexFromName(const QString& parity)
{
    const QString p = parity.trimmed().toLower();
    if (p == QStringLiteral("even") || p == QStringLiteral("e"))
        return 1;
    if (p == QStringLiteral("odd") || p == QStringLiteral("o"))
        return 2;
    if (p == QStringLiteral("space") || p == QStringLiteral("s"))
        return 3;
    if (p == QStringLiteral("mark") || p == QStringLiteral("m"))
        return 4;
    return 0;
}

int stopBitsIndexFromName(const QString& stop)
{
    const QString s = stop.trimmed();
    if (s == QStringLiteral("1.5"))
        return 1;
    if (s == QStringLiteral("2"))
        return 2;
    return 0;
}
#endif

} // namespace

MockLegacyBackend::MockLegacyBackend(QObject* parent)
    : ILegacyBackend(parent)
{
}

Result MockLegacyBackend::configure(const LegacyConfig& config)
{
    config_ = config;
    return Result::success();
}

Result MockLegacyBackend::connectDevice()
{
    ++connectCount_;
    connected_ = true;
    return Result::success();
}

Result MockLegacyBackend::disconnectDevice()
{
    connected_ = false;
    return Result::success();
}

Result MockLegacyBackend::sendValues(const QVector<double>& values)
{
    if (!connected_)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("Legacy 未连接"));
    ++sendValueCount_;
    lastSentValues_ = values;
    return Result::success();
}

Result MockLegacyBackend::sendText(const QString& text)
{
    if (!connected_)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("Legacy 未连接"));
    // Mock：能力表允许透明文本时由 Session 层门控；此处仅记录
    Q_UNUSED(text);
    ++sendValueCount_;
    return Result::success();
}

void MockLegacyBackend::injectRawPointerData(const QVector<double>& values, int type)
{
    // 模拟 DLL 裸指针：栈上临时缓冲，Direct 路径必须立刻复制
    QVector<double> temp = values;
    void* ptr = temp.data();
    const int size = temp.size();
    if (!ptr || size <= 0 || size > kMaxLegacyValues)
        return;
    QVector<double> owned(size);
    std::memcpy(owned.data(), ptr, static_cast<size_t>(size) * sizeof(double));
    emit valuesReceived(owned, type);
}

void MockLegacyBackend::injectControl(int ctrlCmd, int viewId, int msg)
{
    emit controlEvent(ctrlCmd, viewId, msg);
}

// ---------------- CommHandlerBackend ----------------

CommHandlerBackend::CommHandlerBackend(QObject* parent)
    : ILegacyBackend(parent)
{
}

CommHandlerBackend::~CommHandlerBackend()
{
    destroyHandler();
}

void CommHandlerBackend::destroyHandler()
{
#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
    if (handler_) {
        CommHandler* h = static_cast<CommHandler*>(handler_);
        if (connected_)
            h->Disconnect(1);
        delete h;
        handler_ = nullptr;
    }
#else
    handler_ = nullptr;
#endif
    connected_ = false;
}

Result CommHandlerBackend::configure(const LegacyConfig& config)
{
    config_ = config;
#if !(defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER)
    return Result::fail(QStringLiteral("dll_not_linked"),
                        QStringLiteral("本构建未链接 CommHandler（CA_LINK_COMMHANDLER=0）"));
#else
    destroyHandler();
    CommHandler* h = new CommHandler();
    handler_ = h;
    // DirectConnection：在 Worker 线程立即复制裸指针
    connect(h, &CommHandler::emitNewData, this, &CommHandlerBackend::onEmitNewData, Qt::DirectConnection);
    connect(h, &CommHandler::emitEventMsg, this, &CommHandlerBackend::onEmitEventMsg, Qt::DirectConnection);
    connect(h, &CommHandler::emitEventMsgAndData, this, &CommHandlerBackend::onEmitEventMsgAndData,
            Qt::DirectConnection);
    connect(h, &CommHandler::emitUnparsedRx, this, &CommHandlerBackend::onEmitUnparsedRx,
            Qt::DirectConnection);
    connect(h, &CommHandler::emitClientDisConn, this, &CommHandlerBackend::onClientDisconnected,
            Qt::DirectConnection);

    try {
        if (config.commType == 1) {
            // —— 串口：键名对齐 SerialProtocol / SerialPortComm ——
            const int comIndex = comPortIndexFromName(config.portName);
            if (comIndex < 0)
                throw std::invalid_argument("invalid COM port");

            DataInput input{};
            input.iChannelState = 1;
            input.iChannelSize = 1;
            input.iTriggerCtrl = 1;
            input.iTransferType = 0;

            DataOutput output{};
            output.iChannelState = 1;
            output.iChannelSize = 1;
            output.iTransferType = 0;

            h->SetCommType(SERIAL);
            h->setParameter(QStringLiteral("input"), input);
            h->setParameter(QStringLiteral("output"), output);
            h->setParameter(QStringLiteral("iComPort"), comIndex);
            h->setParameter(QStringLiteral("iComBaud"), baudIndexFromRate(config.baudRate));
            h->setParameter(QStringLiteral("iDataBits"), dataBitsIndexFromValue(config.dataBits));
            h->setParameter(QStringLiteral("iParity"), parityIndexFromName(config.parity));
            h->setParameter(QStringLiteral("iStopBits"), stopBitsIndexFromName(config.stopBits));
            h->setParameter(QStringLiteral("iProtocolType"), config.protocolIndex);
            h->setParameter(QStringLiteral("dInForceRange"), 100.0);
            h->setParameter(QStringLiteral("bInquireSendFlag"), true);
        } else {
            // —— 网口：键名对齐 NetworkProtocol / SocketComm ——
            DataInput input{};
            input.iChannelState = 1;
            input.iChannelSize = 2;
            input.iTriggerCtrl = 1;
            input.iTransferType = 0;
            input.iDataType[0] = 0;
            input.iDataType[1] = 1;

            DataOutput output{};
            output.iChannelState = 1;
            output.iChannelSize = 2;
            output.iTransferType = 0;

            const QString localIp =
                config.localAddress.trimmed().isEmpty() ? QStringLiteral("127.0.0.1")
                                                        : config.localAddress.trimmed();
            const int localPort = (config.localPort == 0) ? 0 : static_cast<int>(config.localPort);

            h->SetCommType(NETWORK);
            h->setParameter(QStringLiteral("input"), input);
            h->setParameter(QStringLiteral("output"), output);
            h->setParameter(QStringLiteral("bOnlineCollect"), false);
            h->setParameter(QStringLiteral("bAllOpenCamera"), true);
            h->setParameter(QStringLiteral("iTransferType"), config.transferType);
            h->setParameter(QStringLiteral("iModel"), config.model);
            h->setParameter(QStringLiteral("sLocalIP"), localIp);
            h->setParameter(QStringLiteral("iLocalPort"), localPort);
            h->setParameter(QStringLiteral("sDestIP"), config.remoteAddress.trimmed());
            h->setParameter(QStringLiteral("iDestPort"), static_cast<int>(config.remotePort));
            h->setParameter(QStringLiteral("iProtoType"), config.protocolIndex);
            h->setParameter(QStringLiteral("bInquireSendFlag"), true);

            // 万测：可配置 START/STOP/EXIT 字面量（与 CommLab NetworkProtocol 一致）
            if (config.protocolIndex == 1) {
                h->setParameter(QStringLiteral("bOpenCMD"), std::array<bool, 3>{{true, true, true}});
                std::array<std::array<char, 16>, 3> commands{};
                std::strncpy(commands[0].data(), "START", 15);
                std::strncpy(commands[1].data(), "STOP", 15);
                std::strncpy(commands[2].data(), "EXIT", 15);
                h->setParameter(QStringLiteral("cCMD"), commands);
            }
        }
    } catch (const std::exception& ex) {
        destroyHandler();
        return Result::fail(QStringLiteral("legacy_set_parameter"),
                            QStringLiteral("setParameter 失败：%1").arg(QString::fromLocal8Bit(ex.what())));
    } catch (...) {
        destroyHandler();
        return Result::fail(QStringLiteral("legacy_set_parameter"), QStringLiteral("setParameter 失败"));
    }
    return Result::success();
#endif
}

Result CommHandlerBackend::connectDevice()
{
#if !(defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER)
    return Result::fail(QStringLiteral("dll_not_linked"), QStringLiteral("未链接 CommHandler"));
#else
    if (!handler_)
        return Result::fail(QStringLiteral("not_configured"), QStringLiteral("请先 configure"));
    CommHandler* h = static_cast<CommHandler*>(handler_);
    if (!h->Connect(-1)) {
        connected_ = false;
        // 客户端被拒/超时、串口打不开、UDP bind 失败等均走此分支
        return Result::fail(QStringLiteral("legacy_connect_failed"),
                            QStringLiteral("连接失败：对端拒绝、超时、地址/端口无效或串口无法打开"));
    }
    connected_ = true;
    return Result::success();
#endif
}

Result CommHandlerBackend::disconnectDevice()
{
#if !(defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER)
    connected_ = false;
    return Result::success();
#else
    if (handler_) {
        static_cast<CommHandler*>(handler_)->Disconnect(0);
    }
    connected_ = false;
    return Result::success();
#endif
}

Result CommHandlerBackend::sendValues(const QVector<double>& values)
{
#if !(defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER)
    Q_UNUSED(values);
    return Result::fail(QStringLiteral("dll_not_linked"), QStringLiteral("未链接 CommHandler"));
#else
    if (!handler_ || !connected_)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("Legacy 未连接"));
    std::vector<double> v;
    v.reserve(static_cast<size_t>(values.size()));
    for (double d : values)
        v.push_back(d);
    const CommType ct = (config_.commType == 1) ? SERIAL : NETWORK;
    static_cast<CommHandler*>(handler_)->SendData(v, ct);
    return Result::success();
#endif
}

Result CommHandlerBackend::sendText(const QString& text)
{
#if !(defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER)
    Q_UNUSED(text);
    return Result::fail(QStringLiteral("dll_not_linked"), QStringLiteral("未链接 CommHandler"));
#else
    if (!handler_ || !connected_)
        return Result::fail(QStringLiteral("not_connected"), QStringLiteral("Legacy 未连接"));
    const CommType ct = (config_.commType == 1) ? SERIAL : NETWORK;
    static_cast<CommHandler*>(handler_)->SendData(text, ct);
    return Result::success();
#endif
}

void CommHandlerBackend::onEmitNewData(void* data, int size, int type)
{
    // Direct 槽：仅边界检查 + 立即复制，禁止 UI/写盘
    if (!data || size <= 0 || size > kMaxLegacyValues)
        return;
    QVector<double> owned(size);
    std::memcpy(owned.data(), data, static_cast<size_t>(size) * sizeof(double));
    emit valuesReceived(owned, type);
}

void CommHandlerBackend::onEmitEventMsg(int ctrlCmd, int viewId, int msg)
{
    emit controlEvent(ctrlCmd, viewId, msg);
}

void CommHandlerBackend::onEmitEventMsgAndData(const int& ctrlCmd, const int& viewId, const int& msg,
                                               const QVariantMap& extra)
{
    emit parameterEvent(ctrlCmd, viewId, msg, extra);
}

void CommHandlerBackend::onEmitUnparsedRx(const QByteArray& raw)
{
    // Direct：立刻拷贝字节，禁止持有 DLL 缓冲；超长截断防 Worker 队列膨胀
    if (raw.isEmpty())
        return;
    QByteArray owned = (raw.size() > kMaxUnparsedBytes) ? raw.left(kMaxUnparsedBytes) : QByteArray(raw);
    emit unparsedRx(owned);
}

void CommHandlerBackend::onClientDisconnected()
{
    connected_ = false;
    emit disconnected();
}

} // namespace ca
