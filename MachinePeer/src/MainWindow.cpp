#include "MainWindow.h"

#include <QDateTime>
#include <QHostAddress>
#include <QVariant>
#include <QtGlobal>

namespace {

// 向波特率下拉填充常用速率项。
void addBaudItems(QComboBox* cmb)
{
    const int rates[] = {4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    for (int r : rates) {
        cmb->addItem(QString::number(r), r);
    }
}

} // namespace

// 按下拉项的 userData 选中项；找不到则选第一项。
void MainWindow::selectComboByData(QComboBox* combo, const QVariant& data)
{
    if (!combo) {
        return;
    }
    const int idx = combo->findData(data);
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

// 加载配置、填充控件并进入未打开状态。
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    m_cfg = LoadPeerConfig();
    fillCombos();
    wireSignals();
    updateComEcho();
    onCommTypeChanged(ui.cmbCommType->currentIndex());
    setOpened(false);

    if (m_cfg.fromFile) {
        appendLog(QStringLiteral("[配置] 已加载：%1").arg(m_cfg.sourcePath));
    } else {
        appendLog(QStringLiteral("[配置] 未找到 peerconfig.ini，使用内置默认"
                                 "（请将配置文件放在工作目录或 exe 旁）"));
    }
    appendLog(QStringLiteral("主路径：收对端指令并执行，测量值可回传。"
                             "串口 COM%1 %2 段长=%3；UDP %4:%5 → %6:%7")
                  .arg(m_cfg.serialPortIndex + 1)
                  .arg(SerialLineFormatText(m_cfg.baudRate, m_cfg.dataBits, m_cfg.parity,
                                            m_cfg.stopBits))
                  .arg(m_cfg.serialSegSize)
                  .arg(m_cfg.bindIp)
                  .arg(m_cfg.bindPort)
                  .arg(m_cfg.peerIp)
                  .arg(m_cfg.peerPort));
}

// 关闭串口与 UDP 后销毁。
MainWindow::~MainWindow()
{
    onCloseClicked();
}

// 初始化下拉项，并用 peerconfig 默认值选中。
void MainWindow::fillCombos()
{
    ui.cmbCommType->clear();
    ui.cmbCommType->addItem(QStringLiteral("串口"), 0);
    ui.cmbCommType->addItem(QStringLiteral("UDP 网口"), 1);
    ui.cmbCommType->setCurrentIndex(0);

    ui.cmbSegSize->clear();
    ui.cmbSegSize->addItem(QStringLiteral("8"), 8);
    ui.cmbSegSize->addItem(QStringLiteral("6"), 6);
    ui.cmbSegSize->addItem(QStringLiteral("5"), 5);

    ui.cmbBaud->clear();
    addBaudItems(ui.cmbBaud);

    ui.cmbDataBits->clear();
    ui.cmbDataBits->addItem(QStringLiteral("5"), 5);
    ui.cmbDataBits->addItem(QStringLiteral("6"), 6);
    ui.cmbDataBits->addItem(QStringLiteral("8"), 8);

    ui.cmbParity->clear();
    ui.cmbParity->addItem(QStringLiteral("无 (N)"), QChar(QLatin1Char('N')));
    ui.cmbParity->addItem(QStringLiteral("偶 (E)"), QChar(QLatin1Char('E')));
    ui.cmbParity->addItem(QStringLiteral("奇 (O)"), QChar(QLatin1Char('O')));
    ui.cmbParity->addItem(QStringLiteral("空格 (S)"), QChar(QLatin1Char('S')));
    ui.cmbParity->addItem(QStringLiteral("标记 (M)"), QChar(QLatin1Char('M')));

    ui.cmbStopBits->clear();
    ui.cmbStopBits->addItem(QStringLiteral("1"), QStringLiteral("1"));
    ui.cmbStopBits->addItem(QStringLiteral("1.5"), QStringLiteral("1.5"));
    ui.cmbStopBits->addItem(QStringLiteral("2"), QStringLiteral("2"));

    selectComboByData(ui.cmbSegSize, m_cfg.serialSegSize);
    selectComboByData(ui.cmbBaud, m_cfg.baudRate);
    if (ui.cmbBaud->currentData().toInt() != m_cfg.baudRate) {
        ui.cmbBaud->addItem(QString::number(m_cfg.baudRate), m_cfg.baudRate);
        selectComboByData(ui.cmbBaud, m_cfg.baudRate);
    }
    selectComboByData(ui.cmbDataBits, m_cfg.dataBits);
    selectComboByData(ui.cmbParity, QVariant(m_cfg.parity));
    selectComboByData(ui.cmbStopBits, m_cfg.stopBits);

    ui.spinComPort->setValue(m_cfg.serialPortIndex);
    ui.editBindIp->setText(m_cfg.bindIp);
    ui.spinBindPort->setValue(m_cfg.bindPort);
    ui.editPeerIp->setText(m_cfg.peerIp);
    ui.spinPeerPort->setValue(m_cfg.peerPort);
}

// 绑定控件与串口 / UDP 信号。
void MainWindow::wireSignals()
{
    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &MainWindow::onCommTypeChanged);
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, [this](int) { updateComEcho(); });
    QObject::connect(ui.btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    QObject::connect(ui.btnClose, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    QObject::connect(ui.btnSendMeasure, &QPushButton::clicked, this,
                     &MainWindow::onSendMeasureClicked);
    QObject::connect(ui.btnSelfTest, &QPushButton::clicked, this,
                     &MainWindow::onSelfTestParseClicked);
    QObject::connect(ui.btnCmdStart, &QPushButton::clicked, this, &MainWindow::onCmdStartClicked);
    QObject::connect(ui.btnCmdStop, &QPushButton::clicked, this, &MainWindow::onCmdStopClicked);
    QObject::connect(ui.btnCmdSave, &QPushButton::clicked, this, &MainWindow::onCmdSaveClicked);
    QObject::connect(ui.btnCmdZero, &QPushButton::clicked, this, &MainWindow::onCmdZeroClicked);
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    QObject::connect(&m_serial, &QSerialPort::readyRead, this, &MainWindow::onSerialReadyRead);
    QObject::connect(&m_udp, &QUdpSocket::readyRead, this, &MainWindow::onUdpReadyRead);
}

// 向运行记录追加一行（含时间戳）。
void MainWindow::appendLog(const QString& line)
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    ui.textLog->appendPlainText(QStringLiteral("%1 %2").arg(stamp, line));
}

