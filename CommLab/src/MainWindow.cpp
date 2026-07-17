// MainWindow.cpp — CommLab：入站测数经 processForceTemp 处理后调用库 SendData 回发
#include "MainWindow.h"

#include "CommController.h"
#include "CommHandler.h"
#include "DemoComm.h"
#include "ProtoCapability.h"
#include "ProtoGuide.h"
#include "UIDef.h"

#include <QDateTime>
#include <QJsonDocument>

// 构造：界面初始化、协议填充、库信号排队到 UI 线程
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ctrl(new CommController(this))
{
    ui.setupUi(this);
    setWindowTitle(QStringLiteral("CommLab — 软件"));
    ui.cmbCommType->clear();
    ui.cmbCommType->addItems({QStringLiteral("网口"), QStringLiteral("串口")});
    ui.cmbCommType->setCurrentIndex(1);
    ProtoGuide::fillProtoCombos(ui.cmbNetProto, ui.cmbSerialProto);
    ProtoGuide::fillSerialCombos(ui.cmbBaud, ui.cmbDataBits, ui.cmbParity, ui.cmbStopBits);
    m_net = LoadNetConfig();
    ui.spinLocalPort->setValue(m_net.localPort);
    ui.spinDestPort->setValue(m_net.destPort);
    ui.editLocalIp->setText(m_net.localIp);
    ui.editDestIp->setText(m_net.destIp);
    ui.chkAllowSend->setText(QStringLiteral("收测后回发"));
    ui.chkAllowSend->setChecked(true);

    QObject::connect(ui.cmbCommType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     [this](int) { syncUi(); });
    QObject::connect(ui.spinComPort, QOverload<int>::of(&QSpinBox::valueChanged), this,
                     [this](int v) { ui.lblComEcho->setText(QStringLiteral("COM%1").arg(v + 1)); });
    QObject::connect(ui.chkAllowSend, &QCheckBox::toggled, this, [this](bool on) {
        if (m_ready && m_ctrl) {
            m_ctrl->handler()->setParameter(QStringLiteral("bInquireSendFlag"), on);
        }
    });
    QObject::connect(ui.btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    QObject::connect(ui.btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    QObject::connect(ui.btnClearLog, &QPushButton::clicked, ui.textLog, &QPlainTextEdit::clear);
    QObject::connect(m_ctrl, &CommController::safeDataReceived, this, &MainWindow::onSafeData, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventReceived, this, &MainWindow::onEvent, Qt::QueuedConnection);
    QObject::connect(m_ctrl, &CommController::eventWithDataReceived, this, &MainWindow::onEventWithData,
                     Qt::QueuedConnection);

    ui.lblComEcho->setText(QStringLiteral("COM%1").arg(ui.spinComPort->value() + 1));
    syncUi();
}

// 析构：断开 CommHandler
MainWindow::~MainWindow()
{
    if (m_ctrl)
        disconnectCurrent(m_ctrl->handler());
}

// 收发区追加带时戳日志
void MainWindow::log(const QString& line)
{
    ui.textLog->appendPlainText(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss ")) + line);
}

// 刷新面板与使能
void MainWindow::syncUi()
{
    const bool net = isNetwork();
    ui.serialPanel->setVisible(!net);
    ui.udpPanel->setVisible(net);
    ui.lblNetProto->setVisible(net);
    ui.cmbNetProto->setVisible(net);
    ui.btnConnect->setEnabled(!m_ready);
    ui.btnDisconnect->setEnabled(m_ready);
    ui.cmbCommType->setEnabled(!m_ready);
    ui.udpPanel->setEnabled(!m_ready);
    ui.serialPanel->setEnabled(!m_ready);
    ui.chkAllowSend->setEnabled(m_ready);
    if (!m_ready)
        ui.lblStatus->setText(QStringLiteral("未连接"));
    else
        ui.lblStatus->setText(m_running ? QStringLiteral("运行中") : QStringLiteral("已连接"));
}

// 处理入站测数并调用库 SendData；门控失败则绝不调用 SendData
void MainWindow::processAndReply()
{
    if (!m_ready || !ui.chkAllowSend->isChecked() || !m_hasForce || !m_running)
        return;
    const CommType t = isNetwork() ? NETWORK : SERIAL;
    const int p = isNetwork() ? netProto() : serialProto();
    if (!ProtoCapability::canBusinessReply(t, p)) {
        log(isNetwork() ? QStringLiteral("INFO %1").arg(ProtoCapability::netReplyDenyReason(p))
                        : QStringLiteral("INFO 本串口协议库无业务回发"));
        return;
    }

    double forceOut = 0.0;
    double tempOut = 0.0;
    // true：处理后仍带真实温度
    bool hasTempOut = false;
    processForceTemp(m_force, m_hasTemp, m_temp, &forceOut, &hasTempOut, &tempOut);

    // 是否把温写入 SendData 的 vector（无入站温或科新仅力时为 false）
    bool includeTemp = hasTempOut;
    if (t == SERIAL && ProtoCapability::serialReplyForceOnly(p, hasTempOut))
        includeTemp = false;

    if (!replyProcessed(m_ctrl->handler(), t, p, forceOut, includeTemp, tempOut))
        return;

    if (includeTemp)
        log(QStringLiteral("TX  F=%1 T=%2 (已 SendData)").arg(forceOut, 0, 'g', 8).arg(tempOut, 0, 'g', 8));
    else
        log(QStringLiteral("TX  F=%1 (已 SendData,无入站温不回发温)").arg(forceOut, 0, 'g', 8));
}

// 仅打开当前通道
void MainWindow::onConnectClicked()
{
    CommHandler* c = m_ctrl->handler();
    m_running = false;
    m_hasForce = false;
    m_hasTemp = false;
    m_force = 0.0;
    m_temp = 0.0;
    disconnectCurrent(c);

    bool ok = false;
    if (isNetwork()) {
        setupNetwork(c, m_net.transferType, m_net.model, ui.editLocalIp->text().trimmed(),
                     ui.spinLocalPort->value(), ui.editDestIp->text().trimmed(), ui.spinDestPort->value(),
                     netProto());
        ok = c->Connect();
        if (!ok)
            log(QStringLiteral("ERR net"));
    } else {
        setupSerial(c, ui.spinComPort->value(), ui.cmbBaud->currentIndex(),
                    ui.cmbDataBits->currentData().toInt(), ui.cmbParity->currentIndex(),
                    ui.cmbStopBits->currentIndex(), serialProto());
        ok = c->Connect();
        if (!ok)
            log(QStringLiteral("ERR COM%1").arg(ui.spinComPort->value() + 1));
    }
    m_ready = ok;
    if (ok && !isNetwork() && serialProto() == 1)
        m_running = true;
    syncUi();
}

// 断开连接
void MainWindow::onDisconnectClicked()
{
    if (m_ctrl) {
        if (isNetwork())
            m_ctrl->handler()->setParameter(QStringLiteral("bOnlineCollect"), false);
        disconnectCurrent(m_ctrl->handler());
    }
    m_ready = false;
    m_running = false;
    m_hasForce = false;
    m_hasTemp = false;
    syncUi();
}

// 测数入站：只把库给出的真实通道记为力/温，再处理并 SendData
void MainWindow::onSafeData(QVector<double> values, int /*type*/)
{
    if (values.isEmpty())
        return;

    m_force = values.at(0);
    m_hasForce = true;
    m_hasTemp = false;
    m_temp = 0.0;

    // 三思/科新：values[1] 是状态枚举，不是温度
    const bool statusNotTemp =
        !isNetwork() && ProtoCapability::serialSecondIsStatusNotTemp(serialProto());
    if (!statusNotTemp && values.size() >= 2) {
        m_temp = values.at(1);
        m_hasTemp = true;
    }

    if (m_hasTemp)
        log(QStringLiteral("RX  F=%1 T=%2").arg(m_force, 0, 'g', 8).arg(m_temp, 0, 'g', 8));
    else
        log(QStringLiteral("RX  F=%1").arg(m_force, 0, 'g', 8));

    processAndReply();
}

// 指令入站：同步运行状态，不触发 SendData
void MainWindow::onEvent(int, int, int msg)
{
    log(QStringLiteral("RX  CMD %1").arg(CommController::eventName(msg)));
    if (!m_ctrl)
        return;
    if (msg == W_CUSTOM_COMM_STARTCALC) {
        if (isNetwork())
            m_ctrl->handler()->setParameter(QStringLiteral("bOnlineCollect"), true);
        m_running = true;
        syncUi();
    } else if (msg == W_CUSTOM_COMM_STOPCALC) {
        if (isNetwork())
            m_ctrl->handler()->setParameter(QStringLiteral("bOnlineCollect"), false);
        m_running = false;
        syncUi();
    }
}

// 附加数据事件
void MainWindow::onEventWithData(int, int, int msg, QVariantMap extra)
{
    log(QStringLiteral("RX  CMD %1 extra=%2")
            .arg(CommController::eventName(msg))
            .arg(QString::fromUtf8(QJsonDocument::fromVariant(extra).toJson(QJsonDocument::Compact))));
    if (msg == W_CUSTOM_COMM_STARTCALC || msg == W_CUSTOM_COMM_STOPCALC)
        onEvent(0, 0, msg);
}
