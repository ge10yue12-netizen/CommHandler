#include "MainWindow.h"

#include "HexUtil.h"
#include "LegacyCapability.h"

#include <QRegularExpression>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QUuid>
#include <QVBoxLayout>

namespace {

QString appStyleSheet()
{
    return QStringLiteral(
        "QMainWindow { background: #F3F6F9; }"
        "QScrollArea#SideScroll { background: #EEF2F6; border: none; }"
        "QWidget#SidePanel {"
        "  background: #EEF2F6;"
        "  border-right: 1px solid #D8DEE6;"
        "}"
        "QWidget#MainPanel { background: #F3F6F9; }"
        "QLabel#AppTitle {"
        "  font-size: 15px; font-weight: 600; color: #1F2937; padding: 4px 0 12px 0;"
        "}"
        "QLabel.Section {"
        "  font-size: 12px; font-weight: 600; color: #4B5563; padding-top: 10px;"
        "}"
        "QComboBox, QLineEdit, QSpinBox, QPlainTextEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #D1D9E3;"
        "  border-radius: 8px;"
        "  padding: 6px 8px;"
        "  min-height: 20px;"
        "  color: #111827;"
        "}"
        "QComboBox:focus, QLineEdit:focus, QSpinBox:focus, QPlainTextEdit:focus {"
        "  border: 1px solid #3B82F6;"
        "}"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QPushButton#PrimaryButton {"
        "  background: #3B82F6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#PrimaryButton:disabled { background: #93C5FD; }"
        "QPushButton#PrimaryButton:hover:!disabled { background: #2563EB; }"
        "QPushButton#GhostButton {"
        "  background: transparent;"
        "  color: #2563EB;"
        "  border: none;"
        "  text-align: left;"
        "  padding: 4px 0;"
        "}"
        "QPushButton#SecondaryButton {"
        "  background: #FFFFFF;"
        "  color: #374151;"
        "  border: 1px solid #D1D9E3;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "}"
        "QPushButton#SecondaryButton:disabled { color: #9CA3AF; }"
        "QPushButton#SendButton {"
        "  background: #3B82F6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 12px;"
        "  min-width: 56px;"
        "  min-height: 56px;"
        "  font-size: 18px;"
        "  font-weight: 700;"
        "}"
        "QPushButton#SendButton:disabled { background: #93C5FD; }"
        "QPlainTextEdit#LogView {"
        "  background: #FFFFFF;"
        "  border: 1px solid #D8DEE6;"
        "  border-radius: 12px;"
        "  padding: 8px;"
        "  font-family: Consolas, 'Microsoft YaHei UI';"
        "}"
        "QPlainTextEdit#SendEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #D8DEE6;"
        "  border-radius: 12px;"
        "  min-height: 56px;"
        "}"
        "QLabel#StatusPill {"
        "  background: #E8F0FE;"
        "  color: #1D4ED8;"
        "  border-radius: 8px;"
        "  padding: 6px 10px;"
        "}"
        "QLabel#CapTip {"
        "  color: #6B7280;"
        "  font-size: 11px;"
        "  padding: 4px 0;"
        "}"
        "QCheckBox { color: #374151; spacing: 6px; }"
        "QLabel#FooterStat { color: #6B7280; font-size: 12px; }"
    );
}

void fillNetworkProtocols(QComboBox* box)
{
    box->clear();
    box->addItem(QStringLiteral("0 JSON"), 0);
    box->addItem(QStringLiteral("1 万测"), 1);
    box->addItem(QStringLiteral("2 中机"), 2);
    box->addItem(QStringLiteral("3 三思"), 3);
    box->addItem(QStringLiteral("4 触发存图"), 4);
    box->addItem(QStringLiteral("5 福建威盛"), 5);
    box->addItem(QStringLiteral("6 纳百川自动化"), 6);
    box->addItem(QStringLiteral("7 纳百川线条"), 7);
}

void fillSerialProtocols(QComboBox* box)
{
    box->clear();
    // 标签对齐 CommLab ProtoGuide / 基线 §14.3
    box->addItem(QStringLiteral("0 三思"), 0);
    box->addItem(QStringLiteral("1 科新"), 1);
    box->addItem(QStringLiteral("2 时代新材"), 2);
    box->addItem(QStringLiteral("3 IEEE（≥5 值）"), 3);
    box->addItem(QStringLiteral("4 冠腾（≥2 值）"), 4);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , manager_(this)
    , session_(&manager_, this)
    , legacySession_(&manager_, this)
    , scheduler_(this)
    , capture_(this)
{
    setWindowTitle(QStringLiteral("通信调试助手 — Native / Legacy"));
    // 默认中等尺寸；侧栏可滚动，避免控件数量把窗口撑过高
    resize(960, 600);
    setMinimumSize(800, 520);
    applyStyle();
    buildUi();

    scheduler_.setSession(&session_);

    connect(&session_, &ca::ICommSession::recordReceived, this, &MainWindow::onRecordReceived);
    connect(&session_, &ca::ICommSession::stateChanged, this, &MainWindow::onSessionStateChanged);
    connect(&session_, &ca::NativeSession::channelsChanged, this, &MainWindow::onChannelsChanged);

    connect(&legacySession_, &ca::ICommSession::recordReceived, this, &MainWindow::onRecordReceived);
    connect(&legacySession_, &ca::ICommSession::stateChanged, this, &MainWindow::onSessionStateChanged);
    connect(&legacySession_, &ca::LegacySession::unresponsive, this, &MainWindow::onLegacyUnresponsive);

    connect(&scheduler_, &ca::SendScheduler::taskStopped, this, &MainWindow::onSchedTaskStopped);
    connect(&scheduler_, &ca::SendScheduler::taskFailed, this, &MainWindow::onSchedTaskFailed);
    connect(&capture_, &ca::CaptureManager::captureFailed, this, &MainWindow::onCaptureFailed);
    connect(&capture_, &ca::CaptureManager::captureStarted, this, &MainWindow::onCaptureStarted);

    capture_.setOutputDirectory(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("captures")));

    appendLog(QStringLiteral("就绪：Native（串口/TCP/UDP）与 Legacy DLL（可 Mock 或真实 CommHandler）。"),
              QStringLiteral("sys"));
    refreshLegacyCapabilityTips();
    syncUi();
}

MainWindow::~MainWindow()
{
    scheduler_.stopAll();
    capture_.stopAll();
    session_.close();
    legacySession_.close();
}

void MainWindow::applyStyle()
{
    setStyleSheet(appStyleSheet());
}

bool MainWindow::isLegacyMode() const
{
    return workModeCombo_ && workModeCombo_->currentData().toInt() == static_cast<int>(ca::WorkMode::LegacyDll);
}

ca::ICommSession* MainWindow::activeSession()
{
    return isLegacyMode() ? static_cast<ca::ICommSession*>(&legacySession_)
                          : static_cast<ca::ICommSession*>(&session_);
}

