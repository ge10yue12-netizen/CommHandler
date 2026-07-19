// MainWindow.cpp — MachinePeer 试验机主窗口实现
#include "MainWindow.h"

#include "NetworkProtocol.h"
#include "ProtoGuide.h"
#include "SerialProtocol.h"

#include <QDateTime>

// 构造：通道默认串口；指令与测数分槽；挂接收包解析
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(QStringLiteral("MachinePeer — 试验机"));
    ui.cmbCommType->clear();
    ui.cmbCommType->addItems({QStringLiteral("网口"), QStringLiteral("串口")});
    ui.cmbCommType->setCurrentIndex(1);
    ProtoGuide::fillSerialCombos(ui.cmbBaud, ui.cmbDataBits, ui.cmbParity, ui.cmbStopBits);
    refillProtoCombo();
    m_cfg = LoadPeerConfig();
    ui.editLocalIp->setText(m_cfg.localIp);
    ui.editDestIp->setText(m_cfg.destIp);
    ui.spinLocalPort->setValue(m_cfg.localPort);
    ui.spinDestPort->setValue(m_cfg.destPort);
    ui.spinComPort->setValue(m_cfg.comPort);

    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &MainWindow::onChannelChanged);
    QObject::connect(ui.cmbProto, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     [this](int) { syncUi(); });
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [this](int v) { ui.lblComEcho->setText(QStringLiteral("COM%1").arg(v + 1)); });
    QObject::connect(ui.btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    QObject::connect(ui.btnClose, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    QObject::connect(ui.btnSendMeasure, &QPushButton::clicked, this, &MainWindow::onSendMeasureClicked);
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, ui.textLog, &QPlainTextEdit::clear);
    QObject::connect(ui.btnCmdStart, &QPushButton::clicked, this, &MainWindow::onCmdStart);
    QObject::connect(ui.btnCmdStop, &QPushButton::clicked, this, &MainWindow::onCmdStop);
    QObject::connect(ui.btnCmdSave, &QPushButton::clicked, this, &MainWindow::onCmdSave);
    QObject::connect(ui.btnCmdZero, &QPushButton::clicked, this, &MainWindow::onCmdZero);
    QObject::connect(&m_channels, &PeerChannelManager::serialBytesReceived,
                     this, &MainWindow::onSerialBytesReceived);
    QObject::connect(&m_channels, &PeerChannelManager::networkDatagramReceived,
                     this, &MainWindow::onNetworkDatagramReceived);

    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
    syncUi();
}

// 析构：关闭串口与 UDP
MainWindow::~MainWindow()
{
    closeAll();
}

// 收发区追加带时戳日志行
void MainWindow::log(const QString& line)
{
    ui.textLog->appendPlainText(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line);
}

// 按当前通道重填协议下拉并尽量保留原索引
void MainWindow::refillProtoCombo()
{
    const int keep = ui.cmbProto->currentIndex();
    if (isNetwork())
        ProtoGuide::fillNetProto(ui.cmbProto);
    else
        ProtoGuide::fillSerialProto(ui.cmbProto);
    if (keep >= 0 && keep < ui.cmbProto->count())
        ui.cmbProto->setCurrentIndex(keep);
}

// 通道切换：未打开时可换协议列表，并刷新使能
void MainWindow::onChannelChanged(int)
{
    if (!m_ready)
        refillProtoCombo();
    syncUi();
}

// 按库能力使能开始/停止/存图/清零/发力温等按钮
void MainWindow::syncUi()
{
    // true：当前为网口
    const bool net = isNetwork();
    // 当前协议索引
    const int p = proto();
    ui.serialPanel->setVisible(!net);
    ui.udpPanel->setVisible(net);
    ui.btnOpen->setEnabled(!m_ready);
    ui.btnClose->setEnabled(m_ready);
    ui.cmbCommType->setEnabled(!m_ready);
    ui.cmbProto->setEnabled(!m_ready);
    ui.udpPanel->setEnabled(!m_ready);
    ui.serialPanel->setEnabled(!m_ready);
    ui.groupOps->setEnabled(m_ready);

    // true：库支持开始指令
    const bool canStart = m_ready && (net ? NetworkProtocol::canSendCommand(p, 0)
                                          : SerialProtocol::canSendCommand(p, 0));
    // true：库支持停止指令
    const bool canStop = m_ready && (net ? NetworkProtocol::canSendCommand(p, 1)
                                         : SerialProtocol::canSendCommand(p, 1));
    // true：库支持存图指令
    const bool canSave = m_ready && (net ? NetworkProtocol::canSendCommand(p, 2)
                                         : SerialProtocol::canSendCommand(p, 2));
    // true：库支持清零指令
    const bool canZero = m_ready && (net ? NetworkProtocol::canSendCommand(p, 3)
                                         : SerialProtocol::canSendCommand(p, 3));
    // true：库支持测数入站
    const bool canMeas = m_ready && (net ? NetworkProtocol::canSendMeasurement(p)
                                         : SerialProtocol::canSendMeasurement(p));

    ui.btnCmdStart->setEnabled(canStart);
    ui.btnCmdStop->setEnabled(canStop);
    ui.btnCmdSave->setEnabled(canSave);
    ui.btnCmdZero->setEnabled(canZero);
    ui.btnSendMeasure->setEnabled(canMeas);
    ui.lblStatus->setText(m_ready ? QStringLiteral("已打开") : QStringLiteral("未打开"));
}

