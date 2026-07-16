// MainWindow.cpp — CommLab 软件侧实现
#include "MainWindow.h"

#include "CommController.h"
#include "CommHandler.h"
#include "DemoComm.h"
#include "ProtoGuide.h"

#include <QDateTime>

// 构造：默认串口三思，挂接信号
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ctrl(new CommController(this))
{
    ui.setupUi(this);
    setWindowTitle(QStringLiteral("CommLab — 软件"));
    ui.cmbCommType->addItems({QStringLiteral("UDP"), QStringLiteral("串口")});
    ui.cmbCommType->setCurrentIndex(1);
    ProtoGuide::fillProtoCombos(ui.cmbNetProto, ui.cmbSerialProto);
    ProtoGuide::fillSerialCombos(ui.cmbBaud, ui.cmbDataBits, ui.cmbParity, ui.cmbStopBits);
    m_net = LoadNetConfig();
    ui.spinLocalPort->setValue(m_net.localPort);
    ui.spinDestPort->setValue(m_net.destPort);
    ui.editLocalIp->setText(m_net.localIp);
    ui.editDestIp->setText(m_net.destIp);

    const auto onType = [this](int) { syncUi(); };
    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.cmbNetProto, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.cmbSerialProto, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [this](int v) { ui.lblComEcho->setText(QStringLiteral("COM%1").arg(v + 1)); });
    QObject::connect(ui.chkAllowSend, &QCheckBox::toggled, this, [this](bool on) {
        if (m_ready) {
            m_ctrl->handler()->SetCommType(NETWORK);
            m_ctrl->handler()->setParameter(QStringLiteral("bInquireSendFlag"), on);
        }
        syncUi();
    });
    QObject::connect(ui.btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    QObject::connect(ui.btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    QObject::connect(ui.btnCmdStart, &QPushButton::clicked, this, [this] { sendCmd(ProtoGuide::CommCmd::Start); });
    QObject::connect(ui.btnCmdStop, &QPushButton::clicked, this, [this] { sendCmd(ProtoGuide::CommCmd::Stop); });
    QObject::connect(ui.btnCmdSave, &QPushButton::clicked, this, [this] { sendCmd(ProtoGuide::CommCmd::SaveImage); });
    QObject::connect(ui.btnCmdZero, &QPushButton::clicked, this, [this] { sendCmd(ProtoGuide::CommCmd::ZeroClear); });
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, ui.textLog, &QPlainTextEdit::clear);
    QObject::connect(m_ctrl, &CommController::safeDataReceived, this, &MainWindow::onSafeData, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::measureParseFailed, this,
                     [this](int) {
                         if (!udp())
                             log(QStringLiteral("[收← 未成帧] 串口有数据但库解析 size=0 · 检查 COM 配对或更新 CommHandler.dll"));
                     },
                     Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventReceived, this, &MainWindow::onEvent, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventWithDataReceived, this, &MainWindow::onEventWithData,
                     Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::peerConnected, this, &MainWindow::onPeerConnected, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::peerDisconnected, this, &MainWindow::onPeerDisconnected,
                     Qt::QueuedConnection);

    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
    m_ready = false;
    syncUi();
    log(QStringLiteral("[就绪] 默认串口三思：UDP 发指令，串口收试验机测量"));
}

// 析构：断开双通道
MainWindow::~MainWindow()
{
    if (m_ctrl) disconnectAll(m_ctrl->handler());
}

// 追加日志
void MainWindow::log(const QString& line)
{
    ui.textLog->appendPlainText(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line);
}

// 连接摘要
QString MainWindow::connSummary() const
{
    if (udp()) {
        return QStringLiteral("UDP · 本机 %1:%2 → 目的 %3:%4 · %5")
            .arg(ui.editLocalIp->text().trimmed())
            .arg(ui.spinLocalPort->value())
            .arg(ui.editDestIp->text().trimmed())
            .arg(ui.spinDestPort->value())
            .arg(ProtoGuide::protoName(true, netProto()));
    }
    return QStringLiteral("串口 COM%1·%2 + UDP 指令 %3:%4→%5:%6")
        .arg(ui.spinComPort->value() + 1)
        .arg(ProtoGuide::protoName(false, serialProto()))
        .arg(ui.editLocalIp->text().trimmed())
        .arg(ui.spinLocalPort->value())
        .arg(ui.editDestIp->text().trimmed())
        .arg(ui.spinDestPort->value());
}

