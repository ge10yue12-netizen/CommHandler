#include "MainWindow.h"

#include "HexUtil.h"
#include "LegacyCapability.h"

#include <QRegularExpression>

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSerialPortInfo>
#include <QSignalBlocker>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// 侧栏表单：标签左对齐、统一标签列宽，字段列左缘对齐
void applySideFormAlign(QFormLayout* form, int labelMinWidth = 56)
{
    if (!form)
        return;
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(8);
    form->setVerticalSpacing(2);
    for (int r = 0; r < form->rowCount(); ++r) {
        QLayoutItem* li = form->itemAt(r, QFormLayout::LabelRole);
        if (!li)
            continue;
        if (QLabel* lab = qobject_cast<QLabel*>(li->widget())) {
            lab->setMinimumWidth(labelMinWidth);
            lab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            lab->setObjectName(QStringLiteral("FieldLabel"));
        }
    }
}

QString appStyleSheet()
{
    // 侧栏密度对齐参考：小字号、矮控件、小行距（SidePanel 作用域优先）
    return QStringLiteral(
        "QMainWindow { background: #F3F6F9; font-family: 'Microsoft YaHei UI', 'Segoe UI', sans-serif; font-size: 12px; }"
        "QScrollArea#SideScroll { background: #EEF2F6; border: none; }"
        "QWidget#SidePanel {"
        "  background: #EEF2F6;"
        "  border-right: 1px solid #D8DEE6;"
        "  font-family: 'Microsoft YaHei UI', 'Segoe UI', sans-serif;"
        "  font-size: 12px;"
        "}"
        "QWidget#MainPanel { background: #F3F6F9; }"
        "QLabel#AppTitle {"
        "  font-size: 13px; font-weight: 600; color: #1F2937; padding: 0 0 2px 0;"
        "}"
        "QLabel#SideSection, QLabel#SideSectionFirst {"
        "  font-size: 12px; font-weight: 600; color: #374151;"
        "  padding: 6px 0 0 0; margin: 0;"
        "  border-top: 1px solid #D8DEE6;"
        "}"
        "QLabel#SideSectionFirst { border-top: none; padding-top: 0; }"
        "QLabel#FieldLabel { font-size: 12px; font-weight: 400; color: #4B5563; padding: 0; }"
        "QWidget#SidePanel QComboBox,"
        "QWidget#SidePanel QLineEdit,"
        "QWidget#SidePanel QSpinBox {"
        "  background: #FFFFFF; border: 1px solid #D1D9E3; border-radius: 3px;"
        "  padding: 1px 4px; min-height: 20px; max-height: 22px; font-size: 12px; color: #111827;"
        "}"
        "QWidget#SidePanel QComboBox:focus,"
        "QWidget#SidePanel QLineEdit:focus,"
        "QWidget#SidePanel QSpinBox:focus { border: 1px solid #3B82F6; }"
        "QWidget#SidePanel QComboBox::drop-down { border: none; width: 16px; }"
        "QWidget#SidePanel QCheckBox {"
        "  color: #374151; font-size: 12px; spacing: 4px; padding: 0; min-height: 18px; max-height: 20px;"
        "}"
        "QWidget#SidePanel QCheckBox::indicator { width: 13px; height: 13px; }"
        "QWidget#SidePanel QPushButton#PrimaryButton {"
        "  background: #3B82F6; color: white; border: none; border-radius: 4px;"
        "  padding: 4px 6px; min-height: 26px; max-height: 28px; font-size: 12px; font-weight: 600;"
        "}"
        "QWidget#SidePanel QPushButton#PrimaryButton:disabled { background: #93C5FD; }"
        "QWidget#SidePanel QPushButton#PrimaryButton:hover:!disabled { background: #2563EB; }"
        "QWidget#SidePanel QPushButton#GhostButton {"
        "  background: transparent; color: #2563EB; border: none; text-align: left;"
        "  padding: 0; min-height: 18px; font-size: 12px;"
        "}"
        "QWidget#SidePanel QPushButton#SecondaryButton {"
        "  background: #FFFFFF; color: #374151; border: 1px solid #D1D9E3; border-radius: 3px;"
        "  padding: 2px 4px; min-height: 22px; max-height: 24px; font-size: 12px;"
        "}"
        "QWidget#SidePanel QLabel#StatusPill {"
        "  background: #E8F0FE; color: #1D4ED8; border-radius: 3px;"
        "  padding: 2px 6px; font-size: 11px; min-height: 18px; max-height: 20px;"
        "}"
        "QWidget#SidePanel QLabel#CapTip {"
        "  color: #4B5563; font-size: 11px; padding: 4px 6px;"
        "  background: #E8EDF3; border: 1px solid #D8DEE6; border-radius: 4px;"
        "}"
        "QComboBox, QLineEdit, QSpinBox, QPlainTextEdit {"
        "  background: #FFFFFF; border: 1px solid #D1D9E3; border-radius: 6px;"
        "  padding: 4px 8px; min-height: 18px; color: #111827;"
        "}"
        "QComboBox:focus, QLineEdit:focus, QSpinBox:focus, QPlainTextEdit:focus { border: 1px solid #3B82F6; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QPushButton#SendButton {"
        "  background: #3B82F6; color: white; border: none; border-radius: 10px;"
        "  min-width: 48px; min-height: 48px; font-size: 16px; font-weight: 700;"
        "}"
        "QPushButton#SendButton:disabled { background: #93C5FD; }"
        "QPlainTextEdit#LogView, QPlainTextEdit#DataView {"
        "  background: #FFFFFF; border: 1px solid #D8DEE6; border-radius: 10px; padding: 6px;"
        "  font-family: Consolas, 'Microsoft YaHei UI';"
        "}"
        "QLabel#PaneTitle { font-size: 12px; font-weight: 600; color: #4B5563; padding: 0 0 2px 2px; }"
        "QTabWidget#SendTabs::pane {"
        "  border: 1px solid #D8DEE6; border-radius: 10px; background: #FFFFFF; top: -1px;"
        "}"
        "QTableWidget#SendList {"
        "  background: #FFFFFF; border: 1px solid #D8DEE6; border-radius: 8px; gridline-color: #E5E7EB;"
        "}"
        "QPlainTextEdit#SendEdit {"
        "  background: #FFFFFF; border: 1px solid #D8DEE6; border-radius: 10px;"
        "}"
        "QWidget#SendWorkbenchPane {"
        "  background: transparent;"
        "}"
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
    box->addItem(QStringLiteral("8 联恒光科"), 8);
}

void fillSerialProtocols(QComboBox* box)
{
    box->clear();
    // 标签对齐 CommLab ProtoGuide / 基线 §14.3（含 V1.2.5 联恒=5）
    box->addItem(QStringLiteral("0 三思"), 0);
    box->addItem(QStringLiteral("1 科新"), 1);
    box->addItem(QStringLiteral("2 时代新材"), 2);
    box->addItem(QStringLiteral("3 IEEE（≥5 值）"), 3);
    box->addItem(QStringLiteral("4 冠腾（≥2 值）"), 4);
    box->addItem(QStringLiteral("5 联恒光科（≥2 值）"), 5);
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
    applyStyle();
    buildUi();

    scheduler_.setSession(&session_);

    connect(&session_, &ca::ICommSession::recordReceived, this, &MainWindow::onRecordReceived);
    connect(&session_, &ca::ICommSession::stateChanged, this, &MainWindow::onSessionStateChanged);
    connect(&session_, &ca::ICommSession::errorOccurred, this, &MainWindow::onCommError);
    connect(&session_, &ca::NativeSession::channelsChanged, this, &MainWindow::onChannelsChanged);

    connect(&legacySession_, &ca::ICommSession::recordReceived, this, &MainWindow::onRecordReceived);
    connect(&legacySession_, &ca::ICommSession::stateChanged, this, &MainWindow::onSessionStateChanged);
    connect(&legacySession_, &ca::ICommSession::errorOccurred, this, &MainWindow::onCommError);
    connect(&legacySession_, &ca::LegacySession::unresponsive, this, &MainWindow::onLegacyUnresponsive);

    connect(&scheduler_, &ca::SendScheduler::taskStopped, this, &MainWindow::onSchedTaskStopped);
    connect(&scheduler_, &ca::SendScheduler::taskFailed, this, &MainWindow::onSchedTaskFailed);
    connect(&capture_, &ca::CaptureManager::captureFailed, this, &MainWindow::onCaptureFailed);
    connect(&capture_, &ca::CaptureManager::captureStarted, this, &MainWindow::onCaptureStarted);

    capture_.setOutputDirectory(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("captures")));

    appendLog(QStringLiteral("系统就绪：原生通道（串口/TCP/UDP）与兼容动态库通道（CommHandler，可选模拟后端）。"),
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

bool MainWindow::preferHexDisplay() const
{
    return hexDisplayCheck_ && hexDisplayCheck_->isChecked();
}

bool MainWindow::preferHexSend() const
{
    return hexSendCheck_ && hexSendCheck_->isChecked();
}

QString MainWindow::currentLegacyProtocolLabel() const
{
    const QString proto =
        (legacyProtocolCombo_ && legacyProtocolCombo_->currentIndex() >= 0)
            ? legacyProtocolCombo_->currentText()
            : QStringLiteral("未知协议");
    const int comm = legacyCommTypeCombo_ ? legacyCommTypeCombo_->currentData().toInt() : 1;
    const QString side = (comm == 1) ? QStringLiteral("串口") : QStringLiteral("网口");
    return QStringLiteral("%1 %2").arg(side, proto);
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

void MainWindow::bindUiWidgets()
{
    workModeCombo_ = ui_.workModeCombo;
    transportCombo_ = ui_.transportCombo;
    paramStack_ = ui_.paramStack;

    portCombo_ = ui_.portCombo;
    baudCombo_ = ui_.baudCombo;
    dataBitsCombo_ = ui_.dataBitsCombo;
    parityCombo_ = ui_.parityCombo;
    stopBitsCombo_ = ui_.stopBitsCombo;

    tcpHostEdit_ = ui_.tcpHostEdit;
    tcpPortSpin_ = ui_.tcpPortSpin;

    tcpServerAddrEdit_ = ui_.tcpServerAddrEdit;
    tcpServerPortSpin_ = ui_.tcpServerPortSpin;

    udpLocalAddrEdit_ = ui_.udpLocalAddrEdit;
    udpLocalPortSpin_ = ui_.udpLocalPortSpin;
    udpRemoteAddrEdit_ = ui_.udpRemoteAddrEdit;
    udpRemotePortSpin_ = ui_.udpRemotePortSpin;

    legacyCommTypeCombo_ = ui_.legacyCommTypeCombo;
    legacyProtocolCombo_ = ui_.legacyProtocolCombo;
    legacyEndpointStack_ = ui_.legacyEndpointStack;
    legacyTransferCombo_ = ui_.legacyTransferCombo;
    legacyModelCombo_ = ui_.legacyModelCombo;
    legacyLocalAddrEdit_ = ui_.legacyLocalAddrEdit;
    legacyLocalPortSpin_ = ui_.legacyLocalPortSpin;
    legacyRemoteAddrEdit_ = ui_.legacyRemoteAddrEdit;
    legacyRemotePortSpin_ = ui_.legacyRemotePortSpin;
    legacyPortCombo_ = ui_.legacyPortCombo;
    legacyBaudCombo_ = ui_.legacyBaudCombo;
    legacyDataBitsCombo_ = ui_.legacyDataBitsCombo;
    legacyParityCombo_ = ui_.legacyParityCombo;
    legacyStopBitsCombo_ = ui_.legacyStopBitsCombo;
    legacyMockCheck_ = ui_.legacyMockCheck;
    legacyCapTipLabel_ = ui_.legacyCapTipLabel;

    clientSectionLabel_ = ui_.clientSectionLabel;
    clientCombo_ = ui_.clientCombo;
    btnDisconnectClient_ = ui_.btnDisconnectClient;

    btnOpen_ = ui_.btnOpen;
    statusLabel_ = ui_.statusLabel;

    hexDisplayCheck_ = ui_.hexDisplayCheck;
    captureEnableCheck_ = ui_.captureEnableCheck;
    btnClear_ = ui_.btnClear;

    sendTabs_ = ui_.sendTabs;
    hexSendCheck_ = ui_.hexSendCheck;
    legacySendModeCombo_ = ui_.legacySendModeCombo;
    sendEdit_ = ui_.sendEdit;
    btnSend_ = ui_.btnSend;

    sendListTable_ = ui_.sendListTable;
    btnListAdd_ = ui_.btnListAdd;
    btnListRemove_ = ui_.btnListRemove;
    btnListImport_ = ui_.btnListImport;
    btnListExport_ = ui_.btnListExport;
    btnListSendSelected_ = ui_.btnListSendSelected;

    schedModeCombo_ = ui_.schedModeCombo;
    schedIntervalSpin_ = ui_.schedIntervalSpin;
    schedCountSpin_ = ui_.schedCountSpin;
    btnSchedStart_ = ui_.btnSchedStart;
    btnSchedPause_ = ui_.btnSchedPause;
    btnSchedResume_ = ui_.btnSchedResume;
    btnSchedStop_ = ui_.btnSchedStop;

    dataView_ = ui_.dataView;
    log_ = ui_.log;

    txCountLabel_ = ui_.txCountLabel;
    rxCountLabel_ = ui_.rxCountLabel;
    btnResetCount_ = ui_.btnResetCount;
}

void MainWindow::populateDynamicUi()
{
    // 分区 / 字段标签样式（objectName 供 QSS，避免与 uic 成员名冲突）
    ui_.secConnect->setObjectName(QStringLiteral("SideSectionFirst"));
    ui_.secReceive->setObjectName(QStringLiteral("SideSection"));
    ui_.secSend->setObjectName(QStringLiteral("SideSection"));
    ui_.secOther->setObjectName(QStringLiteral("SideSection"));
    for (QLabel* lab : {ui_.secWorkMode, ui_.secConn, ui_.clientSectionLabel}) {
        if (lab)
            lab->setObjectName(QStringLiteral("FieldLabel"));
    }

    ui_.sideScroll->setObjectName(QStringLiteral("SideScroll"));
    ui_.sidePanel->setObjectName(QStringLiteral("SidePanel"));
    ui_.appTitle->setObjectName(QStringLiteral("AppTitle"));
    // 样式外壳可选：Designer 手改拆掉 mainPanel/sendWorkbenchPane 时不致 C2039
    if (QWidget* mainPanel = findChild<QWidget*>(QStringLiteral("mainPanel")))
        mainPanel->setObjectName(QStringLiteral("MainPanel"));
    else if (ui_.mainLay && ui_.mainLay->parentWidget())
        ui_.mainLay->parentWidget()->setObjectName(QStringLiteral("MainPanel"));
    ui_.btnOpen->setObjectName(QStringLiteral("PrimaryButton"));
    ui_.statusLabel->setObjectName(QStringLiteral("StatusPill"));
    ui_.btnDisconnectClient->setObjectName(QStringLiteral("SecondaryButton"));
    ui_.btnClear->setObjectName(QStringLiteral("GhostButton"));
    ui_.btnResetCount->setObjectName(QStringLiteral("GhostButton"));
    ui_.dataTitle->setObjectName(QStringLiteral("PaneTitle"));
    ui_.logTitle->setObjectName(QStringLiteral("PaneTitle"));
    ui_.sendWorkbenchTitle->setObjectName(QStringLiteral("PaneTitle"));
    if (QWidget* sendPane = findChild<QWidget*>(QStringLiteral("sendWorkbenchPane")))
        sendPane->setObjectName(QStringLiteral("SendWorkbenchPane"));
    else if (ui_.sendWorkbenchLay && ui_.sendWorkbenchLay->parentWidget())
        ui_.sendWorkbenchLay->parentWidget()->setObjectName(QStringLiteral("SendWorkbenchPane"));
    ui_.dataView->setObjectName(QStringLiteral("DataView"));
    ui_.log->setObjectName(QStringLiteral("LogView"));
    ui_.sendTabs->setObjectName(QStringLiteral("SendTabs"));
    ui_.sendEdit->setObjectName(QStringLiteral("SendEdit"));
    ui_.btnSend->setObjectName(QStringLiteral("SendButton"));
    ui_.sendListTable->setObjectName(QStringLiteral("SendList"));
    ui_.legacyCapTipLabel->setObjectName(QStringLiteral("CapTip"));
    ui_.listHint->setObjectName(QStringLiteral("CapTip"));
    ui_.sendSideHint->setObjectName(QStringLiteral("CapTip"));
    ui_.txCountLabel->setObjectName(QStringLiteral("FooterStat"));
    ui_.rxCountLabel->setObjectName(QStringLiteral("FooterStat"));
    for (QPushButton* btn : {ui_.btnListAdd, ui_.btnListRemove, ui_.btnListImport, ui_.btnListExport,
                             ui_.btnListSendSelected, ui_.btnSchedStart, ui_.btnSchedPause,
                             ui_.btnSchedResume, ui_.btnSchedStop}) {
        if (btn)
            btn->setObjectName(QStringLiteral("SecondaryButton"));
    }

    // objectName 变更后重刷样式
    applyStyle();

    workModeCombo_->clear();
    workModeCombo_->addItem(QStringLiteral("原生通道"), static_cast<int>(ca::WorkMode::Native));
    workModeCombo_->addItem(QStringLiteral("兼容动态库"), static_cast<int>(ca::WorkMode::LegacyDll));

    transportCombo_->clear();
    transportCombo_->addItem(QStringLiteral("串口"), static_cast<int>(ca::TransportKind::Serial));
    transportCombo_->addItem(QStringLiteral("TCP 客户端"), static_cast<int>(ca::TransportKind::TcpClient));
    transportCombo_->addItem(QStringLiteral("TCP 服务端"), static_cast<int>(ca::TransportKind::TcpServer));
    transportCombo_->addItem(QStringLiteral("UDP"), static_cast<int>(ca::TransportKind::Udp));

    portCombo_->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& p : ports) {
        const QString tip = p.description().isEmpty()
                                ? p.portName()
                                : QStringLiteral("%1（%2）").arg(p.portName(), p.description());
        portCombo_->addItem(tip, p.portName());
    }
    if (portCombo_->count() == 0)
        portCombo_->addItem(QStringLiteral("COM3"), QStringLiteral("COM3"));

    baudCombo_->clear();
    for (int b : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600})
        baudCombo_->addItem(QString::number(b), b);
    baudCombo_->setCurrentText(QStringLiteral("115200"));

    dataBitsCombo_->clear();
    dataBitsCombo_->addItem(QStringLiteral("8"), 8);
    dataBitsCombo_->addItem(QStringLiteral("7"), 7);
    dataBitsCombo_->addItem(QStringLiteral("6"), 6);
    dataBitsCombo_->addItem(QStringLiteral("5"), 5);

    parityCombo_->clear();
    parityCombo_->addItem(QStringLiteral("无"), QStringLiteral("none"));
    parityCombo_->addItem(QStringLiteral("偶"), QStringLiteral("even"));
    parityCombo_->addItem(QStringLiteral("奇"), QStringLiteral("odd"));

    stopBitsCombo_->clear();
    stopBitsCombo_->addItem(QStringLiteral("1"), QStringLiteral("1"));
    stopBitsCombo_->addItem(QStringLiteral("1.5"), QStringLiteral("1.5"));
    stopBitsCombo_->addItem(QStringLiteral("2"), QStringLiteral("2"));

    legacyCommTypeCombo_->clear();
    legacyCommTypeCombo_->addItem(QStringLiteral("网口"), 0);
    legacyCommTypeCombo_->addItem(QStringLiteral("串口"), 1);

    legacyProtocolCombo_->clear();
    fillNetworkProtocols(legacyProtocolCombo_);

    legacyTransferCombo_->clear();
    legacyTransferCombo_->addItem(QStringLiteral("TCP"), 0);
    legacyTransferCombo_->addItem(QStringLiteral("UDP"), 1);

    legacyModelCombo_->clear();
    legacyModelCombo_->addItem(QStringLiteral("服务端"), 0);
    legacyModelCombo_->addItem(QStringLiteral("客户端"), 1);
    legacyModelCombo_->setCurrentIndex(1);

    legacyPortCombo_->clear();
    for (int i = 0; i < portCombo_->count(); ++i)
        legacyPortCombo_->addItem(portCombo_->itemText(i), portCombo_->itemData(i));

    legacyBaudCombo_->clear();
    for (int b : {4800, 9600, 19200, 38400, 57600, 115200})
        legacyBaudCombo_->addItem(QString::number(b), b);
    legacyBaudCombo_->setCurrentText(QStringLiteral("115200"));

    // 兼容串口参数与原生侧栏选项对齐，经 ILegacyBackend 映射为 DLL 索引
    legacyDataBitsCombo_->clear();
    legacyDataBitsCombo_->addItem(QStringLiteral("8"), 8);
    legacyDataBitsCombo_->addItem(QStringLiteral("7"), 7);
    legacyDataBitsCombo_->addItem(QStringLiteral("6"), 6);
    legacyDataBitsCombo_->addItem(QStringLiteral("5"), 5);

    legacyParityCombo_->clear();
    legacyParityCombo_->addItem(QStringLiteral("无"), QStringLiteral("none"));
    legacyParityCombo_->addItem(QStringLiteral("偶"), QStringLiteral("even"));
    legacyParityCombo_->addItem(QStringLiteral("奇"), QStringLiteral("odd"));

    legacyStopBitsCombo_->clear();
    legacyStopBitsCombo_->addItem(QStringLiteral("1"), QStringLiteral("1"));
    legacyStopBitsCombo_->addItem(QStringLiteral("1.5"), QStringLiteral("1.5"));
    legacyStopBitsCombo_->addItem(QStringLiteral("2"), QStringLiteral("2"));

#if defined(CA_LINK_COMMHANDLER) && CA_LINK_COMMHANDLER
    legacyMockCheck_->setChecked(false);
#else
    legacyMockCheck_->setChecked(true);
#endif

    clientCombo_->clear();
    clientCombo_->addItem(QStringLiteral("全部（广播）"), QString());

    legacySendModeCombo_->clear();
    legacySendModeCombo_->addItem(QStringLiteral("兼容模式：自动"), FormatAuto);
    legacySendModeCombo_->addItem(QStringLiteral("兼容模式：数值 CSV"), FormatValues);
    legacySendModeCombo_->addItem(QStringLiteral("兼容模式：透明文本"), FormatText);

    ensureSendListHeaders();
    sendListTable_->horizontalHeader()->setStretchLastSection(true);
    sendListTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    schedModeCombo_->clear();
    schedModeCombo_->addItem(QStringLiteral("单次（仅首行）"), static_cast<int>(ca::ScheduleMode::Once));
    schedModeCombo_->addItem(QStringLiteral("列表轮询"), static_cast<int>(ca::ScheduleMode::RoundRobin));
    schedModeCombo_->addItem(QStringLiteral("周期（无限轮询）"), static_cast<int>(ca::ScheduleMode::Infinite));
    schedModeCombo_->addItem(QStringLiteral("指定次数（轮询）"), static_cast<int>(ca::ScheduleMode::Counted));

    // 数据显示/日志吃剩余高度；工作台固定高度，切换页签不挤布局
    ui_.panesSplitter->setStretchFactor(0, 3);
    ui_.panesSplitter->setStretchFactor(1, 2);
    ui_.mainLay->setStretch(0, 1);
    ui_.mainLay->setStretch(1, 0);
    ui_.mainLay->setStretch(2, 0);

    // 侧栏表单统一左对齐（汉字列 / 控件列）
    applySideFormAlign(ui_.formConnect);
    applySideFormAlign(ui_.formSerial);
    applySideFormAlign(ui_.formTcpClient);
    applySideFormAlign(ui_.formTcpServer);
    applySideFormAlign(ui_.formUdp);
    applySideFormAlign(ui_.formLegacy);
    applySideFormAlign(ui_.formLegacyNet);
    applySideFormAlign(ui_.formLegacySerial);

    // 参数区：能力已下移，minHeight 只覆盖协议+端点+Mock
    if (paramStack_) {
        paramStack_->setMinimumHeight(220);
        paramStack_->setMaximumHeight(QWIDGETSIZE_MAX);
        paramStack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }
    if (legacyEndpointStack_) {
        legacyEndpointStack_->setMinimumHeight(168);
        legacyEndpointStack_->setMaximumHeight(168);
        legacyEndpointStack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    // 能力区：独立加高，放在载荷提示下方
    if (ui_.lblLegacyCap)
        ui_.lblLegacyCap->setObjectName(QStringLiteral("SideSection"));
    if (legacyCapTipLabel_) {
        legacyCapTipLabel_->setMinimumHeight(108);
        legacyCapTipLabel_->setMaximumHeight(QWIDGETSIZE_MAX);
        legacyCapTipLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        legacyCapTipLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        legacyCapTipLabel_->setWordWrap(true);
    }
    if (ui_.connMethodStack)
        ui_.connMethodStack->setCurrentIndex(isLegacyMode() ? 1 : 0);

    // 广播/客户端区暂隐藏（逻辑保留，不占侧栏）
    if (clientSectionLabel_)
        clientSectionLabel_->setVisible(false);
    if (clientCombo_)
        clientCombo_->setVisible(false);
    if (btnDisconnectClient_)
        btnDisconnectClient_->setVisible(false);

    // 能力区 objectName 变更后重刷侧栏样式
    applyStyle();
}

void MainWindow::buildUi()
{
    ui_.setupUi(this);
    bindUiWidgets();
    populateDynamicUi();

    connect(workModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onWorkModeChanged);
    connect(transportCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTransportChanged);
    connect(legacyCommTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onLegacyCommTypeChanged);
    connect(legacyProtocolCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onLegacyProtocolChanged);
    connect(btnOpen_, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    // 端点参数变更：已连接时自动按新参数重连
    auto applyEndpoint = [this](const QString& reason) {
        return [this, reason]() { requestApplyConnectedConfig(reason); };
    };
    if (portCombo_)
        connect(portCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyEndpoint(QStringLiteral("串口端口")));
    if (baudCombo_)
        connect(baudCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyEndpoint(QStringLiteral("波特率")));
    if (dataBitsCombo_)
        connect(dataBitsCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyEndpoint(QStringLiteral("数据位")));
    if (parityCombo_)
        connect(parityCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyEndpoint(QStringLiteral("校验")));
    if (stopBitsCombo_)
        connect(stopBitsCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, applyEndpoint(QStringLiteral("停止位")));
    if (tcpHostEdit_)
        connect(tcpHostEdit_, &QLineEdit::editingFinished, this, applyEndpoint(QStringLiteral("TCP 主机")));
    if (tcpPortSpin_)
        connect(tcpPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, applyEndpoint(QStringLiteral("TCP 端口")));
    if (tcpServerPortSpin_)
        connect(tcpServerPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
                applyEndpoint(QStringLiteral("TCP 服务端口")));
    if (udpLocalPortSpin_)
        connect(udpLocalPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
                applyEndpoint(QStringLiteral("UDP 本地端口")));
    if (udpRemoteAddrEdit_)
        connect(udpRemoteAddrEdit_, &QLineEdit::editingFinished, this, applyEndpoint(QStringLiteral("UDP 远端地址")));
    if (udpRemotePortSpin_)
        connect(udpRemotePortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
                applyEndpoint(QStringLiteral("UDP 远端端口")));
    if (legacyRemoteAddrEdit_)
        connect(legacyRemoteAddrEdit_, &QLineEdit::editingFinished, this, applyEndpoint(QStringLiteral("兼容远端地址")));
    if (legacyRemotePortSpin_)
        connect(legacyRemotePortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
                applyEndpoint(QStringLiteral("兼容远端端口")));
    if (legacyLocalAddrEdit_)
        connect(legacyLocalAddrEdit_, &QLineEdit::editingFinished, this, applyEndpoint(QStringLiteral("兼容本地地址")));
    if (legacyLocalPortSpin_)
        connect(legacyLocalPortSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
                applyEndpoint(QStringLiteral("兼容本地端口")));
    if (legacyPortCombo_)
        connect(legacyPortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容串口")));
    if (legacyBaudCombo_)
        connect(legacyBaudCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容波特率")));
    if (legacyDataBitsCombo_)
        connect(legacyDataBitsCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容数据位")));
    if (legacyParityCombo_)
        connect(legacyParityCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容校验")));
    if (legacyStopBitsCombo_)
        connect(legacyStopBitsCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容停止位")));
    if (legacyModelCombo_)
        connect(legacyModelCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                applyEndpoint(QStringLiteral("兼容传输角色")));
    connect(btnSend_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(btnClear_, &QPushButton::clicked, this, &MainWindow::onClearLogClicked);
    connect(btnDisconnectClient_, &QPushButton::clicked, this, &MainWindow::onDisconnectClientClicked);
    connect(btnSchedStart_, &QPushButton::clicked, this, &MainWindow::onSchedStartClicked);
    connect(btnSchedPause_, &QPushButton::clicked, this, &MainWindow::onSchedPauseClicked);
    connect(btnSchedResume_, &QPushButton::clicked, this, &MainWindow::onSchedResumeClicked);
    connect(btnSchedStop_, &QPushButton::clicked, this, &MainWindow::onSchedStopClicked);
    connect(btnListAdd_, &QPushButton::clicked, this, &MainWindow::onSendListAddRow);
    connect(btnListRemove_, &QPushButton::clicked, this, &MainWindow::onSendListRemoveRows);
    connect(btnListImport_, &QPushButton::clicked, this, &MainWindow::onSendListImport);
    connect(btnListExport_, &QPushButton::clicked, this, &MainWindow::onSendListExport);
    connect(btnListSendSelected_, &QPushButton::clicked, this, &MainWindow::onSendListSendSelected);
    connect(schedModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        updateSendPlaceholders();
        syncUi();
    });
    connect(legacySendModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) {
                // 改下拉即取消十六进制勾选，保证单一生效源
                if (hexSendCheck_ && hexSendCheck_->isChecked()) {
                    const QSignalBlocker b(hexSendCheck_);
                    hexSendCheck_->setChecked(false);
                }
                updateSendFormatMutex();
                updateSendPlaceholders();
            });
    connect(hexSendCheck_, &QCheckBox::toggled, this, [this](bool on) {
        // 仅原生：明文不知 HEX 时自动嵌入文本⇄HEX，不改动 Session/Transport
        if (!isLegacyMode())
            syncNativeSendEditForHexMode(on);
        updateSendFormatMutex();
        updateSendPlaceholders();
    });
    connect(hexDisplayCheck_, &QCheckBox::toggled, this, [this](bool) { syncUi(); });
    connect(clientCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { syncUi(); });
    connect(btnResetCount_, &QPushButton::clicked, this, [this]() {
        txBytes_ = 0;
        rxBytes_ = 0;
        rxChunks_ = 0;
        lastRxMs_ = 0;
        updateCounters();
    });
    addSendListRow(true, QStringLiteral("示例-力温"), QStringLiteral("12.3,45.6"), 1000, FormatValues);
    updateSendPlaceholders();
}

void MainWindow::onWorkModeChanged(int)
{
    if (isLegacyMode()) {
        paramStack_->setCurrentIndex(4);
        // 连接方式行切换到 Legacy：网口/串口（与下方参数页同源，无重复「通信类型」）
        if (ui_.connMethodStack)
            ui_.connMethodStack->setCurrentIndex(1);
        scheduler_.setSession(&legacySession_);
        // Legacy 默认不勾十六进制发送，避免「自动」被误判为原始 HEX
        if (hexSendCheck_)
            hexSendCheck_->setChecked(false);
    } else {
        paramStack_->setCurrentIndex(transportCombo_->currentIndex());
        if (ui_.connMethodStack)
            ui_.connMethodStack->setCurrentIndex(0);
        scheduler_.setSession(&session_);
    }
    refreshLegacyCapabilityTips();
    updateSendPlaceholders();
    syncUi();
    requestApplyConnectedConfig(QStringLiteral("工作模式"));
}

void MainWindow::onTransportChanged(int index)
{
    if (!isLegacyMode())
        paramStack_->setCurrentIndex(index);
    syncUi();
    requestApplyConnectedConfig(QStringLiteral("连接方式"));
}

void MainWindow::onLegacyCommTypeChanged()
{
    const int comm = legacyCommTypeCombo_->currentData().toInt();
    legacyEndpointStack_->setCurrentIndex(comm == 1 ? 1 : 0);
    const int prevProto = legacyProtocolCombo_->currentData().toInt();
    {
        // clear/addItem 会触发 currentIndexChanged；阻断后避免重入栈溢出
        const QSignalBlocker blocker(legacyProtocolCombo_);
        if (comm == 1)
            fillSerialProtocols(legacyProtocolCombo_);
        else
            fillNetworkProtocols(legacyProtocolCombo_);
        const int idx = legacyProtocolCombo_->findData(prevProto);
        if (idx >= 0)
            legacyProtocolCombo_->setCurrentIndex(idx);
    }
    refreshLegacyCapabilityTips();
    updateSendPlaceholders();
    syncUi();
    requestApplyConnectedConfig(QStringLiteral("网口/串口"));
}

void MainWindow::onLegacyProtocolChanged()
{
    refreshLegacyCapabilityTips();
    updateSendPlaceholders();
    syncUi();
    requestApplyConnectedConfig(QStringLiteral("协议"));
}

void MainWindow::refreshLegacyCapabilityTips()
{
    if (!legacyCapTipLabel_ || !legacyCommTypeCombo_ || !legacyProtocolCombo_)
        return;
    const bool legacy = isLegacyMode();
    if (ui_.lblLegacyCap)
        ui_.lblLegacyCap->setVisible(legacy);
    legacyCapTipLabel_->setVisible(legacy);
    if (!legacy) {
        legacyCapTipLabel_->clear();
        return;
    }

    const ca::LegacyCommKind kind =
        (legacyCommTypeCombo_->currentData().toInt() == 1) ? ca::LegacyCommKind::Serial : ca::LegacyCommKind::Network;
    const ca::LegacyCapabilityProfile profile =
        ca::legacyCapabilityFor(kind, legacyProtocolCombo_->currentData().toInt());
    QStringList lines;
    lines << QStringLiteral("%1  %2  %3  %4")
                 .arg(profile.supports(ca::LegacyCapability::ReceiveValues) ? QStringLiteral("收数值：支持")
                                                                           : QStringLiteral("收数值：不支持"),
                      profile.supports(ca::LegacyCapability::SendEncodedValues) ? QStringLiteral("发数值：支持")
                                                                               : QStringLiteral("发数值：不支持"),
                      profile.supports(ca::LegacyCapability::SendTransparentText) ? QStringLiteral("发文本：支持")
                                                                                 : QStringLiteral("发文本：不支持"),
                      profile.supports(ca::LegacyCapability::ReceiveControlEvents) ? QStringLiteral("控制事件：支持")
                                                                                  : QStringLiteral("控制事件：不支持"));
    const QString lim = profile.entries.value(static_cast<int>(ca::LegacyCapability::SendEncodedValues)).limitation;
    if (!lim.isEmpty())
        lines << lim;
    if (!profile.supports(ca::LegacyCapability::SendEncodedValues)
        && !profile.supports(ca::LegacyCapability::SendTransparentText))
        lines << QStringLiteral("当前协议无可用发送路径；发起发送将被拒绝并记入运行日志");
    lines << QStringLiteral("不支持原始十六进制发送；线帧级核对请使用原生通道");
    const int proto = legacyProtocolCombo_->currentData().toInt();
    const bool binaryWire =
        (kind == ca::LegacyCommKind::Network && (proto == 2 || proto == 8))
        || (kind == ca::LegacyCommKind::Serial && proto == 5);
    if (binaryWire)
        lines << QStringLiteral("线帧为二进制；对端应以十六进制方式核对，勿以文本乱码判定失败");
    if (kind == ca::LegacyCommKind::Network && proto == 3)
        lines << QStringLiteral("网口三思：动态库未实现数值发送分支，不会向链路写出数据");
    if (kind == ca::LegacyCommKind::Serial && proto == 0)
        lines << QStringLiteral("串口三思：定宽 ASCII 数值段，以 0x0D 结束；对端文本模式通常可读");
    if (kind == ca::LegacyCommKind::Serial && proto == 1)
        lines << QStringLiteral("科新：发/收均为单路力值；控制字 HEX 7B51…5D7D（{QLI[1/2]}）");
    if (kind == ca::LegacyCommKind::Serial && proto == 2)
        lines << QStringLiteral("时代新材：对端发 value,num,TYPE,flag\\r\\n；助手仅显示第 1 段数值");
    if (kind == ca::LegacyCommKind::Network && proto == 7)
        lines << QStringLiteral("纳百川线条：calcStart/calcEnd 文本与等价 HEX 字节均应触发控制事件");
    // 展示 ReceiveValues / SendEncodedValues 的 limitation 说明
    const QString rxLim = profile.entries.value(static_cast<int>(ca::LegacyCapability::ReceiveValues)).limitation;
    if (!rxLim.isEmpty())
        lines << rxLim;
    legacyCapTipLabel_->setText(lines.join(QChar('\n')));
}

void MainWindow::updateSendPlaceholders()
{
    if (!sendEdit_)
        return;
    if (!isLegacyMode()) {
        sendEdit_->setPlaceholderText(
            preferHexSend()
                ? QStringLiteral("原生通道 · 十六进制发送：可直接写 3D 0D；勾选时若是明文会自动转为 HEX")
                : QStringLiteral("原生通道 · 原样文本发送；勾选「十六进制发送」可将明文自动转为 HEX"));
        return;
    }
    if (preferHexSend()) {
        sendEdit_->setPlaceholderText(
            QStringLiteral("兼容动态库不支持原始十六进制发送。请取消「十六进制发送」后使用数值 CSV 或透明文本"));
        return;
    }
    const int mode = schedModeCombo_ ? schedModeCombo_->currentData().toInt() : 0;
    const bool list = (mode == static_cast<int>(ca::ScheduleMode::RoundRobin));
    if (list) {
        sendEdit_->setPlaceholderText(
            QStringLiteral("兼容动态库 · 列表轮询：每行一组入参。数值示例：12.3,45.6\n"
                           "支持透明文本的协议可发送普通字符串。原始十六进制请改用原生通道"));
    } else {
        sendEdit_->setPlaceholderText(
            QStringLiteral("兼容动态库 · 数值 CSV 示例：12.3,45.6；或透明文本（视协议能力）\n"
                           "多组发送请使用「列表轮询」。原始十六进制请改用原生通道"));
    }
}

void MainWindow::appendLog(const QString& line, const QString& cssClass)
{
    Q_UNUSED(cssClass);
    if (!log_)
        return;
    const QString stamped = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line;
    log_->appendPlainText(stamped);
}

void MainWindow::appendData(const QString& line)
{
    if (!dataView_)
        return;
    const QString stamped = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line;
    dataView_->appendPlainText(stamped);
}

QString MainWindow::sessionStateTip(ca::SessionState state)
{
    switch (state) {
    case ca::SessionState::Created: return QStringLiteral("会话已创建");
    case ca::SessionState::Opening: return QStringLiteral("正在连接…");
    case ca::SessionState::Connected: return QStringLiteral("已连接");
    case ca::SessionState::Closing: return QStringLiteral("正在关闭…");
    case ca::SessionState::Closed: return QStringLiteral("连接已关闭");
    case ca::SessionState::Reconnecting: return QStringLiteral("正在重连…");
    case ca::SessionState::Unresponsive: return QStringLiteral("会话无响应（禁止新操作，请重启应用程序）");
    case ca::SessionState::Faulted: return QStringLiteral("连接故障（数据/链路可能已中断）");
    }
    return QStringLiteral("未知状态");
}

void MainWindow::updateCounters()
{
    txCountLabel_->setText(QStringLiteral("发送：%1").arg(txBytes_));
    QString rxTail = QStringLiteral("末次接收：无");
    if (lastRxMs_ > 0) {
        const qint64 ago = QDateTime::currentMSecsSinceEpoch() - lastRxMs_;
        if (ago < 1000)
            rxTail = QStringLiteral("末次接收：<1s");
        else
            rxTail = QStringLiteral("末次接收：%1s 前").arg(ago / 1000);
    }
    rxCountLabel_->setText(
        QStringLiteral("接收：%1 - %2 | %3").arg(rxBytes_).arg(rxChunks_).arg(rxTail));
}

void MainWindow::updateSendFormatMutex()
{
    const bool hexOn = preferHexSend();
    const bool legacy = isLegacyMode();
    const ca::SessionState st = activeSession()->state();
    const bool uiAlive = (st != ca::SessionState::Unresponsive);
    if (legacySendModeCombo_) {
        // 十六进制勾选为最高权限：禁下拉，避免与 Legacy 模式并存生效
        legacySendModeCombo_->setEnabled(uiAlive && legacy && !hexOn);
        legacySendModeCombo_->setVisible(legacy);
    }
    if (hexSendCheck_) {
        hexSendCheck_->setEnabled(uiAlive);
        hexSendCheck_->setVisible(true);
    }
}

void MainWindow::syncNativeSendEditForHexMode(bool hexOn)
{
    if (!sendEdit_ || isLegacyMode())
        return;
    const QString text = sendEdit_->toPlainText();
    if (text.isEmpty())
        return;

    if (hexOn) {
        // 已是合法 HEX：视为用户直接写线帧，不二次编码（避免 "3D 0D"→"33 44…"）
        QString hexErr;
        const QByteArray asHex = ca::parseHexPayloadStrict(text, &hexErr);
        if (!asHex.isEmpty() && hexErr.isEmpty())
            return;
        // 明文 → 空格分隔 HEX，发出字节与「取消勾选后原文发送」一致
        const QByteArray raw = text.toUtf8();
        sendEdit_->setPlainText(QString::fromLatin1(raw.toHex(' ')).toUpper());
        appendLog(QStringLiteral("[配置] 已将单条内容转为十六进制（按 UTF-8 编码）；发出字节与原文发送相同"));
        return;
    }

    // 取消勾选：若当前是合法 HEX，还原为对应文本，便于继续编辑
    QString hexErr;
    const QByteArray bytes = ca::parseHexPayloadStrict(text, &hexErr);
    if (bytes.isEmpty() || !hexErr.isEmpty())
        return;
    sendEdit_->setPlainText(QString::fromUtf8(bytes));
    appendLog(QStringLiteral("[配置] 已将十六进制还原为文本；可继续原文编辑发送"));
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
    // 已连接仍允许改工作模式/连接方式/端点参数，变更后自动按新配置重连
    const bool canConfig = canOpen || connected;
    const bool legacy = isLegacyMode();

    const int kind = transportCombo_->currentData().toInt();
    const bool isTcpServer = !legacy && (kind == static_cast<int>(ca::TransportKind::TcpServer));
    const bool showClients = isTcpServer && connected;

    if (workModeCombo_)
        workModeCombo_->setEnabled(canConfig);
    // Native：连接方式=传输类型；Legacy：连接方式=网口/串口（connMethodStack 已切换页）
    if (transportCombo_)
        transportCombo_->setEnabled(canConfig && !legacy);
    if (legacyCommTypeCombo_)
        legacyCommTypeCombo_->setEnabled(canConfig && legacy);
    paramStack_->setEnabled(canConfig);

    // 收发相关选择区不按协议/连接锁死；不可用时在点击后写日志提示
    const bool uiAlive = (st != ca::SessionState::Unresponsive);
    if (btnSend_)
        btnSend_->setEnabled(uiAlive);
    if (hexSendCheck_) {
        hexSendCheck_->setEnabled(uiAlive);
        hexSendCheck_->setVisible(true);
    }
    if (hexDisplayCheck_)
        hexDisplayCheck_->setEnabled(uiAlive);
    updateSendFormatMutex();
    if (ui_.sendSideHint)
        ui_.sendSideHint->setVisible(true);

    const bool schedActive = !activeSchedTaskId_.isNull() && scheduler_.hasTask(activeSchedTaskId_);
    const bool schedPaused = schedActive && scheduler_.isPaused(activeSchedTaskId_);
    if (schedCountSpin_)
        schedCountSpin_->setEnabled(uiAlive);
    if (schedIntervalSpin_)
        schedIntervalSpin_->setEnabled(uiAlive);
    if (schedModeCombo_)
        schedModeCombo_->setEnabled(uiAlive);
    if (btnSchedStart_)
        btnSchedStart_->setEnabled(uiAlive);
    if (btnSchedPause_)
        btnSchedPause_->setEnabled(uiAlive && schedActive && !schedPaused);
    if (btnSchedResume_)
        btnSchedResume_->setEnabled(uiAlive && schedActive && schedPaused);
    if (btnSchedStop_)
        btnSchedStop_->setEnabled(uiAlive && schedActive);
    if (btnListAdd_)
        btnListAdd_->setEnabled(uiAlive);
    if (btnListRemove_)
        btnListRemove_->setEnabled(uiAlive);
    if (btnListImport_)
        btnListImport_->setEnabled(uiAlive);
    if (btnListExport_)
        btnListExport_->setEnabled(uiAlive);
    if (btnListSendSelected_)
        btnListSendSelected_->setEnabled(uiAlive);
    if (sendListTable_)
        sendListTable_->setEnabled(uiAlive);

    // 客户端/广播区暂不展示（保留控件与槽，后续可恢复）
    if (clientSectionLabel_)
        clientSectionLabel_->setVisible(false);
    if (clientCombo_)
        clientCombo_->setVisible(false);
    if (btnDisconnectClient_) {
        btnDisconnectClient_->setVisible(false);
        btnDisconnectClient_->setEnabled(false);
    }

    if (st == ca::SessionState::Unresponsive) {
        btnOpen_->setText(QStringLiteral("无响应（请重启）"));
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
    case ca::SessionState::Connected: {
        // 按通讯方式区分就绪语义：协议索引不参与连通判定，只影响收发解析
        if (legacy) {
            const int comm = legacyCommTypeCombo_ ? legacyCommTypeCombo_->currentData().toInt() : 1;
            if (comm == 1) {
                stateText = QStringLiteral("已连接"); // 串口：端口已打开
            } else {
                const int transfer = legacyTransferCombo_ ? legacyTransferCombo_->currentData().toInt() : 0;
                const int model = legacyModelCombo_ ? legacyModelCombo_->currentData().toInt() : 1;
                if (transfer == 1)
                    stateText = QStringLiteral("已绑定"); // UDP
                else if (model == 0)
                    stateText = QStringLiteral("已监听（等待客户端）"); // TCP 服务端
                else
                    stateText = QStringLiteral("已连接"); // TCP 客户端握手成功
            }
        } else {
            const int kind = transportCombo_->currentData().toInt();
            if (kind == static_cast<int>(ca::TransportKind::TcpServer))
                stateText = QStringLiteral("已监听（等待客户端）");
            else if (kind == static_cast<int>(ca::TransportKind::Udp))
                stateText = QStringLiteral("已绑定");
            else
                stateText = QStringLiteral("已连接"); // 串口已打开 / TCP 客户端已握手
        }
        break;
    }
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
            cfg.transport.legacy.dataBits = legacyDataBitsCombo_->currentData().toInt();
            cfg.transport.legacy.parity = legacyParityCombo_->currentData().toString();
            cfg.transport.legacy.stopBits = legacyStopBitsCombo_->currentData().toString();
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
    if (preferHexSend()) {
        QString hexErr;
        const QByteArray asHex = ca::parseHexPayloadStrict(text, &hexErr);
        if (!asHex.isEmpty() && hexErr.isEmpty())
            return asHex;
        // 含非法 HEX 字符：按 UTF-8 明文发出（与取消勾选线帧一致），避免用户不知 HEX 时被拒
        // 偶数长度/纯 HEX 笔误（如奇数位）仍拒绝，以免误发
        if (hexErr.contains(QStringLiteral("非法十六进制字符"))) {
            const QByteArray utf8 = text.toUtf8();
            if (utf8.isEmpty() && session_.activeTransportKind() != ca::TransportKind::Udp && error)
                *error = QStringLiteral("发送内容为空");
            return utf8;
        }
        if (error)
            *error = hexErr.isEmpty() ? QStringLiteral("HEX 解析失败") : hexErr;
        return QByteArray();
    }
    const QByteArray utf8 = text.toUtf8();
    if (utf8.isEmpty() && session_.activeTransportKind() != ca::TransportKind::Udp && error)
        *error = QStringLiteral("发送内容为空");
    return utf8;
}

ca::Result MainWindow::buildLegacySendRequest(ca::SendRequest* req, QString* error) const
{
    const QString text = sendEdit_ ? sendEdit_->toPlainText().trimmed() : QString();
    const int format = legacySendModeCombo_ ? legacySendModeCombo_->currentData().toInt() : FormatAuto;
    return buildLegacySendRequestFrom(text, format, req, error);
}

ca::Result MainWindow::buildLegacySendRequestFrom(const QString& textIn, int format, ca::SendRequest* req,
                                                  QString* error) const
{
    if (!req)
        return ca::Result::fail(QStringLiteral("null"), QStringLiteral("内部错误"));
    const ca::LegacyCapabilityProfile& p = legacySession_.capabilityProfile();
    const QString protoLabel = currentLegacyProtocolLabel();
    const QString text = textIn.trimmed();
    if (text.isEmpty()) {
        if (error)
            *error = QStringLiteral("发送内容为空");
        return ca::Result::fail(QStringLiteral("empty"), QStringLiteral("发送内容为空"));
    }

    // 十六进制勾选优先于列表/下拉格式
    if (preferHexSend() || format == FormatHex) {
        if (error) {
            *error = QStringLiteral(
                         "协议「%1」不支持原始十六进制发送（兼容动态库无 RawSend）。"
                         "请改用数值 CSV 或透明文本；线帧字节核对请切换至原生通道")
                         .arg(protoLabel);
        }
        return ca::Result::fail(QStringLiteral("legacy_no_raw"), QStringLiteral("Legacy 不支持原始 HEX"));
    }

    const bool canValues = p.supports(ca::LegacyCapability::SendEncodedValues);
    const bool canText = p.supports(ca::LegacyCapability::SendTransparentText);
    if (!canValues && !canText) {
        if (error) {
            *error = QStringLiteral("协议「%1」无可用发送路径（发数值与发文本均不支持），无法发送")
                         .arg(protoLabel);
        }
        return ca::Result::fail(QStringLiteral("capability_denied"), QStringLiteral("当前协议不支持发送"));
    }

    const bool forceValues = (format == FormatValues);
    const bool forceText = (format == FormatText);
    const bool preferValues = (format == FormatAuto && canValues) || forceValues;

    req->requestId = QUuid::createUuid();
    req->sessionId = legacySession_.sessionId();

    auto parseValues = [&](QVariantList* vals, QString* badToken) -> bool {
        vals->clear();
        const QStringList parts = text.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
        if (parts.isEmpty())
            return false;
        for (const QString& part : parts) {
            bool ok = false;
            const double d = part.toDouble(&ok);
            if (!ok) {
                if (badToken)
                    *badToken = part;
                return false;
            }
            *vals << d;
        }
        return true;
    };

    if (forceText) {
        if (!canText) {
            if (error) {
                const QString lim =
                    p.entries.value(static_cast<int>(ca::LegacyCapability::SendTransparentText)).limitation;
                *error = lim.isEmpty()
                             ? QStringLiteral("协议「%1」不支持透明文本发送（请改用数值 CSV 或换协议）")
                                   .arg(protoLabel)
                             : QStringLiteral("协议「%1」不支持透明文本：%2").arg(protoLabel, lim);
            }
            return ca::Result::fail(QStringLiteral("capability_denied"), QStringLiteral("不支持文本发送"));
        }
        req->attributes.insert(QStringLiteral("legacySend"), QStringLiteral("text"));
        req->attributes.insert(QStringLiteral("protocolLabel"), protoLabel);
        req->payload = text.toUtf8();
        return ca::Result::success();
    }

    if (preferValues) {
        QVariantList vals;
        QString bad;
        if (parseValues(&vals, &bad)) {
            const QString lim =
                p.entries.value(static_cast<int>(ca::LegacyCapability::SendEncodedValues)).limitation;
            if (lim.contains(QStringLiteral("5")) && vals.size() < 5) {
                if (error)
                    *error = QStringLiteral("协议「%1」数值发送至少需要 5 个数值，当前 %2 个")
                                 .arg(protoLabel)
                                 .arg(vals.size());
                return ca::Result::fail(QStringLiteral("invalid_value_count"), QStringLiteral("数值个数不足"));
            }
            if (lim.contains(QStringLiteral("2")) && vals.size() < 2) {
                if (error)
                    *error = QStringLiteral("协议「%1」数值发送至少需要 2 个数值（如力,温），当前 %2 个")
                                 .arg(protoLabel)
                                 .arg(vals.size());
                return ca::Result::fail(QStringLiteral("invalid_value_count"), QStringLiteral("数值个数不足"));
            }
            if (!canValues) {
                if (error)
                    *error = QStringLiteral("协议「%1」不支持数值发送").arg(protoLabel);
                return ca::Result::fail(QStringLiteral("capability_denied"), QStringLiteral("不支持数值发送"));
            }
            req->attributes.insert(QStringLiteral("legacySend"), QStringLiteral("values"));
            req->attributes.insert(QStringLiteral("values"), vals);
            req->attributes.insert(QStringLiteral("protocolLabel"), protoLabel);
            // payload 仅作入参回显（CSV），非 DLL 线帧
            req->payload = text.toUtf8();
            return ca::Result::success();
        }
        if (forceValues) {
            if (error)
                *error = QStringLiteral("协议「%1」数值解析失败：%2（请用逗号分隔的十进制数）")
                             .arg(protoLabel, bad);
            return ca::Result::fail(QStringLiteral("parse"), QStringLiteral("数值解析失败"));
        }
    }

    if (canText) {
        req->attributes.insert(QStringLiteral("legacySend"), QStringLiteral("text"));
        req->attributes.insert(QStringLiteral("protocolLabel"), protoLabel);
        req->payload = text.toUtf8();
        return ca::Result::success();
    }

    if (error)
        *error = QStringLiteral("协议「%1」无法解析为数值，且不支持文本发送").arg(protoLabel);
    return ca::Result::fail(QStringLiteral("parse"), QStringLiteral("载荷无法按协议发送"));
}

QString MainWindow::formatRecordForDisplay(const ca::CommRecord& record) const
{
    QString body = record.summary;
    const bool legacyValue = (record.kind == ca::RecordKind::LegacyValueEvent);
    const bool legacyTx = legacyValue && (record.direction == ca::Direction::Tx);

    // 收发设置严格分离：
    // - 「十六进制显示」只影响接收方向字节的展示
    // - 「十六进制发送」只影响发送方向字节的展示（与发送解析同源）
    // - 均未勾选 → 原样 UTF-8 文本；不因未解析收包强制 HEX
    if (legacyTx && !record.bytes.isEmpty()) {
        // Legacy TX：bytes 为业务入参，非 DLL 线帧；展示不受「十六进制显示」影响
        body += QStringLiteral(" | 入参 ") + QString::fromUtf8(record.bytes);
        body += QStringLiteral(" | 线帧由动态库内部编码（接口不回传已发送原始字节；二进制协议请以十六进制核对对端）");
    } else if (!record.bytes.isEmpty()) {
        const bool asHex =
            (record.direction == ca::Direction::Rx)   ? preferHexDisplay()
            : (record.direction == ca::Direction::Tx) ? preferHexSend()
                                                      : false;
        if (asHex)
            body += QStringLiteral(" | 线帧十六进制 ") + QString::fromLatin1(record.bytes.toHex(' '));
        else
            body += QStringLiteral(" | 线帧文本 ") + QString::fromUtf8(record.bytes);
    }
    if (legacyValue) {
        const QVariantList vals = record.attributes.value(QStringLiteral("values")).toList();
        if (!vals.isEmpty()) {
            QStringList parts;
            for (const QVariant& v : vals)
                parts << QString::number(v.toDouble(), 'g', 12);
            body += QStringLiteral(" | 数值 ") + parts.join(QLatin1Char(','));
        }
        const QString proto = record.attributes.value(QStringLiteral("protocolLabel")).toString();
        if (!proto.isEmpty())
            body += QStringLiteral(" | 协议 ") + proto;
        if (record.direction == ca::Direction::Rx) {
            const int legacyType = record.attributes.value(QStringLiteral("legacyType")).toInt();
            body += QStringLiteral(" | 类型=%1").arg(legacyType);
        }
    }
    if (record.kind == ca::RecordKind::LegacyControlEvent
        || record.kind == ca::RecordKind::LegacyParameterEvent) {
        const QString msgName = record.attributes.value(QStringLiteral("msgName")).toString();
        const int ctrlCmd = record.attributes.value(QStringLiteral("ctrlCmd")).toInt();
        const int viewId = record.attributes.value(QStringLiteral("viewId")).toInt();
        const int msg = record.attributes.value(QStringLiteral("msg")).toInt();
        if (!msgName.isEmpty())
            body = QStringLiteral("指令[%1] 控制命令=%2 视窗=%3 消息=%4(0x%5)")
                       .arg(msgName)
                       .arg(ctrlCmd)
                       .arg(viewId)
                       .arg(msg)
                       .arg(msg, 0, 16);
        if (record.kind == ca::RecordKind::LegacyParameterEvent) {
            const QVariantMap extra = record.attributes.value(QStringLiteral("extra")).toMap();
            if (!extra.isEmpty())
                body += QStringLiteral(" | 附加参数键数=%1").arg(extra.size());
        }
    }
    return QStringLiteral("[%1][%2][%3] %4")
        .arg(ca::directionDisplayName(record.direction), ca::recordKindDisplayName(record.kind),
             ca::recordStatusDisplayName(record.status), body);
}

void MainWindow::onOpenClicked()
{
    ca::ICommSession* sess = activeSession();
    if (sess->state() == ca::SessionState::Connected) {
        pendingReopenAfterConfig_ = false;
        capture_.stopSession(sess->sessionId());
        sess->close();
        syncUi();
        return;
    }
    if (!btnOpen_->isEnabled())
        return;
    openWithCurrentUiConfig();
}

// 已连接时修改侧栏配置：关闭当前会话，Closed 后按新 UI 自动重开
void MainWindow::requestApplyConnectedConfig(const QString& reason)
{
    ca::ICommSession* sess = activeSession();
    if (!sess || sess->state() != ca::SessionState::Connected)
        return;
    if (pendingReopenAfterConfig_)
        return;
    pendingReopenAfterConfig_ = true;
    appendLog(QStringLiteral("[配置] 已修改「%1」，正在按新参数重连…").arg(reason));
    scheduler_.stopAll();
    activeSchedTaskId_ = QUuid();
    capture_.stopSession(sess->sessionId());
    sess->close();
    syncUi();
}

// 按当前界面配置打开会话（首次打开与热重连共用）
void MainWindow::openWithCurrentUiConfig()
{
    ca::ICommSession* sess = activeSession();
    if (!sess)
        return;

    scheduler_.stopAll();
    activeSchedTaskId_ = QUuid();
    capture_.stopAll();
    session_.close();
    legacySession_.close();

    const ca::SessionConfig cfg = buildConfig();
    const ca::Result cfgResult = sess->configure(cfg);
    if (!cfgResult.ok) {
        pendingReopenAfterConfig_ = false;
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
            appendLog(QStringLiteral("兼容通道：打开请求已接受（%1）")
                          .arg(cfg.transport.legacy.useMockBackend ? QStringLiteral("模拟后端")
                                                                  : QStringLiteral("动态库 CommHandler")));
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
        pendingReopenAfterConfig_ = false;
        capture_.stopSession(sess->sessionId());
        appendLog(QStringLiteral("打开失败：%1").arg(openResult.message));
    }
    syncUi();
}

void MainWindow::onCloseClicked()
{
    pendingReopenAfterConfig_ = false;
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
    QVector<int> formats;
    if (!collectEnabledListPayloads(&out, &formats, error))
        return QVector<QByteArray>();
    return out;
}

void MainWindow::onSendClicked()
{
    if (activeSession()->state() != ca::SessionState::Connected) {
        appendLog(QStringLiteral("[边界] 发送拒绝：会话未连接（请先打开连接）"));
        return;
    }

    if (isLegacyMode()) {
        ca::SendRequest req;
        QString err;
        const ca::Result built = buildLegacySendRequest(&req, &err);
        if (!built.ok) {
            appendLog(QStringLiteral("[边界] 发送拒绝：%1").arg(err.isEmpty() ? built.message : err));
            return;
        }
        const ca::Result r = legacySession_.send(req);
        if (!r.ok)
            appendLog(QStringLiteral("[异常] 发送失败：%1").arg(r.message));
        else
            appendData(QStringLiteral("[TX][Pending] %1").arg(QString::fromUtf8(req.payload)));
        return;
    }

    QString err;
    const QByteArray payload = buildPayload(&err);
    const bool allowEmptyUdp = (session_.activeTransportKind() == ca::TransportKind::Udp);
    if (payload.isEmpty() && (!allowEmptyUdp || !err.isEmpty())) {
        QString tip = err.isEmpty() ? QStringLiteral("发送内容为空") : err;
        if (preferHexSend() && tip.contains(QStringLiteral("十六进制")))
            tip += QStringLiteral("。提示：可取消「十六进制发送」按原文发出，或先勾选一次让明文自动转为 HEX");
        appendLog(QStringLiteral("[边界] 发送拒绝：%1").arg(tip));
        return;
    }
    ca::SendRequest req;
    req.requestId = QUuid::createUuid();
    req.sessionId = session_.sessionId();
    req.payload = payload;
    fillChannelOnRequest(&req);

    const ca::Result r = session_.send(req);
    if (!r.ok)
        appendLog(QStringLiteral("[异常] 发送失败：%1").arg(r.message));
}

void MainWindow::onSchedStartClicked()
{
    if (activeSession()->state() != ca::SessionState::Connected) {
        appendLog(QStringLiteral("[边界] 调度拒绝：会话未连接（请先打开连接）"));
        return;
    }
    if (!activeSchedTaskId_.isNull() && scheduler_.hasTask(activeSchedTaskId_)) {
        appendLog(QStringLiteral("[边界] 调度拒绝：已有调度任务运行中，请先停止"));
        return;
    }

    QVector<QByteArray> payloads;
    QVector<int> formats;
    QVector<int> intervals;
    QVector<QVariantMap> attrs;
    QString err;
    if (!sendListTable_ || sendListTable_->rowCount() == 0) {
        appendLog(QStringLiteral("[边界] 调度拒绝：发送列表为空，请添加行或导入 txt/csv"));
        return;
    }

    for (int r = 0; r < sendListTable_->rowCount(); ++r) {
        QTableWidgetItem* enItem = sendListTable_->item(r, 0);
        if (!enItem || enItem->checkState() != Qt::Checked)
            continue;
        const QString payload = sendListTable_->item(r, 2)
                                    ? sendListTable_->item(r, 2)->text().trimmed()
                                    : QString();
        if (payload.isEmpty()) {
            appendLog(QStringLiteral("[边界] 调度拒绝：第 %1 行载荷为空").arg(r + 1));
            return;
        }
        int intervalMs = schedIntervalSpin_ ? schedIntervalSpin_->value() : 1000;
        if (sendListTable_->item(r, 3)) {
            bool ok = false;
            const int v = sendListTable_->item(r, 3)->text().toInt(&ok);
            if (ok && v >= 0)
                intervalMs = v;
        }
        int format = FormatAuto;
        if (auto* combo = qobject_cast<QComboBox*>(sendListTable_->cellWidget(r, 4)))
            format = combo->currentData().toInt();

        if (isLegacyMode()) {
            ca::SendRequest probe;
            QString e;
            const ca::Result built = buildLegacySendRequestFrom(payload, format, &probe, &e);
            if (!built.ok) {
                appendLog(QStringLiteral("[边界] 调度拒绝：第 %1 行 — %2").arg(r + 1).arg(e.isEmpty() ? built.message : e));
                return;
            }
            payloads.push_back(probe.payload);
            attrs.push_back(probe.attributes);
            intervals.push_back(intervalMs);
            formats.push_back(format);
        } else {
            QByteArray bytes;
            QString e;
            if (format == FormatHex) {
                bytes = ca::parseHexPayloadStrict(payload, &e);
                if (!e.isEmpty()) {
                    appendLog(QStringLiteral("[边界] 调度拒绝：第 %1 行十六进制 — %2").arg(r + 1).arg(e));
                    return;
                }
            } else if (preferHexSend()) {
                bytes = ca::parseHexPayloadStrict(payload, &e);
                if (bytes.isEmpty() && e.contains(QStringLiteral("非法十六进制字符"))) {
                    e.clear();
                    bytes = payload.toUtf8();
                } else if (!e.isEmpty()) {
                    appendLog(QStringLiteral("[边界] 调度拒绝：第 %1 行十六进制 — %2").arg(r + 1).arg(e));
                    return;
                }
            } else {
                bytes = payload.toUtf8();
            }
            payloads.push_back(bytes);
            attrs.push_back(QVariantMap());
            intervals.push_back(intervalMs);
            formats.push_back(format);
        }
    }

    if (payloads.isEmpty()) {
        appendLog(QStringLiteral("[边界] 调度拒绝：没有勾选启用的列表行"));
        return;
    }
    Q_UNUSED(formats);
    Q_UNUSED(err);

    ca::ScheduleTaskSpec spec;
    const ca::ScheduleMode userMode =
        static_cast<ca::ScheduleMode>(schedModeCombo_->currentData().toInt());
    spec.mode = userMode;
    spec.maxCount = schedCountSpin_->value();
    if (payloads.size() > 1) {
        if (userMode == ca::ScheduleMode::Once) {
            appendLog(QStringLiteral("提示：调度模式为「单次」，列表存在多行时仅发送首个已启用行"));
            payloads.resize(1);
            attrs.resize(1);
            intervals.resize(1);
        } else {
            spec.mode = ca::ScheduleMode::RoundRobin;
            if (userMode == ca::ScheduleMode::Infinite)
                spec.maxCount = 0; // RoundRobin 下 0 = 无限
        }
    }

    spec.payloads = payloads;
    spec.payloadAttributes = attrs;
    spec.payloadIntervals = intervals;
    spec.intervalMs = schedIntervalSpin_->value();
    if (!isLegacyMode() && session_.activeTransportKind() == ca::TransportKind::TcpServer && clientCombo_) {
        spec.channelId = clientCombo_->currentData().toString();
        spec.broadcast = spec.channelId.isEmpty();
    }

    appendLog(QStringLiteral("提示：发送列表调度共 %1 组").arg(payloads.size()));
    for (int i = 0; i < payloads.size(); ++i)
        appendData(QStringLiteral("[TX][Sched#%1] %2").arg(i + 1).arg(QString::fromUtf8(payloads.at(i))));

    scheduler_.stopAll();
    spec.taskId = QUuid::createUuid();
    activeSchedTaskId_ = spec.taskId;
    const ca::Result r = scheduler_.startTask(spec);
    if (!r.ok) {
        activeSchedTaskId_ = QUuid();
        appendLog(QStringLiteral("[异常] 调度启动失败：%1").arg(r.message));
        syncUi();
        return;
    }
    appendLog(QStringLiteral("[连接] 调度已启动"));
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
    if (log_)
        log_->clear();
    if (dataView_)
        dataView_->clear();
}

void MainWindow::onRecordReceived(const ca::CommRecord& record)
{
    const ca::Result cap = capture_.enqueue(record);
    if (!cap.ok && cap.code == QStringLiteral("capture_queue_full"))
        appendLog(QStringLiteral("[异常] 抓包队列满：%1").arg(cap.message));

    const QString line = formatRecordForDisplay(record);

    // 指令与测数、收发载荷均进数据区，便于核对；连接/异常仍进日志
    const bool toData = (record.kind == ca::RecordKind::RawChunk
                         || record.kind == ca::RecordKind::UdpDatagram
                         || record.kind == ca::RecordKind::LegacyValueEvent
                         || record.kind == ca::RecordKind::LegacyControlEvent
                         || record.kind == ca::RecordKind::LegacyParameterEvent);
    if (toData)
        appendData(line);

    if (record.kind == ca::RecordKind::ErrorEvent) {
        if (record.errorCode == QStringLiteral("rx_cap")
            || record.errorCode == QStringLiteral("rx_unparsed"))
            appendLog(QStringLiteral("[边界] %1").arg(record.summary));
        else
            appendLog(QStringLiteral("[异常] %1").arg(line));
    } else if (record.kind == ca::RecordKind::ConnectionEvent)
        appendLog(QStringLiteral("[连接] %1").arg(line));
    else if (!toData)
        appendLog(line);

    const bool isRaw = (record.kind == ca::RecordKind::RawChunk
                        || record.kind == ca::RecordKind::UdpDatagram);
    if (isRaw && record.direction == ca::Direction::Tx
        && record.status == ca::RecordStatus::Submitted) {
        txBytes_ += static_cast<quint64>(record.bytes.size());
        updateCounters();
    }
    if (isRaw && record.direction == ca::Direction::Rx) {
        rxBytes_ += static_cast<quint64>(record.bytes.size());
        ++rxChunks_;
        lastRxMs_ = QDateTime::currentMSecsSinceEpoch();
        updateCounters();
    }
    if (record.kind == ca::RecordKind::LegacyValueEvent && record.direction == ca::Direction::Rx) {
        ++rxChunks_;
        lastRxMs_ = QDateTime::currentMSecsSinceEpoch();
        updateCounters();
    }
    if (record.kind == ca::RecordKind::LegacyValueEvent && record.direction == ca::Direction::Tx
        && record.status == ca::RecordStatus::Submitted) {
        ++txBytes_;
        updateCounters();
    }
    if ((record.kind == ca::RecordKind::LegacyControlEvent
         || record.kind == ca::RecordKind::LegacyParameterEvent)
        && record.direction == ca::Direction::Rx) {
        ++rxChunks_;
        lastRxMs_ = QDateTime::currentMSecsSinceEpoch();
        updateCounters();
    }
}

void MainWindow::onSessionStateChanged(ca::SessionState state)
{
    if (state != lastSessionState_) {
        const bool wasUp = (lastSessionState_ == ca::SessionState::Connected
                            || lastSessionState_ == ca::SessionState::Opening);
        if (state == ca::SessionState::Faulted)
            appendLog(QStringLiteral("[断连] %1").arg(sessionStateTip(state)));
        else if (state == ca::SessionState::Closed && wasUp)
            appendLog(QStringLiteral("[断连] %1").arg(sessionStateTip(state)));
        else if (state == ca::SessionState::Unresponsive)
            appendLog(QStringLiteral("[异常] %1").arg(sessionStateTip(state)));
        else if (state == ca::SessionState::Connected)
            appendLog(QStringLiteral("[连接] %1").arg(sessionStateTip(state)));
        else if (state == ca::SessionState::Opening || state == ca::SessionState::Closing)
            appendLog(QStringLiteral("[连接] %1").arg(sessionStateTip(state)));
        lastSessionState_ = state;
    }

    if (state == ca::SessionState::Closed || state == ca::SessionState::Faulted
        || state == ca::SessionState::Unresponsive) {
        capture_.stopSession(session_.sessionId());
        capture_.stopSession(legacySession_.sessionId());
    }

    // 热重连：关闭完成后按当前 UI 重新打开
    if (pendingReopenAfterConfig_ && state == ca::SessionState::Closed) {
        pendingReopenAfterConfig_ = false;
        QTimer::singleShot(0, this, [this]() { openWithCurrentUiConfig(); });
    }

    syncUi();
}

void MainWindow::onCommError(const ca::CommError& error)
{
    appendLog(QStringLiteral("[异常] %1（%2）").arg(error.message, error.code));
}

void MainWindow::onLegacyUnresponsive(const QString& message)
{
    appendLog(QStringLiteral("[异常] 兼容通道无响应：%1 — 请重启应用程序以回收工作线程").arg(message));
    syncUi();
}

void MainWindow::onCaptureFailed(const QUuid& sessionId, const QString& code, const QString& message)
{
    Q_UNUSED(sessionId);
    appendLog(QStringLiteral("[异常] 抓包错误：%1（%2）").arg(message, code));
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
        appendLog(QStringLiteral("[边界] 断开拒绝：请选择具体客户端，不可对广播项操作"));
        return;
    }

    const ca::Result r = session_.disconnectTcpChannel(channelId);
    if (r.ok)
        appendLog(QStringLiteral("已断开客户端：%1").arg(channelId));
    else
        appendLog(QStringLiteral("断开失败：%1").arg(r.message));
}

void MainWindow::ensureSendListHeaders()
{
    if (!sendListTable_)
        return;
    sendListTable_->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("启用") << QStringLiteral("名称") << QStringLiteral("载荷")
                      << QStringLiteral("延时ms") << QStringLiteral("格式"));
}

QString MainWindow::formatName(int format)
{
    switch (format) {
    case FormatValues: return QStringLiteral("数值");
    case FormatText: return QStringLiteral("文本");
    case FormatHex: return QStringLiteral("十六进制");
    default: return QStringLiteral("自动");
    }
}

int MainWindow::formatFromName(const QString& name)
{
    const QString n = name.trimmed();
    if (n.contains(QStringLiteral("数值")) || n.compare(QStringLiteral("values"), Qt::CaseInsensitive) == 0)
        return FormatValues;
    if (n.contains(QStringLiteral("文本")) || n.compare(QStringLiteral("text"), Qt::CaseInsensitive) == 0)
        return FormatText;
    if (n.contains(QStringLiteral("HEX"), Qt::CaseInsensitive)
        || n.contains(QStringLiteral("十六进制"))
        || n.compare(QStringLiteral("hex"), Qt::CaseInsensitive) == 0)
        return FormatHex;
    return FormatAuto;
}

void MainWindow::addSendListRow(bool enabled, const QString& name, const QString& payload, int intervalMs,
                                int format)
{
    if (!sendListTable_)
        return;
    const int row = sendListTable_->rowCount();
    sendListTable_->insertRow(row);

    auto* en = new QTableWidgetItem;
    en->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    en->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
    sendListTable_->setItem(row, 0, en);
    sendListTable_->setItem(row, 1, new QTableWidgetItem(name));
    sendListTable_->setItem(row, 2, new QTableWidgetItem(payload));
    sendListTable_->setItem(row, 3, new QTableWidgetItem(QString::number(intervalMs)));

    auto* fmt = new QComboBox(sendListTable_);
    fmt->addItem(QStringLiteral("自动"), FormatAuto);
    fmt->addItem(QStringLiteral("数值"), FormatValues);
    fmt->addItem(QStringLiteral("文本"), FormatText);
    fmt->addItem(QStringLiteral("十六进制"), FormatHex);
    const int idx = fmt->findData(format);
    fmt->setCurrentIndex(idx >= 0 ? idx : 0);
    sendListTable_->setCellWidget(row, 4, fmt);
}

bool MainWindow::collectEnabledListPayloads(QVector<QByteArray>* outPayloads, QVector<int>* outFormats,
                                           QString* error) const
{
    if (!outPayloads)
        return false;
    outPayloads->clear();
    if (outFormats)
        outFormats->clear();
    if (!sendListTable_ || sendListTable_->rowCount() == 0) {
        if (error)
            *error = QStringLiteral("发送列表为空");
        return false;
    }
    for (int r = 0; r < sendListTable_->rowCount(); ++r) {
        QTableWidgetItem* enItem = sendListTable_->item(r, 0);
        if (!enItem || enItem->checkState() != Qt::Checked)
            continue;
        const QString payload = sendListTable_->item(r, 2)
                                    ? sendListTable_->item(r, 2)->text().trimmed()
                                    : QString();
        if (payload.isEmpty()) {
            if (error)
                *error = QStringLiteral("第 %1 行载荷为空").arg(r + 1);
            return false;
        }
        int format = FormatAuto;
        if (auto* combo = qobject_cast<QComboBox*>(sendListTable_->cellWidget(r, 4)))
            format = combo->currentData().toInt();
        outPayloads->push_back(payload.toUtf8());
        if (outFormats)
            outFormats->push_back(format);
    }
    if (outPayloads->isEmpty()) {
        if (error)
            *error = QStringLiteral("没有勾选启用的列表行");
        return false;
    }
    return true;
}

void MainWindow::onSendListAddRow()
{
    addSendListRow(true, QStringLiteral("条目%1").arg(sendListTable_->rowCount() + 1),
                   QStringLiteral("0,0"), 1000, isLegacyMode() ? FormatValues : FormatAuto);
}

void MainWindow::onSendListRemoveRows()
{
    if (!sendListTable_)
        return;
    const auto ranges = sendListTable_->selectionModel()->selectedRows();
    QList<int> rows;
    for (const QModelIndex& idx : ranges)
        rows.push_back(idx.row());
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        sendListTable_->removeRow(r);
}

void MainWindow::onSendListImport()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("导入发送列表"), QString(),
        QStringLiteral("文本/CSV (*.txt *.csv);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;
    QString err;
    if (!importSendListFile(path, &err)) {
        appendLog(QStringLiteral("[边界] 导入失败：%1").arg(err));
        return;
    }
    appendLog(QStringLiteral("[连接] 已导入发送列表：%1（共 %2 行）")
                  .arg(path)
                  .arg(sendListTable_->rowCount()));
}

void MainWindow::onSendListExport()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出发送列表"), QStringLiteral("send-list.csv"),
        QStringLiteral("CSV (*.csv);;文本 (*.txt)"));
    if (path.isEmpty())
        return;
    QString err;
    if (!exportSendListFile(path, &err)) {
        appendLog(QStringLiteral("[边界] 导出失败：%1").arg(err));
        return;
    }
    appendLog(QStringLiteral("[连接] 已导出发送列表：%1").arg(path));
}

void MainWindow::onSendListSendSelected()
{
    if (activeSession()->state() != ca::SessionState::Connected) {
        appendLog(QStringLiteral("[边界] 发送拒绝：会话未连接"));
        return;
    }
    if (!sendListTable_)
        return;

    // 与调度一致：只处理「启用」勾选行，不以鼠标高亮为准
    int sent = 0;
    for (int r = 0; r < sendListTable_->rowCount(); ++r) {
        QTableWidgetItem* enItem = sendListTable_->item(r, 0);
        if (!enItem || enItem->checkState() != Qt::Checked)
            continue;
        const QString payload =
            sendListTable_->item(r, 2) ? sendListTable_->item(r, 2)->text().trimmed() : QString();
        int format = FormatAuto;
        if (auto* combo = qobject_cast<QComboBox*>(sendListTable_->cellWidget(r, 4)))
            format = combo->currentData().toInt();

        if (isLegacyMode()) {
            ca::SendRequest req;
            QString err;
            const ca::Result built = buildLegacySendRequestFrom(payload, format, &req, &err);
            if (!built.ok) {
                appendLog(QStringLiteral("[边界] 发送拒绝：第 %1 行 — %2")
                              .arg(r + 1)
                              .arg(err.isEmpty() ? built.message : err));
                continue;
            }
            const ca::Result sr = legacySession_.send(req);
            if (!sr.ok)
                appendLog(QStringLiteral("[异常] 第 %1 行发送失败：%2").arg(r + 1).arg(sr.message));
            else
                ++sent;
            continue;
        }

        QByteArray bytes;
        QString err;
        if (format == FormatHex) {
            bytes = ca::parseHexPayloadStrict(payload, &err);
        } else if (preferHexSend()) {
            bytes = ca::parseHexPayloadStrict(payload, &err);
            // 与单条一致：侧栏勾选 HEX 时，非法字符按 UTF-8 明文发出
            if (bytes.isEmpty() && err.contains(QStringLiteral("非法十六进制字符"))) {
                err.clear();
                bytes = payload.toUtf8();
            }
        } else {
            bytes = payload.toUtf8();
        }
        if (!err.isEmpty() || bytes.isEmpty()) {
            appendLog(QStringLiteral("[边界] 发送拒绝：第 %1 行 — %2")
                          .arg(r + 1)
                          .arg(err.isEmpty() ? QStringLiteral("载荷为空") : err));
            continue;
        }
        ca::SendRequest req;
        req.requestId = QUuid::createUuid();
        req.sessionId = session_.sessionId();
        req.payload = bytes;
        fillChannelOnRequest(&req);
        const ca::Result sr = session_.send(req);
        if (!sr.ok)
            appendLog(QStringLiteral("[异常] 第 %1 行发送失败：%2").arg(r + 1).arg(sr.message));
        else
            ++sent;
    }

    if (sent == 0)
        appendLog(QStringLiteral("[边界] 发送拒绝：没有勾选启用的列表行，或启用行均未发送成功"));
    else
        appendLog(QStringLiteral("[连接] 已按启用行发送 %1 组").arg(sent));
}

bool MainWindow::importSendListFile(const QString& path, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("无法打开文件");
        return false;
    }
    QTextStream in(&f);
    in.setCodec("UTF-8");
    const QString all = in.readAll();
    const QStringList lines = all.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        if (error)
            *error = QStringLiteral("文件为空");
        return false;
    }

    sendListTable_->setRowCount(0);
    const bool asCsv = path.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)
                       || lines.first().contains(QLatin1Char(','));

    int loaded = 0;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (asCsv && i == 0
            && (line.contains(QStringLiteral("enabled"), Qt::CaseInsensitive)
                || line.contains(QStringLiteral("启用")) || line.contains(QStringLiteral("payload"))))
            continue;

        bool enabled = true;
        QString name = QStringLiteral("导入%1").arg(loaded + 1);
        QString payload;
        int intervalMs = schedIntervalSpin_ ? schedIntervalSpin_->value() : 1000;
        int format = FormatAuto;

        if (asCsv) {
            // 简易 CSV：enabled,name,payload,interval_ms,mode（payload 可用双引号包裹）
            QStringList cols;
            QString cur;
            bool inQ = false;
            for (int c = 0; c < line.size(); ++c) {
                const QChar ch = line.at(c);
                if (ch == QLatin1Char('"')) {
                    inQ = !inQ;
                    continue;
                }
                if (ch == QLatin1Char(',') && !inQ) {
                    cols << cur;
                    cur.clear();
                    continue;
                }
                cur.append(ch);
            }
            cols << cur;
            if (cols.size() >= 3) {
                enabled = !(cols.at(0).trimmed() == QStringLiteral("0")
                            || cols.at(0).trimmed().compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0);
                name = cols.at(1).trimmed();
                payload = cols.at(2).trimmed();
                if (cols.size() >= 4) {
                    bool ok = false;
                    const int v = cols.at(3).trimmed().toInt(&ok);
                    if (ok)
                        intervalMs = v;
                }
                if (cols.size() >= 5)
                    format = formatFromName(cols.at(4));
            } else if (cols.size() == 1) {
                payload = cols.at(0).trimmed();
            } else {
                if (error)
                    *error = QStringLiteral("第 %1 行 CSV 列不足").arg(i + 1);
                return false;
            }
        } else {
            payload = line;
        }

        if (payload.isEmpty())
            continue;
        addSendListRow(enabled, name, payload, intervalMs, format);
        ++loaded;
    }
    if (loaded == 0) {
        if (error)
            *error = QStringLiteral("未解析到有效行");
        return false;
    }
    return true;
}