// 更新状态栏文案。
void MainWindow::setStatus(const QString& text)
{
    ui.lblStatus->setText(QStringLiteral("状态：%1").arg(text));
}

// 按通道是否已打开启用 / 禁用相关控件。
void MainWindow::setOpened(bool opened)
{
    m_opened = opened;
    ui.btnOpen->setEnabled(!opened);
    ui.btnClose->setEnabled(opened);
    ui.cmbCommType->setEnabled(!opened);
    ui.serialPanel->setEnabled(!opened);
    ui.udpPanel->setEnabled(!opened);
    ui.btnSendMeasure->setEnabled(opened);
    ui.btnCmdStart->setEnabled(opened);
    ui.btnCmdStop->setEnabled(opened);
    ui.btnCmdSave->setEnabled(opened);
    ui.btnCmdZero->setEnabled(opened);
    ui.btnSelfTest->setEnabled(true);
}

// 根据 COM 序号刷新显示名（序号+1）。
void MainWindow::updateComEcho()
{
    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
}

// 当前是否为 UDP 模式。
bool MainWindow::isUdpMode() const
{
    return ui.cmbCommType->currentIndex() == 1;
}

// 界面当前三思段长（5/6/8）。
int MainWindow::currentSegSize() const
{
    return ui.cmbSegSize->currentData().toInt();
}

// 界面当前波特率。
int MainWindow::currentBaud() const
{
    return ui.cmbBaud->currentData().toInt();
}

// 界面当前数据位。
int MainWindow::currentDataBits() const
{
    return ui.cmbDataBits->currentData().toInt();
}