// 关闭串口与 UDP，并清空粘包缓冲
void MainWindow::closeAll()
{
    m_channels.close();
    m_serialRxBuf.clear();
}

// 解析软件回包：ACK / 仅力 / 力+温 分记日志
void MainWindow::handleSoftReply(const QByteArray& raw)
{
    if (raw.isEmpty())
        return;
    const ProtocolResult pr = isNetwork() ? NetworkProtocol::parseReply(proto(), raw)
                                          : SerialProtocol::parseReply(proto(), raw);
    if (!pr.ok) {
        log(QStringLiteral("RX 未解析 %1").arg(pr.detail));
        return;
    }
    if (pr.isAck) {
        log(QStringLiteral("RX ACK %1").arg(pr.detail));
        return;
    }
    if (pr.hasValues && pr.hasTemp) {
        log(QStringLiteral("RX 解析 力=%1 温=%2")
                .arg(pr.force, 0, 'g', 12)
                .arg(pr.temp, 0, 'g', 12));
    } else if (pr.hasValues) {
        // 科新等：库回发无温度字段，禁止打印温=0
        log(QStringLiteral("RX 解析 力=%1").arg(pr.force, 0, 'g', 12));
    } else {
        log(QStringLiteral("RX 解析 %1").arg(pr.detail));
    }
}

// 从编辑框读取力/温；非法时回落缺省 1.0 / 25.0
bool MainWindow::readForceTemp(double* force, double* temp) const
{
    // true：力字段解析成功
    bool okF = false;
    // true：温字段解析成功
    bool okT = false;
    *force = ui.editForce->text().trimmed().toDouble(&okF);
    *temp = ui.editTemp->text().trimmed().toDouble(&okT);
    if (!okF)
        *force = 1.0;
    if (!okT)
        *temp = 25.0;
    return true;
}

// 按协议组指令帧并经当前通道写出
void MainWindow::sendCmd(int cmd)
{
    if (!m_ready)
        return;
    // 组包失败原因
    QString fail;
    if (isNetwork()) {
        const QByteArray j = NetworkProtocol::packCommand(proto(), cmd, &fail);
        if (j.isEmpty()) {
            log(QStringLiteral("ERR %1").arg(fail));
            return;
        }
        if (m_channels.sendNetwork(j, &fail)) {
            if (proto() == 2)
                log(QStringLiteral("TX  %1").arg(QString::fromLatin1(j.toHex().toUpper())));
            else
                log(QStringLiteral("TX  %1").arg(QString::fromUtf8(j)));
        } else
            log(QStringLiteral("ERR %1").arg(fail));
    } else {
        const QByteArray frame = SerialProtocol::packCommand(proto(), cmd, &fail);
        if (frame.isEmpty()) {
            log(QStringLiteral("ERR %1").arg(fail));
            return;
        }
        // 串口发出的 HEX 文本
        QString hex;
        if (m_channels.sendSerial(frame, &hex, &fail))
            log(QStringLiteral("TX  %1").arg(hex));
        else
            log(QStringLiteral("ERR %1").arg(fail));
    }
}

// 从串口面板生成完整初始化参数
PeerSerialSettings MainWindow::serialSettings() const
{
    PeerSerialSettings settings;
    settings.portIndex = ui.spinComPort->value();
    settings.baudIndex = ui.cmbBaud->currentIndex();
    settings.dataBitsIndex = ui.cmbDataBits->currentData().toInt();
    settings.parityIndex = ui.cmbParity->currentIndex();
    settings.stopBitsIndex = ui.cmbStopBits->currentIndex();
    return settings;
}

