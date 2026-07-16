// MainWindow.cpp — MachinePeer 试验机侧实现
#include "MainWindow.h"

#include "CommController.h"
#include "CommHandler.h"
#include "DemoComm.h"
#include "ProtoGuide.h"
#include "UIDef.h"

#include <QDateTime>

// 构造：默认串口三思
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ctrl(new CommController(this))
{
    ui.setupUi(this);
    setWindowTitle(QStringLiteral("MachinePeer — 试验机"));
    ui.cmbCommType->addItems({QStringLiteral("UDP"), QStringLiteral("串口")});
    ui.cmbCommType->setCurrentIndex(1);
    ProtoGuide::fillProtoCombos(ui.cmbNetProto, ui.cmbSerialProto);
    ProtoGuide::fillSerialCombos(ui.cmbBaud, ui.cmbDataBits, ui.cmbParity, ui.cmbStopBits);
    m_cfg = LoadPeerConfig();
    ui.editLocalIp->setText(m_cfg.localIp);
    ui.editDestIp->setText(m_cfg.destIp);
    ui.spinLocalPort->setValue(m_cfg.localPort);
    ui.spinDestPort->setValue(m_cfg.destPort);
    ui.spinComPort->setValue(m_cfg.comPort);

    const auto onType = [this](int) { syncUi(); };
    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.cmbNetProto, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.cmbSerialProto, QOverload<int>::of(&QComboBox::currentIndexChanged), this, onType);
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [this](int v) { ui.lblComEcho->setText(QStringLiteral("COM%1").arg(v + 1)); });
    QObject::connect(ui.btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    QObject::connect(ui.btnClose, &QPushButton::clicked, this, &MainWindow::onCloseClicked);
    QObject::connect(ui.btnSendMeasure, &QPushButton::clicked, this, [this] { sendMeasure(); });
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, ui.textLog, &QPlainTextEdit::clear);
    QObject::connect(m_ctrl, &CommController::safeDataReceived, this, &MainWindow::onSafeData, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventReceived, this, &MainWindow::onEvent, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventWithDataReceived, this, &MainWindow::onEventWithData,
                     Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::peerConnected, this, &MainWindow::onPeerConnected, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::peerDisconnected, this, &MainWindow::onPeerDisconnected,
                     Qt::QueuedConnection);

    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
    m_ready = false;
    syncUi();
    log(QStringLiteral("[就绪] 先打开；串口发测量，UDP 收软件指令"));
}

MainWindow::~MainWindow()
{
    if (m_ctrl) disconnectAll(m_ctrl->handler());
}

void MainWindow::log(const QString& line)
{
    ui.textLog->appendPlainText(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line);
}

QString MainWindow::connSummary() const
{
    if (udp()) {
        return QStringLiteral("UDP · 本机 %1:%2 · 目的 %3:%4 · %5")
            .arg(ui.editLocalIp->text().trimmed())
            .arg(ui.spinLocalPort->value())
            .arg(ui.editDestIp->text().trimmed())
            .arg(ui.spinDestPort->value())
            .arg(ProtoGuide::protoName(true, proto()));
    }
    return QStringLiteral("串口 COM%1·%2 + UDP 指令 %3:%4")
        .arg(ui.spinComPort->value() + 1)
        .arg(ProtoGuide::protoName(false, serialProto()))
        .arg(ui.editLocalIp->text().trimmed())
        .arg(ui.spinLocalPort->value());
}

void MainWindow::syncUi()
{
    const bool serialMode = !udp();
    ui.serialPanel->setVisible(serialMode);
    ui.udpPanel->setVisible(true);
    ui.lblNetProto->setVisible(udp());
    ui.cmbNetProto->setVisible(udp());

    QString line = udp() ? ProtoGuide::statusLine(true, proto())
                         : ProtoGuide::statusLine(false, serialProto());
    if (m_ready && serialMode) {
        line += QStringLiteral(" · 串口%1 UDP%2")
                    .arg(m_serialUp ? QStringLiteral("OK") : QStringLiteral("FAIL"))
                    .arg(m_cmdNetUp ? QStringLiteral("OK") : QStringLiteral("FAIL"));
    }
    ui.lblStatus->setText(m_ready ? QStringLiteral("已打开 · %1").arg(line)
                                  : QStringLiteral("未打开 · %1").arg(line));

    ui.btnOpen->setEnabled(!m_ready);
    ui.btnClose->setEnabled(m_ready);
    ui.cmbCommType->setEnabled(!m_ready);
    ui.udpPanel->setEnabled(!m_ready);
    ui.serialPanel->setEnabled(!m_ready);
    ui.btnSendMeasure->setEnabled(m_ready);
    ui.editSendValues->setEnabled(m_ready);
    ui.chkAutoReply->setEnabled(m_ready);
    ui.editSendValues->setPlaceholderText(ProtoGuide::measurePlaceholder(false, serialProto()));
}

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
    c->setParameter(QStringLiteral("bInquireSendFlag"), ProtoGuide::canSendValues(false, serialProto()));
}

