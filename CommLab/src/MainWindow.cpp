#include "MainWindow.h"

#include "CommController.h"
#include "CommHandler.h"
#include "NetConfig.h"
#include "SerialFramePreview.h"
#include "SmokeRunner.h"
#include "UIDef.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QIODevice>
#include <QMessageBox>
#include <QSerialPort>
#include <QtGlobal>
#include <exception>
#include <vector>

namespace {

// 将数值列表格式化为日志用 [a,b,c] 文本。
QString formatValues(const QVector<double>& values)
{
    QStringList parts;
    parts.reserve(values.size());
    for (double v : values) {
        parts << QString::number(v, 'g', 12);
    }
    return QStringLiteral("[%1]").arg(parts.join(QLatin1Char(',')));
}

} // namespace

// 组装界面、绑定库回调，进入未连接态。
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(new CommController(this))
{
    ui.setupUi(this);
    fillCombos();
    wireSignals();
    updateComEcho();
    updateLineFormatEcho();
    onCommTypeChanged(ui.cmbCommType->currentIndex());

    QObject::connect(m_controller, &CommController::safeDataReceived,
                     this, &MainWindow::onSafeData);
    QObject::connect(m_controller, &CommController::eventReceived,
                     this, &MainWindow::onEvent);
    QObject::connect(m_controller, &CommController::connectionReceived,
                     this, &MainWindow::onConn);
    QObject::connect(m_controller, &CommController::clientDisconnected,
                     this, &MainWindow::onClientGone);
    QObject::connect(m_controller, &CommController::logMessage,
                     this, &MainWindow::onControllerLog);

    appendLog(QStringLiteral("就绪。默认串口 %1，%2。")
                  .arg(ui.lblComEcho->text())
                  .arg(ui.lblLineFormat->text()));
    if (m_netConfig.fromFile) {
        appendLog(QStringLiteral("[配置] 已加载网口配置：%1").arg(m_netConfig.sourcePath));
    } else {
        appendLog(QStringLiteral("[配置] 未找到 netconfig.ini，使用内置网口默认值"));
    }
    setControlsEnabled(false);
    updateSendUiState();
}

// 断开库连接后销毁。
MainWindow::~MainWindow()
{
    if (m_controller && m_controller->handler()) {
        m_controller->handler()->Disconnect(0);
    }
}

// 填充通道/协议/串口下拉，并载入 netconfig 默认地址。
void MainWindow::fillCombos()
{
    ui.cmbCommType->clear();
    ui.cmbCommType->addItem(QStringLiteral("UDP 网口"), 0);
    ui.cmbCommType->addItem(QStringLiteral("串口"), 1);
    ui.cmbCommType->setCurrentIndex(1);

    // 网口协议号与 SocketComm::onRecvDataSlots / SendData 一致；UserRole+1=1 表示库支持数值发送。
    // 已废弃且源码注释掉的「联恒」旧 case 0 不列入（见手册「废弃项」）。
    ui.cmbNetProto->clear();
    const auto addNet = [this](const QString& text, int protoId, bool canSend) {
        ui.cmbNetProto->addItem(text, protoId);
        ui.cmbNetProto->setItemData(ui.cmbNetProto->count() - 1, canSend ? 1 : 0,
                                   Qt::UserRole + 1);
    };
    addNet(QStringLiteral("0 JSON（可收可发）"), 0, true);
    addNet(QStringLiteral("1 万测（可收可发）"), 1, true);
    addNet(QStringLiteral("2 中机四通道（可收可发）"), 2, true);
    addNet(QStringLiteral("3 三思试验（仅接收）"), 3, false);
    addNet(QStringLiteral("4 触发存图（仅事件）"), 4, false);
    addNet(QStringLiteral("5 福建威盛（仅接收）"), 5, false);
    addNet(QStringLiteral("6 纳百川全自动（可收可发）"), 6, true);
    addNet(QStringLiteral("7 纳百川识别线条（可收可发）"), 7, true);
    // 默认选三思，便于联调收测量。
    for (int i = 0; i < ui.cmbNetProto->count(); ++i) {
        if (ui.cmbNetProto->itemData(i).toInt() == 3) {
            ui.cmbNetProto->setCurrentIndex(i);
            break;
        }
    }

    ui.cmbBaud->clear();
    ui.cmbBaud->addItem(QStringLiteral("4800"), 0);
    ui.cmbBaud->addItem(QStringLiteral("9600"), 1);
    ui.cmbBaud->addItem(QStringLiteral("19200"), 2);
    ui.cmbBaud->addItem(QStringLiteral("38400"), 3);
    ui.cmbBaud->addItem(QStringLiteral("57600"), 4);
    ui.cmbBaud->addItem(QStringLiteral("115200"), 5);
    ui.cmbBaud->setCurrentIndex(5);

    // iDataBits：0=Data5，1=Data6，3=Data8；通信库无 Data7 映射，故不提供 7。
    ui.cmbDataBits->clear();
    ui.cmbDataBits->addItem(QStringLiteral("5"), 0);
    ui.cmbDataBits->addItem(QStringLiteral("6"), 1);
    ui.cmbDataBits->addItem(QStringLiteral("8"), 3);
    ui.cmbDataBits->setCurrentIndex(2);

    ui.cmbParity->clear();
    ui.cmbParity->addItem(QStringLiteral("无校验 (N)"), 0);
    ui.cmbParity->addItem(QStringLiteral("偶校验 (E)"), 1);
    ui.cmbParity->addItem(QStringLiteral("奇校验 (O)"), 2);
    ui.cmbParity->addItem(QStringLiteral("空格 (S)"), 3);
    ui.cmbParity->addItem(QStringLiteral("标记 (M)"), 4);
    ui.cmbParity->setCurrentIndex(0);

    ui.cmbStopBits->clear();
    ui.cmbStopBits->addItem(QStringLiteral("1"), 0);
    ui.cmbStopBits->addItem(QStringLiteral("1.5"), 1);
    ui.cmbStopBits->addItem(QStringLiteral("2"), 2);
    ui.cmbStopBits->setCurrentIndex(0);

    ui.cmbSerialProto->clear();
    ui.cmbSerialProto->addItem(QStringLiteral("0 三思 SSCK（可收可发）"), 0);
    ui.cmbSerialProto->addItem(QStringLiteral("1 长春科新"), 1);
    ui.cmbSerialProto->addItem(QStringLiteral("2 时代新材（发送未实现）"), 2);
    ui.cmbSerialProto->addItem(QStringLiteral("3 IEEE"), 3);
    ui.cmbSerialProto->addItem(QStringLiteral("4 冠腾"), 4);

    const NetConfig net = LoadNetConfig();
    ui.spinLocalPort->setValue(net.localPort);
    ui.spinDestPort->setValue(net.destPort);
    ui.editLocalIp->setText(net.localIp);
    ui.editDestIp->setText(net.destIp);
    m_netConfig = net;
}