// 刷新控件
void MainWindow::syncUi()
{
    const bool serialMode = !udp();
    ui.serialPanel->setVisible(serialMode);
    // 串口主闭环时仍须配置 UDP 指令面端口
    ui.udpPanel->setVisible(true);
    ui.lblNetProto->setVisible(udp());
    ui.cmbNetProto->setVisible(udp());

    const int sp = serialProto();
    QString line = udp() ? ProtoGuide::statusLine(true, netProto())
                         : ProtoGuide::statusLine(false, sp);
    if (m_ready && serialMode) {
        line += QStringLiteral(" · 串口%1 UDP%2")
                    .arg(m_serialUp ? QStringLiteral("OK") : QStringLiteral("FAIL"))
                    .arg(m_cmdNetUp ? QStringLiteral("OK") : QStringLiteral("FAIL"));
    }
    ui.lblStatus->setText(m_ready ? QStringLiteral("已连接 · %1").arg(line)
                                  : QStringLiteral("未连接 · %1").arg(line));

    const bool allowCmd = m_ready && m_cmdNetUp && ui.chkAllowSend->isChecked();
    ui.btnConnect->setEnabled(!m_ready);
    ui.btnDisconnect->setEnabled(m_ready);
    ui.cmbCommType->setEnabled(!m_ready);
    ui.udpPanel->setEnabled(!m_ready);
    ui.serialPanel->setEnabled(!m_ready);
    ui.chkAllowSend->setEnabled(m_ready && m_cmdNetUp);
    ui.btnCmdStart->setEnabled(allowCmd);
    ui.btnCmdStop->setEnabled(allowCmd);
    ui.btnCmdSave->setEnabled(allowCmd);
    ui.btnCmdZero->setEnabled(allowCmd);
}

// 写串口测量面参数
void MainWindow::applySerialParams()
{
    CommHandler* c = m_ctrl->handler();
    c->SetCommType(SERIAL);
    applyIoFlags(c, false);
    c->setParameter(QStringLiteral("iComPort"), ui.spinComPort->value());
    c->setParameter(QStringLiteral("iComBaud"), ui.cmbBaud->currentIndex());
    c->setParameter(QStringLiteral("iDataBits"), ui.cmbDataBits->currentData().toInt());
    c->setParameter(QStringLiteral("iParity"), ui.cmbParity->currentIndex());
    c->setParameter(QStringLiteral("iStopBits"), ui.cmbStopBits->currentIndex());
    c->setParameter(QStringLiteral("iProtocolType"), serialProto());
    c->setParameter(QStringLiteral("dInForceRange"), 100.0);
    c->setParameter(QStringLiteral("bInquireSendFlag"), false);
}

// 写 UDP JSON 指令面
void MainWindow::applyCmdNetParams()
{
    CommHandler* c = m_ctrl->handler();
    NetConfig cfg = m_net;
    cfg.localIp = ui.editLocalIp->text().trimmed();
    cfg.destIp = ui.editDestIp->text().trimmed();
    cfg.localPort = ui.spinLocalPort->value();
    cfg.destPort = ui.spinDestPort->value();
    c->SetCommType(NETWORK);
    applyIoFlags(c, false);
    c->setParameter(QStringLiteral("iTransferType"), cfg.transferType);
    c->setParameter(QStringLiteral("iModel"), cfg.model);
    c->setParameter(QStringLiteral("sLocalIP"), cfg.localIp);
    c->setParameter(QStringLiteral("iLocalPort"), cfg.localPort);
    c->setParameter(QStringLiteral("sDestIP"), cfg.destIp);
    c->setParameter(QStringLiteral("iDestPort"), cfg.destPort);
    c->setParameter(QStringLiteral("iProtoType"), 0);
    c->setParameter(QStringLiteral("bInquireSendFlag"), ui.chkAllowSend->isChecked());
}

// 写纯网口数据面
void MainWindow::applyUdpDataParams()
{
    CommHandler* c = m_ctrl->handler();
    NetConfig cfg = m_net;
    cfg.localIp = ui.editLocalIp->text().trimmed();
    cfg.destIp = ui.editDestIp->text().trimmed();
    cfg.localPort = ui.spinLocalPort->value();
    cfg.destPort = ui.spinDestPort->value();
    c->SetCommType(NETWORK);
    applyIoFlags(c, false);
    c->setParameter(QStringLiteral("iTransferType"), cfg.transferType);
    c->setParameter(QStringLiteral("iModel"), cfg.model);
    c->setParameter(QStringLiteral("sLocalIP"), cfg.localIp);
    c->setParameter(QStringLiteral("iLocalPort"), cfg.localPort);
    c->setParameter(QStringLiteral("sDestIP"), cfg.destIp);
    c->setParameter(QStringLiteral("iDestPort"), cfg.destPort);
    c->setParameter(QStringLiteral("iProtoType"), netProto());
    c->setParameter(QStringLiteral("bInquireSendFlag"), ui.chkAllowSend->isChecked());
}