// 从网口面板生成完整初始化参数
PeerNetworkSettings MainWindow::networkSettings() const
{
    PeerNetworkSettings settings;
    settings.localIp = ui.editLocalIp->text().trimmed();
    settings.localPort = ui.spinLocalPort->value();
    settings.destinationIp = ui.editDestIp->text().trimmed();
    settings.destinationPort = ui.spinDestPort->value();
    return settings;
}

// 通过 PeerChannelManager 打开当前通道
void MainWindow::onOpenClicked()
{
    QString fail;
    m_ready = isNetwork()
        ? m_channels.openNetwork(networkSettings(), &fail)
        : m_channels.openSerial(serialSettings(), &fail);
    if (!m_ready)
        log(QStringLiteral("ERR %1").arg(fail));
    m_serialRxBuf.clear();
    syncUi();
}

// 关闭当前通道
void MainWindow::onCloseClicked()
{
    closeAll();
    m_ready = false;
    syncUi();
}

// 发送开始指令（不连带发测数）
void MainWindow::onCmdStart()
{
    sendCmd(0);
}

// 发送停止指令
void MainWindow::onCmdStop()
{
    sendCmd(1);
}

// 发送存图指令
void MainWindow::onCmdSave()
{
    sendCmd(2);
}

// 发送清零指令
void MainWindow::onCmdZero()
{
    sendCmd(3);
}

// 仅发送测数帧（力/温），与指令分离
void MainWindow::onSendMeasureClicked()
{
    if (!m_ready)
        return;
    // 待发力
    double force = 1.0;
    // 待发温
    double temp = 25.0;
    readForceTemp(&force, &temp);
    // 组包失败原因
    QString fail;
    if (isNetwork()) {
        const QByteArray j = NetworkProtocol::packMeasurement(proto(), force, temp, &fail);
        if (j.isEmpty()) {
            log(QStringLiteral("ERR %1").arg(fail));
            return;
        }
        if (m_channels.sendNetwork(j, &fail)) {
            if (proto() == 2 || proto() == 3)
                log(QStringLiteral("TX  %1").arg(QString::fromLatin1(j.toHex().toUpper())));
            else
                log(QStringLiteral("TX  %1").arg(QString::fromUtf8(j)));
        } else
            log(QStringLiteral("ERR %1").arg(fail));
    } else {
        const QByteArray frame = SerialProtocol::packMeasurement(proto(), force, temp, &fail);
        if (frame.isEmpty()) {
            log(QStringLiteral("ERR %1").arg(fail));
            return;
        }
        // 串口发出的 HEX 文本
        QString hex;
        if (m_channels.sendSerial(frame, &hex, &fail))
            log(QStringLiteral("TX  %1").arg(hex));
        else
            log(QStringLiteral("ERR %1").arg(fail));
    }
}

// 串口收软件回传：三思按 0x0D 组帧；科新按 14 字节定长切帧
void MainWindow::onSerialBytesReceived(QByteArray chunk)
{
    if (chunk.isEmpty())
        return;
    log(QStringLiteral("RX  %1").arg(QString::fromLatin1(chunk.toHex().toUpper())));
    if (proto() == 0) {
        m_serialRxBuf.append(chunk);
        int pos = 0;
        while ((pos = m_serialRxBuf.indexOf(char(0x0D))) >= 0) {
            const QByteArray frame = m_serialRxBuf.left(pos + 1);
            m_serialRxBuf.remove(0, pos + 1);
            handleSoftReply(frame);
        }
    } else if (proto() == 1) {
        m_serialRxBuf.append(chunk);
        const QList<QByteArray> frames = SerialProtocol::takeKexinReplyFrames(&m_serialRxBuf);
        for (const QByteArray& frame : frames)
            handleSoftReply(frame);
    } else if (proto() == 3) {
        m_serialRxBuf.append(chunk);
        const QList<QByteArray> frames = SerialProtocol::takeIeeeReplyFrames(&m_serialRxBuf);
        for (const QByteArray& frame : frames)
            handleSoftReply(frame);
    } else {
        handleSoftReply(chunk);
    }
}

// UDP 收软件回传：记原文或 HEX，并按协议解析业务量
void MainWindow::onNetworkDatagramReceived(QByteArray datagram)
{
    if (NetworkProtocol::isTextFrame(datagram))
        log(QStringLiteral("RX  %1").arg(QString::fromUtf8(datagram)));
    else
        log(QStringLiteral("RX  %1").arg(QString::fromLatin1(datagram.toHex().toUpper())));
    handleSoftReply(datagram);
}