// 连接界面按钮与下拉信号到本窗槽。
void MainWindow::wireSignals()
{
    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &MainWindow::onCommTypeChanged);
    QObject::connect(ui.cmbNetProto, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &MainWindow::onNetProtoChanged);
    QObject::connect(ui.cmbSerialProto, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, [this](int) { updateSendUiState(); });
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, [this](int) { updateComEcho(); });
    const auto refreshLine = [this](int) { updateLineFormatEcho(); };
    QObject::connect(ui.cmbBaud, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshLine);
    QObject::connect(ui.cmbDataBits, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshLine);
    QObject::connect(ui.cmbParity, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshLine);
    QObject::connect(ui.cmbStopBits, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshLine);
    QObject::connect(ui.chkAllowSend, &QCheckBox::toggled, this, &MainWindow::onAllowSendToggled);
    QObject::connect(ui.btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    QObject::connect(ui.btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    QObject::connect(ui.btnSmokeRx, &QPushButton::clicked, this, &MainWindow::onSmokeUdpSansiClicked);
    QObject::connect(ui.btnSmokeTx, &QPushButton::clicked, this, &MainWindow::onSmokeUdpSendClicked);
    QObject::connect(ui.btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    QObject::connect(ui.btnCmdStart, &QPushButton::clicked, this, &MainWindow::onCmdStartClicked);
    QObject::connect(ui.btnCmdStop, &QPushButton::clicked, this, &MainWindow::onCmdStopClicked);
    QObject::connect(ui.btnCmdSave, &QPushButton::clicked, this, &MainWindow::onCmdSaveClicked);
    QObject::connect(ui.btnCmdZero, &QPushButton::clicked, this, &MainWindow::onCmdZeroClicked);
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    QObject::connect(ui.btnSaveLog, &QPushButton::clicked, this, &MainWindow::onSaveLogClicked);
}

// 带时间戳追加一行运行记录。
void MainWindow::appendLog(const QString& line)
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    ui.textLog->appendPlainText(QStringLiteral("%1 %2").arg(stamp, line));
}

// 更新状态栏短文案。
void MainWindow::setStatus(const QString& text)
{
    ui.lblStatus->setText(QStringLiteral("状态：%1").arg(text));
}

// 刷新收发与错误计数标签。
void MainWindow::refreshCounters()
{
    ui.lblCounters->setText(
        QStringLiteral("计数：接收数据=%1 事件=%2 发送尝试=%3 错误=%4")
            .arg(m_rxDataCount)
            .arg(m_rxEventCount)
            .arg(m_txAttemptCount)
            .arg(m_errorCount));
}

// 根据 spinComPort 显示 COMx 名称。
void MainWindow::updateComEcho()
{
    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
}

// 取界面数据位枚举值。
int MainWindow::currentDataBitsParam() const
{
    return ui.cmbDataBits->currentData().toInt();
}

// 取界面校验枚举值。
int MainWindow::currentParityParam() const
{
    return ui.cmbParity->currentData().toInt();
}

// 取界面停止位枚举值。
int MainWindow::currentStopBitsParam() const
{
    return ui.cmbStopBits->currentData().toInt();
}

// 三思段长：Data5/6/8 → 5/6/8。
int MainWindow::currentSansiSegSize() const
{
    // 与 SerialPortComm::Connect 写入 m_iDataBits 后的枚举数值一致。
    switch (currentDataBitsParam()) {
    case 0:
        return 5;
    case 1:
        return 6;
    case 3:
        return 8;
    default:
        return 8;
    }
}

// 拼接线格式摘要如「115200 8N1」。
QString MainWindow::currentLineFormatText() const
{
    QChar parityChar = QLatin1Char('N');
    switch (currentParityParam()) {
    case 1:
        parityChar = QLatin1Char('E');
        break;
    case 2:
        parityChar = QLatin1Char('O');
        break;
    case 3:
        parityChar = QLatin1Char('S');
        break;
    case 4:
        parityChar = QLatin1Char('M');
        break;
    default:
        parityChar = QLatin1Char('N');
        break;
    }

    QString stopText = QStringLiteral("1");
    switch (currentStopBitsParam()) {
    case 1:
        stopText = QStringLiteral("1.5");
        break;
    case 2:
        stopText = QStringLiteral("2");
        break;
    default:
        stopText = QStringLiteral("1");
        break;
    }

    // 线格式示例：115200 8N1；停止位 1.5 时写作 8N1.5。
    return QStringLiteral("%1 %2%3%4")
        .arg(ui.cmbBaud->currentText())
        .arg(currentSansiSegSize())
        .arg(parityChar)
        .arg(stopText);
}

// 把线格式摘要刷到界面标签。
void MainWindow::updateLineFormatEcho()
{
    ui.lblLineFormat->setText(currentLineFormatText());
}

// 当前网口协议是否允许数值 SendData（读 UserRole+1）。
bool MainWindow::currentNetProtoSupportsSend() const
{
    return ui.cmbNetProto->currentData(Qt::UserRole + 1).toInt() == 1;
}

// 取可用 CommHandler；失败时记错误并返回空。
CommHandler* MainWindow::requireHandler(const QString& actionTag)
{
    if (!m_controller) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[%1] 失败：控制器未初始化").arg(actionTag));
        return nullptr;
    }
    CommHandler* comm = m_controller->handler();
    if (!comm) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[%1] 失败：通信句柄不可用").arg(actionTag));
        return nullptr;
    }
    return comm;
}