const ca::ICommSession* MainWindow::activeSession() const
{
    return isLegacyMode() ? static_cast<const ca::ICommSession*>(&legacySession_)
                          : static_cast<const ca::ICommSession*>(&session_);
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 侧栏放入滚动区：矮屏下可滚，主窗不被参数区最小高度抬高
    auto* sideScroll = new QScrollArea(central);
    sideScroll->setObjectName(QStringLiteral("SideScroll"));
    sideScroll->setWidgetResizable(true);
    sideScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sideScroll->setFrameShape(QFrame::NoFrame);
    sideScroll->setFixedWidth(340);

    auto* side = new QWidget();
    side->setObjectName(QStringLiteral("SidePanel"));
    side->setMinimumWidth(300);
    auto* sideLay = new QVBoxLayout(side);
    sideLay->setContentsMargins(16, 16, 16, 16);
    sideLay->setSpacing(6);

    auto* title = new QLabel(QStringLiteral("通信调试助手"), side);
    title->setObjectName(QStringLiteral("AppTitle"));
    sideLay->addWidget(title);

    auto* workLabel = new QLabel(QStringLiteral("工作模式"), side);
    workLabel->setProperty("class", "Section");
    sideLay->addWidget(workLabel);

    workModeCombo_ = new QComboBox(side);
    workModeCombo_->addItem(QStringLiteral("原生 Native"), static_cast<int>(ca::WorkMode::Native));
    workModeCombo_->addItem(QStringLiteral("兼容 Legacy DLL"), static_cast<int>(ca::WorkMode::LegacyDll));
    sideLay->addWidget(workModeCombo_);

    auto* modeLabel = new QLabel(QStringLiteral("连接方式"), side);
    modeLabel->setProperty("class", "Section");
    sideLay->addWidget(modeLabel);

    transportCombo_ = new QComboBox(side);
    transportCombo_->addItem(QStringLiteral("串口"), static_cast<int>(ca::TransportKind::Serial));
    transportCombo_->addItem(QStringLiteral("TCP 客户端"), static_cast<int>(ca::TransportKind::TcpClient));
    transportCombo_->addItem(QStringLiteral("TCP 服务端"), static_cast<int>(ca::TransportKind::TcpServer));
    transportCombo_->addItem(QStringLiteral("UDP"), static_cast<int>(ca::TransportKind::Udp));
    sideLay->addWidget(transportCombo_);

    paramStack_ = new QStackedWidget(side);

    // —— 串口页 ——
    auto* serialPage = new QWidget(paramStack_);
    auto* serialForm = new QFormLayout(serialPage);
    serialForm->setContentsMargins(0, 8, 0, 0);
    serialForm->setSpacing(8);
    portCombo_ = new QComboBox(serialPage);
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& p : ports) {
        const QString tip = p.description().isEmpty() ? p.portName()
                                                      : QStringLiteral("%1（%2）").arg(p.portName(), p.description());
        portCombo_->addItem(tip, p.portName());
    }
    if (portCombo_->count() == 0)
        portCombo_->addItem(QStringLiteral("COM3"), QStringLiteral("COM3"));

    baudCombo_ = new QComboBox(serialPage);
    for (int b : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600})
        baudCombo_->addItem(QString::number(b), b);
    baudCombo_->setCurrentText(QStringLiteral("115200"));

    dataBitsCombo_ = new QComboBox(serialPage);
    dataBitsCombo_->addItem(QStringLiteral("8"), 8);
    dataBitsCombo_->addItem(QStringLiteral("7"), 7);
    dataBitsCombo_->addItem(QStringLiteral("6"), 6);
    dataBitsCombo_->addItem(QStringLiteral("5"), 5);

    parityCombo_ = new QComboBox(serialPage);
    parityCombo_->addItem(QStringLiteral("无"), QStringLiteral("none"));
    parityCombo_->addItem(QStringLiteral("偶"), QStringLiteral("even"));
    parityCombo_->addItem(QStringLiteral("奇"), QStringLiteral("odd"));

    stopBitsCombo_ = new QComboBox(serialPage);
    stopBitsCombo_->addItem(QStringLiteral("1"), QStringLiteral("1"));
    stopBitsCombo_->addItem(QStringLiteral("1.5"), QStringLiteral("1.5"));
    stopBitsCombo_->addItem(QStringLiteral("2"), QStringLiteral("2"));

    serialForm->addRow(QStringLiteral("端口"), portCombo_);
    serialForm->addRow(QStringLiteral("波特率"), baudCombo_);
    serialForm->addRow(QStringLiteral("数据位"), dataBitsCombo_);
    serialForm->addRow(QStringLiteral("校验"), parityCombo_);
    serialForm->addRow(QStringLiteral("停止位"), stopBitsCombo_);
    paramStack_->addWidget(serialPage);

    // —— TCP 客户端页 ——
    auto* tcpPage = new QWidget(paramStack_);
    auto* tcpForm = new QFormLayout(tcpPage);
    tcpForm->setContentsMargins(0, 8, 0, 0);
    tcpForm->setSpacing(8);
    tcpHostEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), tcpPage);
    tcpPortSpin_ = new QSpinBox(tcpPage);
    tcpPortSpin_->setRange(1, 65535);
    tcpPortSpin_->setValue(9000);
    tcpForm->addRow(QStringLiteral("远端地址"), tcpHostEdit_);
    tcpForm->addRow(QStringLiteral("远端端口"), tcpPortSpin_);
    paramStack_->addWidget(tcpPage);

    // —— TCP 服务端页 ——
    auto* tcpServerPage = new QWidget(paramStack_);
    auto* tcpServerForm = new QFormLayout(tcpServerPage);
    tcpServerForm->setContentsMargins(0, 8, 0, 0);
    tcpServerForm->setSpacing(8);
    tcpServerAddrEdit_ = new QLineEdit(QStringLiteral("0.0.0.0"), tcpServerPage);
    tcpServerPortSpin_ = new QSpinBox(tcpServerPage);
    tcpServerPortSpin_->setRange(1, 65535);
    tcpServerPortSpin_->setValue(9000);
    tcpServerForm->addRow(QStringLiteral("本地地址"), tcpServerAddrEdit_);
    tcpServerForm->addRow(QStringLiteral("本地端口"), tcpServerPortSpin_);
    paramStack_->addWidget(tcpServerPage);

    // —— UDP 页 ——
    auto* udpPage = new QWidget(paramStack_);
    auto* udpForm = new QFormLayout(udpPage);
    udpForm->setContentsMargins(0, 8, 0, 0);
    udpForm->setSpacing(8);
    udpLocalAddrEdit_ = new QLineEdit(QStringLiteral("0.0.0.0"), udpPage);
    udpLocalPortSpin_ = new QSpinBox(udpPage);
    udpLocalPortSpin_->setRange(1, 65535);
    udpLocalPortSpin_->setValue(9001);
    udpRemoteAddrEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), udpPage);
    udpRemotePortSpin_ = new QSpinBox(udpPage);
    udpRemotePortSpin_->setRange(1, 65535);
    udpRemotePortSpin_->setValue(9002);
    udpForm->addRow(QStringLiteral("本地地址"), udpLocalAddrEdit_);
    udpForm->addRow(QStringLiteral("本地端口"), udpLocalPortSpin_);
    udpForm->addRow(QStringLiteral("远端地址"), udpRemoteAddrEdit_);
    udpForm->addRow(QStringLiteral("远端端口"), udpRemotePortSpin_);
    paramStack_->addWidget(udpPage);

    // —— Legacy 页 ——
    auto* legacyPage = new QWidget(paramStack_);
    auto* legacyForm = new QFormLayout(legacyPage);
    legacyForm->setContentsMargins(0, 8, 0, 0);
    legacyForm->setSpacing(8);
    legacyCommTypeCombo_ = new QComboBox(legacyPage);
    legacyCommTypeCombo_->addItem(QStringLiteral("网口 Network"), 0);
    legacyCommTypeCombo_->addItem(QStringLiteral("串口 Serial"), 1);
    legacyProtocolCombo_ = new QComboBox(legacyPage);
    fillNetworkProtocols(legacyProtocolCombo_);

    legacyEndpointStack_ = new QStackedWidget(legacyPage);
    auto* legacyNet = new QWidget(legacyEndpointStack_);
    auto* legacyNetForm = new QFormLayout(legacyNet);
    legacyNetForm->setContentsMargins(0, 0, 0, 0);
    legacyTransferCombo_ = new QComboBox(legacyNet);
    legacyTransferCombo_->addItem(QStringLiteral("TCP"), 0);
    legacyTransferCombo_->addItem(QStringLiteral("UDP"), 1);
    legacyModelCombo_ = new QComboBox(legacyNet);
    legacyModelCombo_->addItem(QStringLiteral("服务端"), 0);
    legacyModelCombo_->addItem(QStringLiteral("客户端"), 1);
    legacyModelCombo_->setCurrentIndex(1);
    legacyLocalAddrEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), legacyNet);
    legacyLocalPortSpin_ = new QSpinBox(legacyNet);
    legacyLocalPortSpin_->setRange(0, 65535);
    legacyLocalPortSpin_->setValue(0);
    legacyRemoteAddrEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"), legacyNet);
    legacyRemotePortSpin_ = new QSpinBox(legacyNet);
    legacyRemotePortSpin_->setRange(1, 65535);
    legacyRemotePortSpin_->setValue(9000);
    legacyNetForm->addRow(QStringLiteral("传输"), legacyTransferCombo_);
    legacyNetForm->addRow(QStringLiteral("角色"), legacyModelCombo_);
    legacyNetForm->addRow(QStringLiteral("本地地址"), legacyLocalAddrEdit_);
    legacyNetForm->addRow(QStringLiteral("本地端口"), legacyLocalPortSpin_);
    legacyNetForm->addRow(QStringLiteral("远端地址"), legacyRemoteAddrEdit_);
    legacyNetForm->addRow(QStringLiteral("远端端口"), legacyRemotePortSpin_);
    legacyEndpointStack_->addWidget(legacyNet);

    auto* legacySer = new QWidget(legacyEndpointStack_);
    auto* legacySerForm = new QFormLayout(legacySer);
    legacySerForm->setContentsMargins(0, 0, 0, 0);
    legacyPortCombo_ = new QComboBox(legacySer);
    for (int i = 0; i < portCombo_->count(); ++i)
        legacyPortCombo_->addItem(portCombo_->itemText(i), portCombo_->itemData(i));
    legacyBaudCombo_ = new QComboBox(legacySer);
    for (int b : {4800, 9600, 19200, 38400, 57600, 115200})
        legacyBaudCombo_->addItem(QString::number(b), b);
    legacyBaudCombo_->setCurrentText(QStringLiteral("115200"));
    legacySerForm->addRow(QStringLiteral("端口"), legacyPortCombo_);
    legacySerForm->addRow(QStringLiteral("波特率"), legacyBaudCombo_);
    legacyEndpointStack_->addWidget(legacySer);

    legacyMockCheck_ = new QCheckBox(QStringLiteral("使用 Mock 后端（不调 DLL）"), legacyPage);