// 界面当前校验字母（N/E/O/S/M）。
QChar MainWindow::currentParity() const
{
    return ui.cmbParity->currentData().toChar();
}

// 界面当前停止位（1/1.5/2）。
QString MainWindow::currentStopBits() const
{
    return ui.cmbStopBits->currentData().toString();
}

// 切换串口 / UDP 面板可见性。
void MainWindow::onCommTypeChanged(int index)
{
    const bool udp = (index == 1);
    ui.serialPanel->setVisible(!udp);
    ui.udpPanel->setVisible(udp);
}

// 按界面参数打开串口或绑定 UDP。
void MainWindow::onOpenClicked()
{
    if (m_opened) {
        return;
    }

    if (isUdpMode()) {
        const QString bindIp = ui.editBindIp->text().trimmed();
        const QHostAddress addr(bindIp);
        if (bindIp.isEmpty() || addr.isNull()) {
            appendLog(QStringLiteral("[打开] 本机 IP 无效"));
            setStatus(QStringLiteral("参数无效"));
            return;
        }
        const QString peerIp = ui.editPeerIp->text().trimmed();
        if (peerIp.isEmpty() || QHostAddress(peerIp).isNull()) {
            appendLog(QStringLiteral("[打开] 对端 IP 无效"));
            setStatus(QStringLiteral("参数无效"));
            return;
        }
        const quint16 port = static_cast<quint16>(ui.spinBindPort->value());
        if (!m_udp.bind(addr, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            appendLog(QStringLiteral("[打开] UDP 绑定失败：%1").arg(m_udp.errorString()));
            setStatus(QStringLiteral("UDP 失败"));
            return;
        }
        appendLog(QStringLiteral("[打开] UDP %1:%2 → %3:%4")
                      .arg(bindIp)
                      .arg(port)
                      .arg(peerIp)
                      .arg(ui.spinPeerPort->value()));
        setStatus(QStringLiteral("UDP 已打开"));
    } else {
        m_serialRxBuffer.clear();
        m_serial.setPortName(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
        if (!ApplySerialLineFormat(&m_serial, currentBaud(), currentDataBits(), currentParity(),
                                   currentStopBits())) {
            appendLog(QStringLiteral("[打开] 线格式无效或驱动拒绝："
                                     "波特率/数据位/校验/停止位"));
            setStatus(QStringLiteral("线格式失败"));
            return;
        }
        if (!m_serial.open(QIODevice::ReadWrite)) {
            appendLog(QStringLiteral("[打开] 串口失败：%1").arg(m_serial.errorString()));
            setStatus(QStringLiteral("串口失败"));
            return;
        }
        appendLog(QStringLiteral("[打开] %1 %2 段长=%3")
                      .arg(m_serial.portName())
                      .arg(SerialLineFormatText(currentBaud(), currentDataBits(), currentParity(),
                                                currentStopBits()))
                      .arg(currentSegSize()));
        setStatus(QStringLiteral("串口已打开"));
    }

    setOpened(true);
}

// 关闭当前通道并清空串口收缓冲。
void MainWindow::onCloseClicked()
{
    if (m_serial.isOpen()) {
        m_serial.close();
    }
    if (m_udp.state() != QAbstractSocket::UnconnectedState) {
        m_udp.close();
    }
    m_serialRxBuffer.clear();
    if (m_opened) {
        appendLog(QStringLiteral("[关闭] 通道已关闭"));
    }
    setOpened(false);
    setStatus(QStringLiteral("未打开"));
}

// 清空运行记录。
void MainWindow::onClearLogClicked()
{
    ui.textLog->clear();
}

// 解析界面测量输入；失败时写入 err。
bool MainWindow::parseMeasureFields(double* force, double* move, double* strain, QString* err) const
{
    bool okF = false;
    bool okM = false;
    bool okS = false;
    const double f = ui.editForce->text().trimmed().toDouble(&okF);
    const double m = ui.editMove->text().trimmed().toDouble(&okM);
    const double s = ui.editStrain->text().trimmed().toDouble(&okS);
    if (!okF || !okM || !okS) {
        if (err) {
            *err = QStringLiteral("力/位移/应变须为有效浮点数");
        }
        return false;
    }
    *force = f;
    *move = m;
    *strain = s;
    return true;
}

// 组包并发送界面上的力 / 位移 / 应变。
bool MainWindow::sendMeasurePayload()
{
    double force = 0;
    double move = 0;
    double strain = 0;
    QString err;
    if (!parseMeasureFields(&force, &move, &strain, &err)) {
        appendLog(QStringLiteral("[发送] 失败：%1").arg(err));
        return false;
    }

    if (isUdpMode()) {
        const QByteArray packet = WireCodec::packSansiUdp(force, move, strain);
        const qint64 n = m_udp.writeDatagram(
            packet, QHostAddress(ui.editPeerIp->text().trimmed()),
            static_cast<quint16>(ui.spinPeerPort->value()));
        if (n < 0) {
            appendLog(QStringLiteral("[发送] UDP 失败：%1").arg(m_udp.errorString()));
            return false;
        }
        appendLog(QStringLiteral("[发送][测量] UDP 力=%1 位移=%2 应变=%3 → 对端应有 [接收]")
                      .arg(force)
                      .arg(move)
                      .arg(strain));
        return true;
    }

    const QByteArray frame =
        WireCodec::packSansiSerial(QVector<double>{force}, currentSegSize());
    if (frame.isEmpty()) {
        appendLog(QStringLiteral("[发送][测量] 串口组包失败"));
        return false;
    }
    const qint64 n = m_serial.write(frame);
    m_serial.flush();
    if (n < 0) {
        appendLog(QStringLiteral("[发送][测量] 串口失败：%1").arg(m_serial.errorString()));
        return false;
    }
    appendLog(QStringLiteral("[发送][测量] 串口 %1 → 对端应有 [接收]")
                  .arg(WireCodec::toHexSpaced(frame)));
    return true;
}

// 手动向对端发送一组测量值。
void MainWindow::onSendMeasureClicked()
{
    if (!m_opened) {
        appendLog(QStringLiteral("[发送] 通道未打开"));
        return;
    }
    sendMeasurePayload();
}

// 经当前通道发送原始字节并记录日志。
bool MainWindow::sendRaw(const QByteArray& bytes, const QString& kind, const QString& detail)
{
    if (!m_opened || bytes.isEmpty()) {
        return false;
    }
    if (isUdpMode()) {
        const qint64 n = m_udp.writeDatagram(
            bytes, QHostAddress(ui.editPeerIp->text().trimmed()),
            static_cast<quint16>(ui.spinPeerPort->value()));
        if (n < 0) {
            appendLog(QStringLiteral("[发送][%1] UDP 失败：%2").arg(kind, m_udp.errorString()));
            return false;
        }
    } else {
        const qint64 n = m_serial.write(bytes);
        m_serial.flush();
        if (n < 0) {
            appendLog(QStringLiteral("[发送][%1] 串口失败：%2").arg(kind, m_serial.errorString()));
            return false;
        }
    }
    appendLog(QStringLiteral("[发送][%1] %2 %3 → 对端应有 [接收]")
                  .arg(kind, detail, WireCodec::toHexSpaced(bytes)));
    return true;
}

// 本端组包并发送控制指令（附加双向能力，非主路径）。
bool MainWindow::sendOutboundCommand(WireCodec::SerialCommand cmd)
{
    const QByteArray bytes = WireCodec::packSerialCommand(cmd);
    if (bytes.isEmpty()) {
        return false;
    }
    const QString name = WireCodec::serialCommandName(cmd);
    ui.lblLastAction->setText(QStringLiteral("最近动作：本端发出 %1（附加）").arg(name));
    return sendRaw(bytes, QStringLiteral("指令"), name);
}

// 附加：本端发「开始」指令到对端。
void MainWindow::onCmdStartClicked()
{
    sendOutboundCommand(WireCodec::SerialCommand::StartCalc);
}

// 附加：本端发「停止」指令。
void MainWindow::onCmdStopClicked()
{
    sendOutboundCommand(WireCodec::SerialCommand::StopCalc);
}

// 附加：本端发「存图」指令。
void MainWindow::onCmdSaveClicked()
{
    sendOutboundCommand(WireCodec::SerialCommand::SaveImage);
}

// 附加：本端发「清零」指令。
void MainWindow::onCmdZeroClicked()
{
    sendOutboundCommand(WireCodec::SerialCommand::ZeroClear);
}

// 本地自检：不经端口注入「开始」指令帧。
void MainWindow::onSelfTestParseClicked()
{
    const QByteArray frame =
        WireCodec::packSerialCommand(WireCodec::SerialCommand::StartCalc);
    appendLog(QStringLiteral("[自检] 本地注入帧 %1（不经端口）")
                  .arg(WireCodec::toHexSpaced(frame)));
    handleSerialFrame(frame);
}

// 执行已识别的控制指令；「开始」且勾选自动回测时发送测量值。
void MainWindow::executeCommand(WireCodec::SerialCommand cmd, const QString& sourceTag)
{
    const QString name = WireCodec::serialCommandName(cmd);
    ui.lblLastAction->setText(QStringLiteral("最近动作：执行 %1（%2）").arg(name, sourceTag));
    appendLog(QStringLiteral("[接收][指令] %1 ← %2").arg(name, sourceTag));
    appendLog(QStringLiteral("[执行] %1").arg(name));

    switch (cmd) {
    case WireCodec::SerialCommand::StartCalc:
        if (ui.chkAutoReply->isChecked()) {
            appendLog(QStringLiteral("[执行] 回发测量值到对端"));
            sendMeasurePayload();
        }
        break;
    case WireCodec::SerialCommand::StopCalc:
    case WireCodec::SerialCommand::SaveImage:
    case WireCodec::SerialCommand::ZeroClear:
        break;
    default:
        break;
    }
}

// 处理一帧串口数据：控制指令则执行，否则记为数据帧。
void MainWindow::handleSerialFrame(const QByteArray& frame)
{
    const auto cmd = WireCodec::classifySerialCommand(frame);
    if (cmd != WireCodec::SerialCommand::Unknown) {
        executeCommand(cmd, QStringLiteral("串口 %1").arg(WireCodec::toHexSpaced(frame)));
        return;
    }
    appendLog(QStringLiteral("[接收][数据] len=%1 %2")
                  .arg(frame.size())
                  .arg(WireCodec::toHexSpaced(frame)));
}

// 串口有可读数据时切帧并处理。
void MainWindow::onSerialReadyRead()
{
    m_serialRxBuffer.append(m_serial.readAll());
    const QVector<QByteArray> frames = WireCodec::takeFramesEndingCr(&m_serialRxBuffer);
    for (const QByteArray& frame : frames) {
        handleSerialFrame(frame);
    }
}

// UDP 有数据报时记录；若为控制指令则执行。
void MainWindow::onUdpReadyRead()
{
    while (m_udp.hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(static_cast<int>(m_udp.pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        m_udp.readDatagram(buf.data(), buf.size(), &sender, &senderPort);
        const QString src = QStringLiteral("%1:%2").arg(sender.toString()).arg(senderPort);
        appendLog(QStringLiteral("[接收][原始] UDP %1 len=%2 %3")
                      .arg(src)
                      .arg(buf.size())
                      .arg(WireCodec::toHexSpaced(buf.left(qMin(32, buf.size())))));

        const auto cmd = WireCodec::classifySerialCommand(buf);
        if (cmd != WireCodec::SerialCommand::Unknown) {
            executeCommand(cmd, QStringLiteral("UDP %1").arg(WireCodec::toHexSpaced(buf)));
        } else if (buf.size() == 24) {
            appendLog(QStringLiteral("[接收][测量] UDP 疑似三思 24 字节包 ← %1").arg(src));
        } else {
            appendLog(QStringLiteral("[接收][数据] UDP 未识别为控制指令 ← %1").arg(src));
        }
    }
}