// 已知串口命令事件的十六进制对照说明。
QString MainWindow::serialEventHexHint(int msg)
{
    switch (msg) {
    case W_CUSTOM_COMM_STARTCALC:
        return QStringLiteral("线帧 3D 0D（本软件也可对该字节发令）");
    case W_CUSTOM_COMM_STOPCALC:
        return QStringLiteral("线帧 3E 0D（本软件也可对该字节发令）");
    case W_CUSTOM_COMM_AUTO_SAVE_IMAGE:
        return QStringLiteral("线帧 6B 0D（本软件也可对该字节发令）");
    case W_CUSTOM_ZERO_CLEARING:
        return QStringLiteral("线帧 5A 0D（本软件也可对该字节发令）");
    default:
        return {};
    }
}

// 串口三思发后输出本地 [预期帧] 十六进制。
void MainWindow::appendSerialHexPreview(const QVector<double>& values)
{
    const int serProto = ui.cmbSerialProto->currentData().toInt();
    if (serProto != 0) {
        appendLog(QStringLiteral("[预期帧] 当前串口协议=%1，仅三思协议(0)支持与通信库对齐的十六进制预览。")
                      .arg(serProto));
        return;
    }
    const QByteArray packed = SerialFramePreview::packSansiSsck(values, currentSansiSegSize());
    if (packed.isEmpty()) {
        appendLog(QStringLiteral("[预期帧] 组包失败：未填写有效数值或数据位段长无效"));
        return;
    }
    appendLog(QStringLiteral("[预期帧] %1（长度=%2，段长=%3，线格式=%4）")
                  .arg(SerialFramePreview::toHexSpaced(packed))
                  .arg(packed.size())
                  .arg(currentSansiSegSize())
                  .arg(currentLineFormatText()));
    appendLog(QStringLiteral("[说明] 请用相同线格式在对端十六进制区对照。"));
}