#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
    legacyMockCheck_->setChecked(false);
#else
    legacyMockCheck_->setChecked(true);
#endif
    legacyCapTipLabel_ = new QLabel(legacyPage);
    legacyCapTipLabel_->setObjectName(QStringLiteral("CapTip"));
    legacyCapTipLabel_->setWordWrap(true);

    legacyForm->addRow(QStringLiteral("通信类型"), legacyCommTypeCombo_);
    legacyForm->addRow(QStringLiteral("协议"), legacyProtocolCombo_);
    legacyForm->addRow(QStringLiteral("端点"), legacyEndpointStack_);
    legacyForm->addRow(QString(), legacyMockCheck_);
    legacyForm->addRow(QStringLiteral("能力"), legacyCapTipLabel_);
    paramStack_->addWidget(legacyPage); // index 4

    sideLay->addWidget(paramStack_);

    btnOpen_ = new QPushButton(QStringLiteral("打开连接"), side);
    btnOpen_->setObjectName(QStringLiteral("PrimaryButton"));
    sideLay->addWidget(btnOpen_);

    statusLabel_ = new QLabel(QStringLiteral("状态：已创建"), side);
    statusLabel_->setObjectName(QStringLiteral("StatusPill"));
    sideLay->addWidget(statusLabel_);

    clientSectionLabel_ = new QLabel(QStringLiteral("已连接客户端"), side);
    clientSectionLabel_->setProperty("class", "Section");
    sideLay->addWidget(clientSectionLabel_);

    clientCombo_ = new QComboBox(side);
    clientCombo_->addItem(QStringLiteral("全部（广播）"), QString());
    sideLay->addWidget(clientCombo_);

    btnDisconnectClient_ = new QPushButton(QStringLiteral("断开选中客户端"), side);
    btnDisconnectClient_->setObjectName(QStringLiteral("SecondaryButton"));
    sideLay->addWidget(btnDisconnectClient_);

    auto* rxSec = new QLabel(QStringLiteral("接收设置"), side);
    rxSec->setProperty("class", "Section");
    sideLay->addWidget(rxSec);
    hexDisplayCheck_ = new QCheckBox(QStringLiteral("十六进制显示"), side);
    hexDisplayCheck_->setChecked(true);
    sideLay->addWidget(hexDisplayCheck_);

    auto* txSec = new QLabel(QStringLiteral("发送设置"), side);
    txSec->setProperty("class", "Section");
    sideLay->addWidget(txSec);
    hexSendCheck_ = new QCheckBox(QStringLiteral("十六进制发送"), side);
    hexSendCheck_->setChecked(true);
    sideLay->addWidget(hexSendCheck_);
    captureEnableCheck_ = new QCheckBox(QStringLiteral("抓包落盘（JSONL）"), side);
    captureEnableCheck_->setChecked(true);
    sideLay->addWidget(captureEnableCheck_);

    auto* schedSec = new QLabel(QStringLiteral("发送调度"), side);
    schedSec->setProperty("class", "Section");
    sideLay->addWidget(schedSec);

    schedModeCombo_ = new QComboBox(side);
    schedModeCombo_->addItem(QStringLiteral("单次"), static_cast<int>(ca::ScheduleMode::Once));
    schedModeCombo_->addItem(QStringLiteral("周期（无限）"), static_cast<int>(ca::ScheduleMode::Infinite));
    schedModeCombo_->addItem(QStringLiteral("指定次数"), static_cast<int>(ca::ScheduleMode::Counted));
    schedModeCombo_->addItem(QStringLiteral("列表轮询"), static_cast<int>(ca::ScheduleMode::RoundRobin));
    sideLay->addWidget(schedModeCombo_);

    schedIntervalSpin_ = new QSpinBox(side);
    schedIntervalSpin_->setRange(0, 3600000);
    schedIntervalSpin_->setValue(1000);
    schedIntervalSpin_->setSuffix(QStringLiteral(" ms"));
    sideLay->addWidget(schedIntervalSpin_);

    schedCountSpin_ = new QSpinBox(side);
    schedCountSpin_->setRange(1, 1000000);
    schedCountSpin_->setValue(10);
    schedCountSpin_->setPrefix(QStringLiteral("次数 "));
    sideLay->addWidget(schedCountSpin_);

    btnSchedStart_ = new QPushButton(QStringLiteral("启动调度"), side);
    btnSchedStart_->setObjectName(QStringLiteral("SecondaryButton"));
    sideLay->addWidget(btnSchedStart_);
    btnSchedPause_ = new QPushButton(QStringLiteral("暂停"), side);
    btnSchedPause_->setObjectName(QStringLiteral("SecondaryButton"));
    sideLay->addWidget(btnSchedPause_);
    btnSchedResume_ = new QPushButton(QStringLiteral("继续"), side);
    btnSchedResume_->setObjectName(QStringLiteral("SecondaryButton"));
    sideLay->addWidget(btnSchedResume_);
    btnSchedStop_ = new QPushButton(QStringLiteral("停止调度"), side);
    btnSchedStop_->setObjectName(QStringLiteral("SecondaryButton"));
    sideLay->addWidget(btnSchedStop_);

    btnClear_ = new QPushButton(QStringLiteral("清空数据"), side);
    btnClear_->setObjectName(QStringLiteral("GhostButton"));
    sideLay->addWidget(btnClear_);
    sideLay->addStretch(1);
    sideScroll->setWidget(side);

    auto* main = new QWidget(central);
    main->setObjectName(QStringLiteral("MainPanel"));
    auto* mainLay = new QVBoxLayout(main);
    mainLay->setContentsMargins(16, 16, 16, 12);
    mainLay->setSpacing(10);

    log_ = new QPlainTextEdit(main);
    log_->setObjectName(QStringLiteral("LogView"));
    log_->setReadOnly(true);
    mainLay->addWidget(log_, 1);

    auto* sendRow = new QHBoxLayout();
    sendEdit_ = new QPlainTextEdit(main);
    sendEdit_->setObjectName(QStringLiteral("SendEdit"));
    sendEdit_->setPlaceholderText(QStringLiteral("Native：HEX/文本；Legacy：数值 CSV 或透明文本"));
    sendEdit_->setMaximumHeight(72);
    btnSend_ = new QPushButton(QStringLiteral("➤"), main);
    btnSend_->setObjectName(QStringLiteral("SendButton"));
    btnSend_->setToolTip(QStringLiteral("发送"));
    sendRow->addWidget(sendEdit_, 1);
    sendRow->addWidget(btnSend_);
    mainLay->addLayout(sendRow);

    auto* footer = new QHBoxLayout();
    txCountLabel_ = new QLabel(QStringLiteral("发送：0"), main);
    txCountLabel_->setObjectName(QStringLiteral("FooterStat"));
    rxCountLabel_ = new QLabel(QStringLiteral("接收：0 - 0"), main);
    rxCountLabel_->setObjectName(QStringLiteral("FooterStat"));
    btnResetCount_ = new QPushButton(QStringLiteral("复位计数"), main);
    btnResetCount_->setObjectName(QStringLiteral("GhostButton"));
    footer->addWidget(txCountLabel_);
    footer->addSpacing(16);
    footer->addWidget(rxCountLabel_);
    footer->addStretch(1);
    footer->addWidget(btnResetCount_);
    mainLay->addLayout(footer);

    root->addWidget(sideScroll);
    root->addWidget(main, 1);

    connect(workModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onWorkModeChanged);
    connect(transportCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTransportChanged);
    connect(legacyCommTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onLegacyCommOrProtocolChanged);
    connect(legacyProtocolCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onLegacyCommOrProtocolChanged);
    connect(btnOpen_, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    connect(btnSend_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(btnClear_, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    connect(btnDisconnectClient_, &QPushButton::clicked, this, &MainWindow::onDisconnectClientClicked);
    connect(btnSchedStart_, &QPushButton::clicked, this, &MainWindow::onSchedStartClicked);
    connect(btnSchedPause_, &QPushButton::clicked, this, &MainWindow::onSchedPauseClicked);
    connect(btnSchedResume_, &QPushButton::clicked, this, &MainWindow::onSchedResumeClicked);
    connect(btnSchedStop_, &QPushButton::clicked, this, &MainWindow::onSchedStopClicked);
    connect(schedModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { syncUi(); });
    connect(clientCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { syncUi(); });
    connect(btnResetCount_, &QPushButton::clicked, this, [this]() {
        txBytes_ = 0;
        rxBytes_ = 0;
        rxChunks_ = 0;
        updateCounters();
    });
}

void MainWindow::onWorkModeChanged(int)
{
    if (isLegacyMode()) {
        paramStack_->setCurrentIndex(4);
        transportCombo_->setVisible(false);
        hexSendCheck_->setChecked(false);
        scheduler_.setSession(&legacySession_);
    } else {
        paramStack_->setCurrentIndex(transportCombo_->currentIndex());
        transportCombo_->setVisible(true);
        hexSendCheck_->setChecked(true);
        scheduler_.setSession(&session_);
    }
    refreshLegacyCapabilityTips();
    syncUi();
}

void MainWindow::onTransportChanged(int index)
{
    if (!isLegacyMode())
        paramStack_->setCurrentIndex(index);
    syncUi();
}

void MainWindow::onLegacyCommOrProtocolChanged()
{
    const int comm = legacyCommTypeCombo_->currentData().toInt();
    legacyEndpointStack_->setCurrentIndex(comm == 1 ? 1 : 0);
    const int prevProto = legacyProtocolCombo_->currentData().toInt();
    if (comm == 1)
        fillSerialProtocols(legacyProtocolCombo_);
    else
        fillNetworkProtocols(legacyProtocolCombo_);
    const int idx = legacyProtocolCombo_->findData(prevProto);
    if (idx >= 0)
        legacyProtocolCombo_->setCurrentIndex(idx);
    refreshLegacyCapabilityTips();
    syncUi();
}

void MainWindow::refreshLegacyCapabilityTips()
{
    if (!legacyCapTipLabel_ || !legacyCommTypeCombo_ || !legacyProtocolCombo_)
        return;
    const ca::LegacyCommKind kind =
        (legacyCommTypeCombo_->currentData().toInt() == 1) ? ca::LegacyCommKind::Serial : ca::LegacyCommKind::Network;
    const ca::LegacyCapabilityProfile profile =
        ca::legacyCapabilityFor(kind, legacyProtocolCombo_->currentData().toInt());
    QStringList tips;
    tips << (profile.supports(ca::LegacyCapability::ReceiveValues) ? QStringLiteral("收数值✓")
                                                                   : QStringLiteral("收数值✗"));
    tips << (profile.supports(ca::LegacyCapability::SendEncodedValues) ? QStringLiteral("发数值✓")
                                                                       : QStringLiteral("发数值✗"));
    tips << (profile.supports(ca::LegacyCapability::SendTransparentText) ? QStringLiteral("发文本✓")
                                                                         : QStringLiteral("发文本✗"));
    tips << (profile.supports(ca::LegacyCapability::ReceiveControlEvents) ? QStringLiteral("控制事件✓")
                                                                          : QStringLiteral("控制事件✗"));
    const QString lim = profile.entries.value(static_cast<int>(ca::LegacyCapability::SendEncodedValues)).limitation;
    if (!lim.isEmpty())
        tips << lim;
    legacyCapTipLabel_->setText(tips.join(QStringLiteral(" · ")));
}

void MainWindow::appendLog(const QString& line, const QString& cssClass)
{
    Q_UNUSED(cssClass);
    const QString stamped = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line;
    log_->appendPlainText(stamped);
}

void MainWindow::updateCounters()
{
    txCountLabel_->setText(QStringLiteral("发送：%1").arg(txBytes_));
    rxCountLabel_->setText(QStringLiteral("接收：%1 - %2").arg(rxBytes_).arg(rxChunks_));
}

void MainWindow::refreshClientCombo()
{
    if (!clientCombo_)
        return;

    const QString prevId = clientCombo_->currentData().toString();
    clientCombo_->blockSignals(true);
    clientCombo_->clear();
    clientCombo_->addItem(QStringLiteral("全部（广播）"), QString());

    const QVector<ca::TcpChannelInfo> channels = session_.tcpChannels();
    int restoreIndex = 0;
    for (int i = 0; i < channels.size(); ++i) {
        const ca::TcpChannelInfo& ch = channels.at(i);
        const QString label = QStringLiteral("%1:%2").arg(ch.remoteAddress).arg(ch.remotePort);
        clientCombo_->addItem(label, ch.channelId);
        if (ch.channelId == prevId)
            restoreIndex = i + 1;
    }
    clientCombo_->setCurrentIndex(restoreIndex);
    clientCombo_->blockSignals(false);
}

void MainWindow::syncUi()
{
    const ca::SessionState st = activeSession()->state();
    const bool connected = (st == ca::SessionState::Connected);
    const bool canOpen = (st == ca::SessionState::Created || st == ca::SessionState::Closed);
    const bool canConfig = canOpen;
    const bool legacy = isLegacyMode();

    const int kind = transportCombo_->currentData().toInt();
    const bool isTcpServer = !legacy && (kind == static_cast<int>(ca::TransportKind::TcpServer));
    const bool showClients = isTcpServer && connected;

    if (workModeCombo_)
        workModeCombo_->setEnabled(canConfig);
    transportCombo_->setEnabled(canConfig && !legacy);
    paramStack_->setEnabled(canConfig);

    bool canSend = connected;
    if (legacy && connected) {
        const ca::LegacyCapabilityProfile& p = legacySession_.capabilityProfile();
        canSend = p.supports(ca::LegacyCapability::SendEncodedValues)
                  || p.supports(ca::LegacyCapability::SendTransparentText);
    }
    btnSend_->setEnabled(canSend);
    if (hexSendCheck_)
        hexSendCheck_->setEnabled(!legacy);
    if (hexDisplayCheck_)
        hexDisplayCheck_->setEnabled(!legacy);

    const bool schedActive = !activeSchedTaskId_.isNull() && scheduler_.hasTask(activeSchedTaskId_);
    const bool schedPaused = schedActive && scheduler_.isPaused(activeSchedTaskId_);
    const int mode = schedModeCombo_ ? schedModeCombo_->currentData().toInt() : 0;
    if (schedCountSpin_)
        schedCountSpin_->setEnabled(mode == static_cast<int>(ca::ScheduleMode::Counted)
                                    || mode == static_cast<int>(ca::ScheduleMode::RoundRobin));
    if (schedIntervalSpin_)
        schedIntervalSpin_->setEnabled(mode != static_cast<int>(ca::ScheduleMode::Once));
    if (btnSchedStart_)
        btnSchedStart_->setEnabled(canSend && !schedActive);
    if (btnSchedPause_)
        btnSchedPause_->setEnabled(connected && schedActive && !schedPaused);
    if (btnSchedResume_)
        btnSchedResume_->setEnabled(connected && schedActive && schedPaused);
    if (btnSchedStop_)
        btnSchedStop_->setEnabled(schedActive);
    if (schedModeCombo_)
        schedModeCombo_->setEnabled(connected && !schedActive);

    if (clientSectionLabel_)
        clientSectionLabel_->setVisible(showClients);
    if (clientCombo_)
        clientCombo_->setVisible(showClients);
    if (btnDisconnectClient_) {
        btnDisconnectClient_->setVisible(showClients);
        btnDisconnectClient_->setEnabled(showClients && !clientCombo_->currentData().toString().isEmpty());
    }

    if (st == ca::SessionState::Unresponsive) {
        btnOpen_->setText(QStringLiteral("不可响应（请重启）"));
        btnOpen_->setEnabled(false);
    } else if (connected) {
        btnOpen_->setText(QStringLiteral("关闭连接"));
        btnOpen_->setEnabled(true);
    } else if (st == ca::SessionState::Opening || st == ca::SessionState::Closing) {
        btnOpen_->setText(QStringLiteral("请稍候…"));
        btnOpen_->setEnabled(false);
    } else {
        btnOpen_->setText(QStringLiteral("打开连接"));
        btnOpen_->setEnabled(canOpen);
    }

    QString stateText = QStringLiteral("已创建");
    switch (st) {
    case ca::SessionState::Opening: stateText = QStringLiteral("正在打开"); break;
    case ca::SessionState::Connected: stateText = QStringLiteral("已连接"); break;
    case ca::SessionState::Closing: stateText = QStringLiteral("正在关闭"); break;
    case ca::SessionState::Closed: stateText = QStringLiteral("已关闭"); break;
    case ca::SessionState::Faulted: stateText = QStringLiteral("故障"); break;
    case ca::SessionState::Unresponsive: stateText = QStringLiteral("不可响应"); break;
    default: break;
    }
    statusLabel_->setText(QStringLiteral("状态：%1").arg(stateText));

    if (!showClients && clientCombo_)
        refreshClientCombo();
}

ca::SessionConfig MainWindow::buildConfig() const
{
    ca::SessionConfig cfg;
    cfg.sessionId = QUuid::createUuid();
    cfg.name = QStringLiteral("ui-session");
    cfg.role = ca::SessionRole::Tool;

    if (isLegacyMode()) {
        cfg.mode = ca::WorkMode::LegacyDll;
        cfg.transport.legacy.commType = legacyCommTypeCombo_->currentData().toInt();
        cfg.transport.legacy.protocolIndex = legacyProtocolCombo_->currentData().toInt();
        cfg.transport.legacy.useMockBackend = legacyMockCheck_ && legacyMockCheck_->isChecked();
        if (cfg.transport.legacy.commType == 1) {
            cfg.transport.legacy.portName = legacyPortCombo_->currentData().toString();
            cfg.transport.legacy.baudRate = legacyBaudCombo_->currentData().toInt();
            cfg.transport.legacy.dataBits = 8;
            cfg.transport.legacy.parity = QStringLiteral("none");
            cfg.transport.legacy.stopBits = QStringLiteral("1");
        } else {
            cfg.transport.legacy.transferType = legacyTransferCombo_->currentData().toInt();
            cfg.transport.legacy.model = legacyModelCombo_->currentData().toInt();
            cfg.transport.legacy.localAddress = legacyLocalAddrEdit_->text().trimmed();
            cfg.transport.legacy.localPort = static_cast<quint16>(legacyLocalPortSpin_->value());
            cfg.transport.legacy.remoteAddress = legacyRemoteAddrEdit_->text().trimmed();
            cfg.transport.legacy.remotePort = static_cast<quint16>(legacyRemotePortSpin_->value());
        }
        return cfg;
    }

    cfg.mode = ca::WorkMode::Native;
    const int kind = transportCombo_->currentData().toInt();
    if (kind == static_cast<int>(ca::TransportKind::TcpClient)) {
        cfg.transport.kind = ca::TransportKind::TcpClient;
        cfg.transport.tcpClient.remoteAddress = tcpHostEdit_->text().trimmed();
        cfg.transport.tcpClient.remotePort = static_cast<quint16>(tcpPortSpin_->value());
    } else if (kind == static_cast<int>(ca::TransportKind::TcpServer)) {
        cfg.transport.kind = ca::TransportKind::TcpServer;
        cfg.transport.tcpServer.localAddress = tcpServerAddrEdit_->text().trimmed();
        cfg.transport.tcpServer.localPort = static_cast<quint16>(tcpServerPortSpin_->value());
    } else if (kind == static_cast<int>(ca::TransportKind::Udp)) {
        cfg.transport.kind = ca::TransportKind::Udp;
        cfg.transport.udp.localAddress = udpLocalAddrEdit_->text().trimmed();
        cfg.transport.udp.localPort = static_cast<quint16>(udpLocalPortSpin_->value());
        cfg.transport.udp.remoteAddress = udpRemoteAddrEdit_->text().trimmed();
        cfg.transport.udp.remotePort = static_cast<quint16>(udpRemotePortSpin_->value());
    } else {
        cfg.transport.kind = ca::TransportKind::Serial;
        cfg.transport.serial.portName = portCombo_->currentData().toString();
        cfg.transport.serial.baudRate = baudCombo_->currentData().toInt();
        cfg.transport.serial.dataBits = dataBitsCombo_->currentData().toInt();
        cfg.transport.serial.parity = parityCombo_->currentData().toString();
        cfg.transport.serial.stopBits = stopBitsCombo_->currentData().toString();
    }
    return cfg;
}

QByteArray MainWindow::buildPayload(QString* error) const
{
    const QString text = sendEdit_->toPlainText();
    if (hexSendCheck_->isChecked())
        return ca::parseHexPayloadStrict(text, error);
    const QByteArray utf8 = text.toUtf8();
    if (utf8.isEmpty() && session_.activeTransportKind() != ca::TransportKind::Udp && error)
        *error = QStringLiteral("发送内容为空");
    return utf8;
}

ca::Result MainWindow::buildLegacySendRequest(ca::SendRequest* req, QString* error) const
{
    if (!req)
        return ca::Result::fail(QStringLiteral("null"), QStringLiteral("内部错误"));
    const ca::LegacyCapabilityProfile& p = legacySession_.capabilityProfile();
    const QString text = sendEdit_->toPlainText().trimmed();
    if (text.isEmpty()) {
        if (error)
            *error = QStringLiteral("发送内容为空");
        return ca::Result::fail(QStringLiteral("empty"), QStringLiteral("发送内容为空"));
    }

    req->requestId = QUuid::createUuid();
    req->sessionId = legacySession_.sessionId();

    if (p.supports(ca::LegacyCapability::SendEncodedValues)
        && (!p.supports(ca::LegacyCapability::SendTransparentText) || text.contains(QLatin1Char(','))
            || text.contains(QRegularExpression(QStringLiteral("^\\s*-?\\d"))))) {
        QVariantList vals;
        const QStringList parts = text.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            bool ok = false;
            const double d = part.toDouble(&ok);
            if (!ok) {
                if (error)
                    *error = QStringLiteral("数值解析失败：%1").arg(part);
                return ca::Result::fail(QStringLiteral("parse"), QStringLiteral("数值解析失败"));
            }
            vals << d;
        }
        req->attributes.insert(QStringLiteral("legacySend"), QStringLiteral("values"));
        req->attributes.insert(QStringLiteral("values"), vals);
        req->payload = text.toUtf8();
        return ca::Result::success();
    }

    if (!p.supports(ca::LegacyCapability::SendTransparentText)) {
        if (error)
            *error = QStringLiteral("当前协议不支持发送");
        return ca::Result::fail(QStringLiteral("capability_denied"), QStringLiteral("当前协议不支持发送"));
    }
    req->attributes.insert(QStringLiteral("legacySend"), QStringLiteral("text"));
    req->payload = text.toUtf8();
    return ca::Result::success();
}

void MainWindow::onOpenClicked()
{
    ca::ICommSession* sess = activeSession();
    if (sess->state() == ca::SessionState::Connected) {
        capture_.stopSession(sess->sessionId());
        sess->close();
        syncUi();
        return;
    }
    if (!btnOpen_->isEnabled())
        return;

    scheduler_.stopAll();
    activeSchedTaskId_ = QUuid();
    capture_.stopAll();
    session_.close();
    legacySession_.close();

    const ca::SessionConfig cfg = buildConfig();
    const ca::Result cfgResult = sess->configure(cfg);
    if (!cfgResult.ok) {
        appendLog(QStringLiteral("配置失败：%1").arg(cfgResult.message));
        syncUi();
        return;
    }
    if (captureEnableCheck_ && captureEnableCheck_->isChecked()) {
        const ca::Result cap = capture_.startSession(sess->sessionId(), cfg.name);
        if (!cap.ok)
            appendLog(QStringLiteral("抓包启动失败：%1").arg(cap.message));
    }
    const ca::Result openResult = sess->open();
    if (openResult.ok) {
        if (isLegacyMode()) {
            appendLog(QStringLiteral("Legacy 打开已接受（%1）")
                          .arg(cfg.transport.legacy.useMockBackend ? QStringLiteral("Mock")
                                                                  : QStringLiteral("CommHandler")));
        } else {
            const ca::ResourceClaim& claim = session_.claim();
            QString endpoint;
            if (!claim.serialPort.isEmpty()) {
                endpoint = claim.serialPort;
            } else if (session_.activeTransportKind() == ca::TransportKind::TcpServer) {
                const QString addr = claim.localAddress.isEmpty() ? QStringLiteral("0.0.0.0") : claim.localAddress;
                endpoint = QStringLiteral("监听 %1:%2").arg(addr).arg(claim.localPort);
            } else if (session_.activeTransportKind() == ca::TransportKind::Udp) {
                const QString addr = claim.localAddress.isEmpty() ? QStringLiteral("0.0.0.0") : claim.localAddress;
                endpoint = QStringLiteral("绑定 %1:%2").arg(addr).arg(claim.localPort);
                if (!claim.remoteAddress.isEmpty() && claim.remotePort != 0)
                    endpoint += QStringLiteral(" → %1:%2").arg(claim.remoteAddress).arg(claim.remotePort);
            } else {
                endpoint = QStringLiteral("%1:%2").arg(claim.remoteAddress).arg(claim.remotePort);
            }
            appendLog(QStringLiteral("打开成功：%1").arg(endpoint));
            if (session_.activeTransportKind() == ca::TransportKind::TcpServer)
                refreshClientCombo();
        }
    } else {
        capture_.stopSession(sess->sessionId());
        appendLog(QStringLiteral("打开失败：%1").arg(openResult.message));
    }
    syncUi();
}

void MainWindow::onCloseClicked()
{
    capture_.stopSession(activeSession()->sessionId());
    activeSession()->close();
    syncUi();
}

void MainWindow::fillChannelOnRequest(ca::SendRequest* req) const
{
    if (!req || isLegacyMode())
        return;
    if (session_.activeTransportKind() == ca::TransportKind::TcpServer && clientCombo_) {
        req->channelId = clientCombo_->currentData().toString();
        req->broadcast = req->channelId.isEmpty();
    }
}

QVector<QByteArray> MainWindow::buildSchedulePayloads(QString* error) const
{
    QVector<QByteArray> out;
    if (isLegacyMode()) {
        // Legacy 调度：整框作为一条载荷（数值 CSV 或文本）
        const QString text = sendEdit_->toPlainText().trimmed();
        if (text.isEmpty()) {
            if (error)
                *error = QStringLiteral("发送内容为空");
            return out;
        }
        out.push_back(text.toUtf8());
        return out;
    }

    const QString text = sendEdit_->toPlainText();
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    const int mode = schedModeCombo_->currentData().toInt();
    const bool roundRobin = (mode == static_cast<int>(ca::ScheduleMode::RoundRobin));

    if (!roundRobin) {
        QString err;
        const QByteArray one = buildPayload(&err);
        const bool allowEmptyUdp = (session_.activeTransportKind() == ca::TransportKind::Udp);
        if (one.isEmpty() && (!allowEmptyUdp || !err.isEmpty())) {
            if (error)
                *error = err.isEmpty() ? QStringLiteral("发送内容为空") : err;
            return out;
        }
        out.push_back(one);
        return out;
    }

    if (lines.isEmpty()) {
        if (error)
            *error = QStringLiteral("轮询模式需要至少一行载荷");
        return out;
    }
    for (const QString& line : lines) {
        QString err;
        QByteArray bytes;
        if (hexSendCheck_->isChecked())
            bytes = ca::parseHexPayloadStrict(line, &err);
        else
            bytes = line.toUtf8();
        if (!err.isEmpty() || (bytes.isEmpty() && session_.activeTransportKind() != ca::TransportKind::Udp)) {
            if (error)
                *error = err.isEmpty() ? QStringLiteral("轮询行载荷无效") : err;
            return QVector<QByteArray>();
        }
        out.push_back(bytes);
    }
    return out;
}

void MainWindow::onSendClicked()
{
    if (isLegacyMode()) {
        ca::SendRequest req;
        QString err;
        const ca::Result built = buildLegacySendRequest(&req, &err);
        if (!built.ok) {
            appendLog(QStringLiteral("发送失败：%1").arg(err.isEmpty() ? built.message : err));
            return;
        }
        const ca::Result r = legacySession_.send(req);
        if (!r.ok)
            appendLog(QStringLiteral("发送失败：%1").arg(r.message));
        return;
    }

    QString err;
    const QByteArray payload = buildPayload(&err);
    const bool allowEmptyUdp = (session_.activeTransportKind() == ca::TransportKind::Udp);
    if (payload.isEmpty() && (!allowEmptyUdp || !err.isEmpty())) {
        appendLog(QStringLiteral("发送失败：%1").arg(err.isEmpty() ? QStringLiteral("发送内容为空") : err));
        return;
    }
    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.sessionId = session_.sessionId();
    req.payload = payload;
    fillChannelOnRequest(&req);

    const ca::Result r = session_.send(req);
    if (!r.ok)
        appendLog(QStringLiteral("发送失败：%1").arg(r.message));
}

void MainWindow::onSchedStartClicked()
{
    if (activeSession()->state() != ca::SessionState::Connected) {
        appendLog(QStringLiteral("调度失败：会话未连接"));
        return;
    }
    QString err;
    const QVector<QByteArray> payloads = buildSchedulePayloads(&err);
    if (payloads.isEmpty()) {
        appendLog(QStringLiteral("调度失败：%1").arg(err));
        return;
    }

    ca::ScheduleTaskSpec spec;
    spec.mode = static_cast<ca::ScheduleMode>(schedModeCombo_->currentData().toInt());
    spec.payloads = payloads;
    spec.intervalMs = schedIntervalSpin_->value();
    spec.maxCount = schedCountSpin_->value();
    if (!isLegacyMode() && session_.activeTransportKind() == ca::TransportKind::TcpServer && clientCombo_) {
        spec.channelId = clientCombo_->currentData().toString();
        spec.broadcast = spec.channelId.isEmpty();
    }
    if (isLegacyMode()) {
        // SendScheduler 只填 payload；LegacySession::send 需 attributes — 用文本路径或 CSV
        // 在 Legacy 侧：scheduler 发出的 SendRequest 仅有 payload；validate 将按 values/text 推断
        appendLog(QStringLiteral("提示：Legacy 调度按 payload 推断数值 CSV 或文本"));
    }

    scheduler_.stopAll();
    spec.taskId = QUuid::createUuid();
    activeSchedTaskId_ = spec.taskId;
    const ca::Result r = scheduler_.startTask(spec);
    if (!r.ok) {
        activeSchedTaskId_ = QUuid();
        appendLog(QStringLiteral("调度启动失败：%1").arg(r.message));
        syncUi();
        return;
    }
    appendLog(QStringLiteral("调度已启动"));
    syncUi();
}

void MainWindow::onSchedPauseClicked()
{
    if (activeSchedTaskId_.isNull())
        return;
    const ca::Result r = scheduler_.pauseTask(activeSchedTaskId_);
    if (!r.ok)
        appendLog(QStringLiteral("暂停失败：%1").arg(r.message));
    syncUi();
}

void MainWindow::onSchedResumeClicked()
{
    if (activeSchedTaskId_.isNull())
        return;
    const ca::Result r = scheduler_.resumeTask(activeSchedTaskId_);
    if (!r.ok)
        appendLog(QStringLiteral("继续失败：%1").arg(r.message));
    syncUi();
}

void MainWindow::onSchedStopClicked()
{
    scheduler_.stopAll();
    activeSchedTaskId_ = QUuid();
    appendLog(QStringLiteral("调度已停止"));
    syncUi();
}

void MainWindow::onSchedTaskStopped(const QUuid& taskId, const QString& reason)
{
    if (taskId == activeSchedTaskId_)
        activeSchedTaskId_ = QUuid();
    appendLog(QStringLiteral("调度结束：%1").arg(reason));
    syncUi();
}

void MainWindow::onSchedTaskFailed(const QUuid& taskId, const QString& code, const QString& message)
{
    Q_UNUSED(taskId);
    appendLog(QStringLiteral("调度失败：%1（%2）").arg(message, code));
    syncUi();
}

void MainWindow::onClearLogClicked()
{
    log_->clear();
}

void MainWindow::onRecordReceived(const ca::CommRecord& record)
{
    const ca::Result cap = capture_.enqueue(record);
    if (!cap.ok && cap.code == QStringLiteral("capture_queue_full"))
        appendLog(QStringLiteral("抓包队列满：%1").arg(cap.message));

    QString body = record.summary;
    if (!record.bytes.isEmpty()) {
        if (hexDisplayCheck_->isChecked())
            body += QStringLiteral(" | ") + QString::fromLatin1(record.bytes.toHex(' '));
        else
            body += QStringLiteral(" | ") + QString::fromUtf8(record.bytes);
    }
    if (record.kind == ca::RecordKind::LegacyValueEvent) {
        const QVariantList vals = record.attributes.value(QStringLiteral("values")).toList();
        if (!vals.isEmpty()) {
            QStringList parts;
            for (const QVariant& v : vals)
                parts << QString::number(v.toDouble());
            body += QStringLiteral(" | ") + parts.join(QLatin1Char(','));
        }
    }
    appendLog(QStringLiteral("[%1][%2] %3")
                  .arg(ca::recordKindName(record.kind), ca::recordStatusName(record.status), body));

    const bool isData = (record.kind == ca::RecordKind::RawChunk
                         || record.kind == ca::RecordKind::UdpDatagram);
    if (isData && record.direction == ca::Direction::Tx
        && record.status == ca::RecordStatus::Submitted) {
        txBytes_ += static_cast<quint64>(record.bytes.size());
        updateCounters();
    }
    if (isData && record.direction == ca::Direction::Rx) {
        rxBytes_ += static_cast<quint64>(record.bytes.size());
        ++rxChunks_;
        updateCounters();
    }
    if (record.kind == ca::RecordKind::LegacyValueEvent && record.direction == ca::Direction::Rx) {
        ++rxChunks_;
        updateCounters();
    }
    if (record.kind == ca::RecordKind::LegacyValueEvent && record.direction == ca::Direction::Tx
        && record.status == ca::RecordStatus::Submitted) {
        ++txBytes_;
        updateCounters();
    }
}

void MainWindow::onSessionStateChanged(ca::SessionState state)
{
    if (state == ca::SessionState::Closed || state == ca::SessionState::Faulted
        || state == ca::SessionState::Unresponsive) {
        capture_.stopSession(session_.sessionId());
        capture_.stopSession(legacySession_.sessionId());
    }
    syncUi();
}

void MainWindow::onLegacyUnresponsive(const QString& message)
{
    appendLog(QStringLiteral("Legacy 不可响应：%1 — 请重启应用回收 Worker").arg(message));
    syncUi();
}

void MainWindow::onCaptureFailed(const QUuid& sessionId, const QString& code, const QString& message)
{
    Q_UNUSED(sessionId);
    appendLog(QStringLiteral("抓包错误：%1（%2）").arg(message, code));
}

void MainWindow::onCaptureStarted(const QUuid& sessionId, const QString& filePath)
{
    Q_UNUSED(sessionId);
    appendLog(QStringLiteral("抓包已开始：%1").arg(filePath));
}

void MainWindow::onChannelsChanged()
{
    refreshClientCombo();
    syncUi();
}

void MainWindow::onDisconnectClientClicked()
{
    if (!clientCombo_)
        return;

    const QString channelId = clientCombo_->currentData().toString();
    if (channelId.isEmpty()) {
        appendLog(QStringLiteral("断开失败：请选择具体客户端，不可对广播项操作"));
        return;
    }

    const ca::Result r = session_.disconnectTcpChannel(channelId);
    if (r.ok)
        appendLog(QStringLiteral("已断开客户端：%1").arg(channelId));
    else
        appendLog(QStringLiteral("断开失败：%1").arg(r.message));
}