void MainWindow::applyCmdNetParams()
{
    CommHandler* c = m_ctrl->handler();
    PeerConfig cfg = m_cfg;
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
    c->setParameter(QStringLiteral("bInquireSendFlag"), false);
}

void MainWindow::applyUdpParams()
{
    CommHandler* c = m_ctrl->handler();
    PeerConfig cfg = m_cfg;
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
    c->setParameter(QStringLiteral("iProtoType"), proto());
    c->setParameter(QStringLiteral("bInquireSendFlag"), false);
}

void MainWindow::onOpenClicked()
{
    CommHandler* c = m_ctrl->handler();
    m_serialUp = false;
    m_cmdNetUp = false;

    if (udp()) {
        applyUdpParams();
        m_cmdNetUp = c->Connect();
        if (!m_cmdNetUp) {
            log(QStringLiteral("[连接] 失败 · %1").arg(connSummary()));
            m_ready = false;
            syncUi();
            return;
        }
        log(QStringLiteral("[连接] 网口单通道 · %1").arg(connSummary()));
    } else {
        applySerialParams();
        c->SetCommType(SERIAL);
        m_serialUp = c->Connect();
        if (!m_serialUp) {
            log(QStringLiteral("[连接] 串口失败 · 检查 COM%1").arg(ui.spinComPort->value() + 1));
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
            log(QStringLiteral("[连接] UDP 指令面失败 · 端口 %1 是否被占用").arg(ui.spinLocalPort->value()));
            m_ready = false;
            syncUi();
            return;
        }
        log(QStringLiteral("[连接] 成功 · %1").arg(connSummary()));
        log(QStringLiteral("[连接] 串口=测量发 · UDP=指令收"));
    }
    m_ready = true;
    syncUi();
}

void MainWindow::onCloseClicked()
{
    disconnectAll(m_ctrl->handler());
    m_ready = false;
    m_serialUp = false;
    m_cmdNetUp = false;
    log(QStringLiteral("[连接] 关闭"));
    syncUi();
}

void MainWindow::sendMeasure()
{
    if (!m_ready) { log(QStringLiteral("[拒发] 未打开")); return; }
    const QVector<double> v = ProtoGuide::parseValues(ui.editSendValues->text());
    QString fail;
    if (!sendValues(m_ctrl->handler(), udp(), udp() ? proto() : serialProto(), v, &fail)) {
        log(QStringLiteral("[拒发] %1").arg(fail));
        return;
    }
    log(QStringLiteral("[发→ 组包] %1")
            .arg(ProtoGuide::explainSendVector(udp(), udp() ? proto() : serialProto(), v)));
}

void MainWindow::onSafeData(QVector<double> values, int type)
{
    Q_UNUSED(values);
    Q_UNUSED(type);
    log(QStringLiteral("[收← 非预期] 试验机不应收测量（仅软件机收 emitNewData）"));
}

void MainWindow::onEvent(int, int, int msg)
{
    if (msg == W_CUSTOM_COMM_STARTCALC) {
        log(QStringLiteral("[收← 解包] %1").arg(ProtoGuide::explainRecvEvent(msg, CommController::eventName(msg))));
        if (!ui.chkAutoReply->isChecked())
            return;
        const QString text = ui.editSendValues->text().trimmed();
        if (text.isEmpty()) {
            const QVector<double> def{1.0};
            QString fail;
            if (!sendValues(m_ctrl->handler(), udp(), udp() ? proto() : serialProto(), def, &fail))
                log(QStringLiteral("[拒发] %1").arg(fail));
            else
                log(QStringLiteral("[发→ 组包] %1（自动应答默认 1.0）")
                        .arg(ProtoGuide::explainSendVector(udp(), udp() ? proto() : serialProto(), def)));
            return;
        }
        sendMeasure();
        return;
    }
    if (msg == W_CUSTOM_COMM_STOPCALC) {
        log(QStringLiteral("[收← 解包] %1").arg(ProtoGuide::explainRecvEvent(msg, CommController::eventName(msg))));
        return;
    }
    log(QStringLiteral("[收← 解包] %1").arg(ProtoGuide::explainRecvEvent(msg, CommController::eventName(msg))));
}

void MainWindow::onEventWithData(int, int, int msg, QVariantMap extra)
{
    QStringList parts;
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it)
        parts << QStringLiteral("%1=%2").arg(it.key(), it.value().toString());
    log(QStringLiteral("[收← 解包] %1 · 附加 {%2}")
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