// 按连接态与协议能力刷新发送区控件。
void MainWindow::updateSendUiState()
{
    const bool udp = (ui.cmbCommType->currentIndex() == 0);
    const bool connected = ui.btnDisconnect->isEnabled();
    bool canSendProto = false;
    if (udp) {
        canSendProto = currentNetProtoSupportsSend();
        ui.lblSendHint->setText(
            canSendProto
                ? QStringLiteral("点击发送后自动启用许可，并在运行记录中输出预期帧。")
                : QStringLiteral("当前协议库侧无数值发送（iProtoType=3/4/5），请改选 0/1/2/6/7。"));
    } else {
        const int serProto = ui.cmbSerialProto->currentData().toInt();
        canSendProto = (serProto != 2);
        ui.lblSendHint->setText(
            canSendProto
                ? QStringLiteral("发送后可在运行记录中查看预期帧。")
                : QStringLiteral("当前协议发送未实现。"));
    }

    ui.chkAllowSend->setEnabled(connected && canSendProto);
    ui.editSendValues->setEnabled(connected && canSendProto);
    // 串口：即使未勾选发送许可也可点击发送（发送时强制启用）。网口：须勾选，以降低误发风险。
    ui.btnSend->setEnabled(connected && canSendProto
                           && (!udp || ui.chkAllowSend->isChecked()));
    // 控制指令：已连接即可发（走 SendData(QString)，与数值协议开关独立）。
    ui.btnCmdStart->setEnabled(connected);
    ui.btnCmdStop->setEnabled(connected);
    ui.btnCmdSave->setEnabled(connected);
    ui.btnCmdZero->setEnabled(connected);
    if (!canSendProto && udp) {
        ui.chkAllowSend->setChecked(false);
    }
}

// 连接成功后放开操作按钮；断开后收回。
void MainWindow::setControlsEnabled(bool connected)
{
    ui.btnConnect->setEnabled(!connected);
    ui.btnDisconnect->setEnabled(connected);
    ui.cmbCommType->setEnabled(!connected);
    ui.udpPanel->setEnabled(!connected);
    ui.serialPanel->setEnabled(!connected);
    updateSendUiState();
}

// 切换 UDP / 串口配置面板可见性。
void MainWindow::onCommTypeChanged(int index)
{
    const bool udp = (index == 0);
    ui.udpPanel->setVisible(udp);
    ui.serialPanel->setVisible(!udp);
    ui.btnSmokeRx->setEnabled(udp);
    ui.btnSmokeTx->setEnabled(udp);
    updateSendUiState();
}

// 网口协议变更时刷新发送可用性。
void MainWindow::onNetProtoChanged(int)
{
    updateSendUiState();
}

// 将发送许可复选框同步为 bInquireSendFlag。
void MainWindow::onAllowSendToggled(bool checked)
{
    if (!ui.btnDisconnect->isEnabled()) {
        return;
    }
    CommHandler* comm = requireHandler(QStringLiteral("发送许可"));
    if (!comm) {
        return;
    }
    try {
        comm->setParameter(QStringLiteral("bInquireSendFlag"), checked);
        appendLog(checked ? QStringLiteral("[发送许可] 已启用")
                          : QStringLiteral("[发送许可] 已关闭"));
    } catch (const std::exception& ex) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[错误] 设置发送许可失败：%1")
                      .arg(QString::fromLocal8Bit(ex.what())));
    } catch (...) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[错误] 设置发送许可失败：未知异常"));
    }
    updateSendUiState();
}

// 将界面 IP/端口与网口协议写入 DLL。
bool MainWindow::applyUdpParams()
{
    CommHandler* comm = m_controller->handler();
    const int proto = ui.cmbNetProto->currentData().toInt();
    NetConfig cfg = m_netConfig;
    cfg.localIp = ui.editLocalIp->text().trimmed();
    cfg.destIp = ui.editDestIp->text().trimmed();
    cfg.localPort = ui.spinLocalPort->value();
    cfg.destPort = ui.spinDestPort->value();
    ConfigureLabUdp(comm, proto, cfg);
    comm->setParameter(QStringLiteral("bInquireSendFlag"), ui.chkAllowSend->isChecked());
    appendLog(QStringLiteral("[配置] 网口协议=%1，本机=%2:%3，目的=%4:%5")
                  .arg(proto)
                  .arg(cfg.localIp)
                  .arg(cfg.localPort)
                  .arg(cfg.destIp)
                  .arg(cfg.destPort));
    return true;
}