// 连接
void MainWindow::onConnectClicked()
{
    CommHandler* c = m_ctrl->handler();
    m_serialUp = false;
    m_cmdNetUp = false;

    if (udp()) {
        applyUdpDataParams();
        m_cmdNetUp = c->Connect();
        if (!m_cmdNetUp) {
            log(QStringLiteral("[连接] 失败 · %1").arg(connSummary()));
            m_ready = false;
            syncUi();
            return;
        }
        log(QStringLiteral("[连接] 网口单通道 · %1").arg(connSummary()));
        log(QStringLiteral("[提示] 纯网口无法收试验机 tn=2 测量；主闭环请选串口"));
    } else {
        applySerialParams();
        c->SetCommType(SERIAL);
        m_serialUp = c->Connect();
        if (!m_serialUp) {
            log(QStringLiteral("[连接] 串口失败 · 检查 COM%1 是否存在/未被占用")
                    .arg(ui.spinComPort->value() + 1));
            m_ready = false;
            syncUi();
            return;
        }
        applyCmdNetParams();
        c->SetCommType(NETWORK);
        m_cmdNetUp = c->Connect();
        if (!m_cmdNetUp) {
            c->SetCommType(SERIAL);
            c->Disconnect(0);
            m_serialUp = false;
            log(QStringLiteral("[连接] UDP 指令面失败 · 请先启动 MachinePeer"));
            m_ready = false;
            syncUi();
            return;
        }
        log(QStringLiteral("[连接] 成功 · %1").arg(connSummary()));
        log(QStringLiteral("[连接] 串口=测量收 · UDP=指令发（Peer 须先打开）"));
    }

    ui.chkAllowSend->setChecked(true);
    c->SetCommType(NETWORK);
    c->setParameter(QStringLiteral("bInquireSendFlag"), true);
    m_ready = true;
    syncUi();
}

// 断开
void MainWindow::onDisconnectClicked()
{
    disconnectAll(m_ctrl->handler());
    m_ready = false;
    m_serialUp = false;
    m_cmdNetUp = false;
    log(QStringLiteral("[连接] 断开"));
    syncUi();
}

// 发四指令
void MainWindow::sendCmd(ProtoGuide::CommCmd cmd)
{
    if (!m_ready) { log(QStringLiteral("[拒发] 未连接")); return; }
    if (cmd == ProtoGuide::CommCmd::Start || cmd == ProtoGuide::CommCmd::Stop) {
        if (!m_cmdNetUp) { log(QStringLiteral("[拒发] UDP 指令面未连接")); return; }
    }
    CommHandler* c = m_ctrl->handler();
    QString fail;
    if (!sendCommand(c, !udp(), udp() ? netProto() : 0, cmd, &fail)) {
        log(QStringLiteral("[拒发] %1 · %2").arg(ProtoGuide::cmdDisplayName(cmd), fail));
        return;
    }
    log(QStringLiteral("[发→ 组包] %1 · %2")
            .arg(ProtoGuide::cmdDisplayName(cmd), ProtoGuide::explainSendCommand(udp(), netProto(), cmd)));
}

// 收试验机测量（串口 emitNewData）
void MainWindow::onSafeData(QVector<double> values, int type)
{
    const bool onSerial = !udp();
    log(QStringLiteral("[收← 解包] %1")
            .arg(ProtoGuide::explainRecvData(onSerial, onSerial ? serialProto() : netProto(), type, values)));
}

// 软件侧不应出现 emitEventMsg（只发指令、收测量）；若出现说明 IO 角色配置错误
void MainWindow::onEvent(int, int, int msg)
{
    log(QStringLiteral("[异常] %1").arg(ProtoGuide::explainRecvEvent(msg, CommController::eventName(msg))));
}

void MainWindow::onEventWithData(int, int, int msg, QVariantMap extra)
{
    QStringList parts;
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it)
        parts << QStringLiteral("%1=%2").arg(it.key(), it.value().toString());
    log(QStringLiteral("[异常] %1 · 附加 {%2}")
            .arg(ProtoGuide::explainRecvEvent(msg, CommController::eventName(msg)), parts.join(QLatin1Char(','))));
}

void MainWindow::onPeerConnected(int, const QString& ip, int port)
{
    log(QStringLiteral("[对端] 连入 %1:%2").arg(ip).arg(port));
}

void MainWindow::onPeerDisconnected()
{
    log(QStringLiteral("[对端] 断开"));
}