bool MainWindow::exportSendListFile(const QString& path, QString* error) const
{
    if (!sendListTable_ || sendListTable_->rowCount() == 0) {
        if (error)
            *error = QStringLiteral("列表为空");
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("无法写入文件");
        return false;
    }
    QTextStream out(&f);
    out.setCodec("UTF-8");
    const bool asCsv = path.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive);
    if (asCsv)
        out << QStringLiteral("enabled,name,payload,interval_ms,mode\n");

    for (int r = 0; r < sendListTable_->rowCount(); ++r) {
        const bool en = sendListTable_->item(r, 0) && sendListTable_->item(r, 0)->checkState() == Qt::Checked;
        const QString name = sendListTable_->item(r, 1) ? sendListTable_->item(r, 1)->text() : QString();
        const QString payload = sendListTable_->item(r, 2) ? sendListTable_->item(r, 2)->text() : QString();
        const QString interval = sendListTable_->item(r, 3) ? sendListTable_->item(r, 3)->text() : QStringLiteral("1000");
        int format = FormatAuto;
        if (auto* combo = qobject_cast<QComboBox*>(sendListTable_->cellWidget(r, 4)))
            format = combo->currentData().toInt();
        if (asCsv) {
            QString p = payload;
            p.replace(QLatin1Char('"'), QStringLiteral("\"\""));
            out << (en ? QStringLiteral("1") : QStringLiteral("0")) << QLatin1Char(',')
                << QLatin1Char('"') << name << QLatin1Char('"') << QLatin1Char(',')
                << QLatin1Char('"') << p << QLatin1Char('"') << QLatin1Char(',')
                << interval << QLatin1Char(',')
                << formatName(format) << QLatin1Char('\n');
        } else {
            out << payload << QLatin1Char('\n');
        }
    }
    return true;
}