// 将界面串口参数写入 DLL。
bool MainWindow::applySerialParams()
{
    // 将界面串口参数写入通信库（连接前调用）。
    CommHandler* comm = m_controller->handler();
    const int comPort = ui.spinComPort->value();
    const int baudIdx = ui.cmbBaud->currentData().toInt();
    const int dataBits = currentDataBitsParam();
    const int parity = currentParityParam();
    const int stopBits = currentStopBitsParam();
    const int proto = ui.cmbSerialProto->currentData().toInt();

    DataInput input{};
    input.iChannelState = 1;
    input.iTriggerCtrl = 1;

    comm->SetCommType(SERIAL);
    comm->setParameter(QStringLiteral("iComPort"), comPort);
    comm->setParameter(QStringLiteral("iComBaud"), baudIdx);
    comm->setParameter(QStringLiteral("iDataBits"), dataBits);
    comm->setParameter(QStringLiteral("iParity"), parity);
    comm->setParameter(QStringLiteral("iStopBits"), stopBits);
    comm->setParameter(QStringLiteral("iProtocolType"), proto);
    comm->setParameter(QStringLiteral("dInForceRange"), 100.0);
    comm->setParameter(QStringLiteral("input"), input);
    comm->setParameter(QStringLiteral("bInquireSendFlag"), ui.chkAllowSend->isChecked());

    appendLog(QStringLiteral("[配置] 串口 %1（序号 %2），线格式=%3，协议=%4，"
                             "数据位参数=%5，校验参数=%6，停止位参数=%7")
                  .arg(QStringLiteral("COM%1").arg(comPort + 1))
                  .arg(comPort)
                  .arg(currentLineFormatText())
                  .arg(proto)
                  .arg(dataBits)
                  .arg(parity)
                  .arg(stopBits));
    appendLog(QStringLiteral("[说明] 命令本端可发：3D0D 开始，3E0D 停止，6B0D 存图，5A0D 清零。"));
    return true;
}

// 解析逗号分隔浮点；失败时写 errOut 并返回空。
QVector<double> MainWindow::parseValues(const QString& text, QString* errOut)
{
    QVector<double> out;
    const QStringList parts = text.split(QLatin1Char(','),
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                                         Qt::SkipEmptyParts
#else
                                         QString::SkipEmptyParts
#endif
    );
    if (parts.isEmpty()) {
        if (errOut) {
            *errOut = QStringLiteral("未填写有效数值");
        }
        return out;
    }
    for (const QString& p : parts) {
        bool ok = false;
        const double v = p.trimmed().toDouble(&ok);
        if (!ok) {
            if (errOut) {
                *errOut = QStringLiteral("数值格式无效：%1").arg(p);
            }
            out.clear();
            return out;
        }
        out.push_back(v);
    }
    return out;
}

// 配置参数并 Connect；串口成功后自动开发送许可。
void MainWindow::onConnectClicked()
{
    // 写入通信参数并建立连接；串口成功后自动启用发送许可。
    CommHandler* comm = requireHandler(QStringLiteral("连接"));
    if (!comm) {
        setStatus(QStringLiteral("通信控制器不可用"));
        return;
    }

    const bool udpMode = (ui.cmbCommType->currentIndex() == 0);
    try {
        if (udpMode) {
            applyUdpParams();
        } else {
            applySerialParams();
        }
    } catch (const std::exception& ex) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[错误] 设置参数异常：%1")
                      .arg(QString::fromLocal8Bit(ex.what())));
        setStatus(QStringLiteral("参数失败"));
        return;
    } catch (...) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[错误] 设置参数异常：未知异常"));
        setStatus(QStringLiteral("参数失败"));
        return;
    }

    const bool ok = comm->Connect();
    appendLog(ok ? QStringLiteral("[连接] 成功")
                 : QStringLiteral("[连接] 失败（Connect 返回 false）"));
    if (ok && !udpMode) {
        try {
            comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
            ui.chkAllowSend->blockSignals(true);
            ui.chkAllowSend->setChecked(true);
            ui.chkAllowSend->blockSignals(false);
            appendLog(QStringLiteral("[发送许可] 串口连接后已自动启用"));
        } catch (const std::exception& ex) {
            appendLog(QStringLiteral("[发送许可] 自动启用失败：%1")
                          .arg(QString::fromLocal8Bit(ex.what())));
        } catch (...) {
            appendLog(QStringLiteral("[发送许可] 自动启用失败：未知异常"));
        }
        setStatus(QStringLiteral("已连接 %1").arg(ui.lblComEcho->text()));
    } else {
        setStatus(ok ? QStringLiteral("已连接") : QStringLiteral("连接失败"));
    }
    setControlsEnabled(ok);
}

// 调用 Disconnect(0) 并收回控件。
void MainWindow::onDisconnectClicked()
{
    if (CommHandler* comm = requireHandler(QStringLiteral("断开"))) {
        try {
            comm->Disconnect(0);
            appendLog(QStringLiteral("[连接] 已断开"));
        } catch (const std::exception& ex) {
            ++m_errorCount;
            refreshCounters();
            appendLog(QStringLiteral("[断开] 异常：%1")
                          .arg(QString::fromLocal8Bit(ex.what())));
        } catch (...) {
            ++m_errorCount;
            refreshCounters();
            appendLog(QStringLiteral("[断开] 异常：未知异常"));
        }
    }
    setStatus(QStringLiteral("已断开"));
    setControlsEnabled(false);
}

void MainWindow::onSendClicked()
{
    // 校验连接与数值后调用 SendData；串口三思额外输出预期十六进制帧。
    if (!ui.btnDisconnect->isEnabled()) {
        appendLog(QStringLiteral("[发送] 未连接，已忽略"));
        return;
    }

    CommHandler* comm = requireHandler(QStringLiteral("发送"));
    if (!comm) {
        return;
    }

    QString err;
    const QVector<double> values = parseValues(ui.editSendValues->text(), &err);
    if (values.isEmpty()) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[发送] 失败：%1").arg(err));
        return;
    }
    if (values.size() > kMaxSendValueCount) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[发送] 已拒绝：数值个数 %1 超过上限 %2")
                      .arg(values.size())
                      .arg(kMaxSendValueCount));
        return;
    }

    const bool udp = (ui.cmbCommType->currentIndex() == 0);
    if (udp && !currentNetProtoSupportsSend()) {
        appendLog(QStringLiteral("[发送] 已拒绝：当前网口协议未实现发送"));
        return;
    }

    ++m_txAttemptCount;
    refreshCounters();
    try {
        comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
        ui.chkAllowSend->blockSignals(true);
        ui.chkAllowSend->setChecked(true);
        ui.chkAllowSend->blockSignals(false);
        updateSendUiState();
        // 直接传入通信库通道枚举常量，避免局部 CommType 变量扩大分析告警面。
        std::vector<double> v(values.begin(), values.end());
        comm->SendData(v, udp ? NETWORK : SERIAL);
        appendLog(QStringLiteral("[发送][数值] 已下发，通道=%1 数值=%2")
                      .arg(udp ? QStringLiteral("网口") : QStringLiteral("串口"))
                      .arg(formatValues(values)));
        if (!udp) {
            appendSerialHexPreview(values);
        }
    } catch (const std::exception& ex) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[发送] 异常：%1").arg(QString::fromLocal8Bit(ex.what())));
    } catch (...) {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[发送] 异常：未知异常"));
    }
}

// 组装命令码 + 0x0D。
QByteArray MainWindow::packControlFrame(quint8 cmdByte)
{
    QByteArray raw;
    raw.append(static_cast<char>(cmdByte));
    raw.append(static_cast<char>(0x0D));
    return raw;
}

// UDP：向界面目的地址 writeDatagram 原始指令帧。
bool MainWindow::sendControlCommandViaUdpRaw(const QByteArray& frame, const QString& name)
{
    const QString destIp = ui.editDestIp->text().trimmed();
    const quint16 destPort = static_cast<quint16>(ui.spinDestPort->value());
    const QHostAddress addr(destIp);
    if (destIp.isEmpty() || addr.isNull()) {
        appendLog(QStringLiteral("[发送][指令] UDP 目的 IP 无效"));
        return false;
    }
    const qint64 n = m_cmdUdp.writeDatagram(frame, addr, destPort);
    const QString hex = SerialFramePreview::toHexSpaced(frame);
    if (n < 0) {
        appendLog(QStringLiteral("[发送][指令] UDP 失败：%1").arg(m_cmdUdp.errorString()));
        return false;
    }
    appendLog(QStringLiteral("[发送][指令] UDP→%1:%2 %3（%4） 写出%5字节")
                  .arg(destIp)
                  .arg(destPort)
                  .arg(hex)
                  .arg(name)
                  .arg(n));
    appendLog(QStringLiteral("[提示] 试验机日志应出现 [接收][指令] %1").arg(name));
    return true;
}

// 串口旁路：Disconnect → QSerialPort 写帧 → Connect（绕过库内可能空实现的 SendData(QString)）。
bool MainWindow::sendControlCommandViaSerialBypass(const QByteArray& frame, const QString& name)
{
    CommHandler* comm = requireHandler(QStringLiteral("指令旁路"));
    if (!comm) {
        return false;
    }

    const QString portName = QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1);
    const QString hex = SerialFramePreview::toHexSpaced(frame);

    appendLog(QStringLiteral("[发送][指令] 串口旁路：暂释库占用 → %1 写 %2（%3）")
                  .arg(portName, hex, name));

    try {
        comm->Disconnect(0);
    } catch (...) {
        appendLog(QStringLiteral("[发送][指令] Disconnect 异常，仍尝试打开串口写出"));
    }

    QSerialPort port;
    port.setPortName(portName);
    port.setBaudRate(ui.cmbBaud->currentText().toInt());
    // 数据位与界面枚举对齐：0→5，1→6，3→8
    switch (currentDataBitsParam()) {
    case 0:
        port.setDataBits(QSerialPort::Data5);
        break;
    case 1:
        port.setDataBits(QSerialPort::Data6);
        break;
    default:
        port.setDataBits(QSerialPort::Data8);
        break;
    }
    switch (currentParityParam()) {
    case 1:
        port.setParity(QSerialPort::EvenParity);
        break;
    case 2:
        port.setParity(QSerialPort::OddParity);
        break;
    case 3:
        port.setParity(QSerialPort::SpaceParity);
        break;
    case 4:
        port.setParity(QSerialPort::MarkParity);
        break;
    default:
        port.setParity(QSerialPort::NoParity);
        break;
    }
    switch (currentStopBitsParam()) {
    case 1:
        port.setStopBits(QSerialPort::OneAndHalfStop);
        break;
    case 2:
        port.setStopBits(QSerialPort::TwoStop);
        break;
    default:
        port.setStopBits(QSerialPort::OneStop);
        break;
    }
    port.setFlowControl(QSerialPort::NoFlowControl);

    bool wrote = false;
    if (!port.open(QIODevice::ReadWrite)) {
        appendLog(QStringLiteral("[发送][指令] 旁路打开 %1 失败：%2")
                      .arg(portName, port.errorString()));
    } else {
        const qint64 n = port.write(frame);
        port.flush();
        port.waitForBytesWritten(500);
        port.close();
        if (n == frame.size()) {
            wrote = true;
            appendLog(QStringLiteral("[发送][指令] 串口旁路已写出 %1（%2）共 %3 字节")
                          .arg(hex, name)
                          .arg(n));
            appendLog(QStringLiteral("[提示] 试验机日志应出现 [接收][指令] %1").arg(name));
        } else {
            appendLog(QStringLiteral("[发送][指令] 旁路写出不完整：期望%1 实际%2")
                          .arg(frame.size())
                          .arg(n));
        }
    }

    // 恢复库连接，便于继续收测量 / 发数值。
    try {
        applySerialParams();
        const bool ok = comm->Connect();
        appendLog(ok ? QStringLiteral("[连接] 旁路发令后已重连")
                     : QStringLiteral("[连接] 旁路发令后重连失败，请手动连接"));
        if (ok) {
            try {
                comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
                ui.chkAllowSend->blockSignals(true);
                ui.chkAllowSend->setChecked(true);
                ui.chkAllowSend->blockSignals(false);
            } catch (...) {
            }
            setControlsEnabled(true);
        } else {
            setControlsEnabled(false);
        }
    } catch (const std::exception& ex) {
        appendLog(QStringLiteral("[连接] 重连异常：%1").arg(QString::fromLocal8Bit(ex.what())));
        setControlsEnabled(false);
        return false;
    } catch (...) {
        appendLog(QStringLiteral("[连接] 重连异常：未知"));
        setControlsEnabled(false);
        return false;
    }
    return wrote;
}

// 向试验机发出二字节控制指令；UDP 原始写出，串口旁路写出。
bool MainWindow::sendControlCommand(quint8 cmdByte, const QString& name)
{
    if (!ui.btnDisconnect->isEnabled()) {
        appendLog(QStringLiteral("[发送][指令] 未连接，已忽略"));
        return false;
    }

    const QByteArray frame = packControlFrame(cmdByte);
    const bool udp = (ui.cmbCommType->currentIndex() == 0);

    ++m_txAttemptCount;
    refreshCounters();

    if (udp) {
        return sendControlCommandViaUdpRaw(frame, name);
    }
    return sendControlCommandViaSerialBypass(frame, name);
}

// 发出开始计算指令 3D0D。
void MainWindow::onCmdStartClicked()
{
    sendControlCommand(0x3D, QStringLiteral("开始计算"));
}

// 发出停止计算指令 3E0D。
void MainWindow::onCmdStopClicked()
{
    sendControlCommand(0x3E, QStringLiteral("停止计算"));
}

// 发出自动存图指令 6B0D。
void MainWindow::onCmdSaveClicked()
{
    sendControlCommand(0x6B, QStringLiteral("自动存图"));
}

// 发出清零指令 5A0D。
void MainWindow::onCmdZeroClicked()
{
    sendControlCommand(0x5A, QStringLiteral("清零"));
}

void MainWindow::runHeadlessSmoke(const QString& tag, int (*fn)(QString*))
{
    // 自检使用独立 CommHandler 实例，须先释放本窗端口占用。
    if (ui.btnDisconnect->isEnabled()) {
        onDisconnectClicked();
    }

    ui.btnSmokeRx->setEnabled(false);
    ui.btnSmokeTx->setEnabled(false);
    ui.btnConnect->setEnabled(false);

    QString detail;
    const int rc = fn(&detail);
    if (rc == 0) {
        appendLog(QStringLiteral("[%1] 通过：%2").arg(tag, detail));
        setStatus(QStringLiteral("%1通过").arg(tag));
    } else {
        ++m_errorCount;
        refreshCounters();
        appendLog(QStringLiteral("[%1] 失败：%2").arg(tag, detail));
        setStatus(QStringLiteral("自检失败"));
    }

    setControlsEnabled(false);
    onCommTypeChanged(ui.cmbCommType->currentIndex());
}

// UI 入口：切换到协议 3 后跑三思接收自检。
void MainWindow::onSmokeUdpSansiClicked()
{
    ui.cmbCommType->setCurrentIndex(0);
    onCommTypeChanged(0);
    for (int i = 0; i < ui.cmbNetProto->count(); ++i) {
        if (ui.cmbNetProto->itemData(i).toInt() == 3) {
            ui.cmbNetProto->setCurrentIndex(i);
            break;
        }
    }
    runHeadlessSmoke(QStringLiteral("自检-收"), RunUdpSansiSmoke);
}

// UI 入口：切换到协议 0 后跑 JSON 发送自检。
void MainWindow::onSmokeUdpSendClicked()
{
    ui.cmbCommType->setCurrentIndex(0);
    onCommTypeChanged(0);
    for (int i = 0; i < ui.cmbNetProto->count(); ++i) {
        if (ui.cmbNetProto->itemData(i).toInt() == 0) {
            ui.cmbNetProto->setCurrentIndex(i);
            break;
        }
    }
    runHeadlessSmoke(QStringLiteral("自检-发"), RunUdpJsonSendSmoke);
}

// 清空运行记录文本框。
void MainWindow::onClearLogClicked()
{
    ui.textLog->clear();
}

// 弹出对话框将运行记录保存为文件。
void MainWindow::onSaveLogClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存记录"), QStringLiteral("commlab_log.txt"),
        QStringLiteral("文本文件 (*.txt)"));
    if (path.isEmpty()) {
        return;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), f.errorString());
        return;
    }
    f.write(ui.textLog->toPlainText().toUtf8());
    appendLog(QStringLiteral("[记录] 已保存到 %1").arg(path));
}

// 展示测得数值回调。
void MainWindow::onSafeData(QVector<double> values, int type)
{
    ++m_rxDataCount;
    refreshCounters();
    appendLog(QStringLiteral("[接收][测量] 类型=%1，个数=%2，数值=%3")
                  .arg(type)
                  .arg(values.size())
                  .arg(formatValues(values)));
}

// 展示库解析的控制事件。
void MainWindow::onEvent(int ctrlCmd, int viewId, int msg)
{
    ++m_rxEventCount;
    refreshCounters();
    const QString name = CommController::eventName(msg);
    const QString hexHint = serialEventHexHint(msg);
    if (hexHint.isEmpty()) {
        appendLog(QStringLiteral("[接收][事件] 控制码=%1 视窗=%2 消息=%3 名称=%4")
                      .arg(ctrlCmd)
                      .arg(viewId)
                      .arg(msg)
                      .arg(name));
    } else {
        appendLog(QStringLiteral("[接收][事件] %1 | 控制码=%2 视窗=%3 | %4")
                      .arg(name)
                      .arg(ctrlCmd)
                      .arg(viewId)
                      .arg(hexHint));
    }
}

// 展示新连接通知。
void MainWindow::onConn(int iType, QString ip, int port)
{
    appendLog(QStringLiteral("[对端连接] 类型=%1 IP=%2，端口=%3").arg(iType).arg(ip).arg(port));
}

// 展示对端断开。
void MainWindow::onClientGone()
{
    appendLog(QStringLiteral("[断开] 对端已主动断开"));
}

// 转发 CommController 日志到运行记录。
void MainWindow::onControllerLog(QString text)
{
    // 仅警告/错误计入错误计数；空数据回调等 [收] 提示不抬高错误计数。
    if (text.contains(QLatin1String("[警告]")) || text.contains(QLatin1String("[错误]"))) {
        ++m_errorCount;
        refreshCounters();
    }
    appendLog(text);
}