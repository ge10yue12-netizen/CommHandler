// MainWindow.cpp — CommLab：入站测数经 processForceTemp 处理后调用库 SendData 回发
#include "MainWindow.h"

#include "CommController.h"
#include "CommHandler.h"
#include "NetworkProtocol.h"
#include "ProtoGuide.h"
#include "SerialProtocol.h"
#include "UIDef.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QTimer>
#include <exception>
#include <new>

namespace {

// 业务算法扩展点；示例保持输入输出一致，产品可替换为标定或滤波
void processMeasurement(double forceIn, bool hasTempIn, double tempIn, double* forceOut,
                        bool* hasTempOut, double* tempOut)
{
    *forceOut = forceIn;
    *hasTempOut = hasTempIn;
    *tempOut = hasTempIn ? tempIn : 0.0;
}

} // namespace

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
        if (m_ready && m_ctrl)
            m_ctrl->channels()->setReplyEnabled(on);
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
        m_ctrl->channels()->close();
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

// 处理入站测数并调用库 SendData
void MainWindow::processAndReply()
{
    if (!m_ready || !ui.chkAllowSend->isChecked() || !m_ctrl || !m_ctrl->handler())
        return;

    CommHandler* handler = m_ctrl->handler();
    if (isNetwork()) {
        if (!m_hasForce)
            return;
        const int proto = netProto();
        if (proto != 2)
            return;
        try {
            if (!NetworkProtocol::sendMeasurementReply(handler, proto, m_force, m_hasTemp, m_temp))
                return;
            if (m_hasTemp)
                log(QStringLiteral("TX  F=%1 T=%2")
                        .arg(m_force, 0, 'g', 8)
                        .arg(m_temp, 0, 'g', 8));
            else
                log(QStringLiteral("TX  F=%1").arg(m_force, 0, 'g', 8));
        } catch (const std::bad_alloc&) {
            log(QStringLiteral("ERR SendData 内存分配失败，已跳过回发"));
        } catch (const std::exception& error) {
            log(QStringLiteral("ERR SendData：%1").arg(QString::fromLocal8Bit(error.what())));
        }
        return;
    }

    if (!m_hasForce || !m_running)
        return;
    const int proto = serialProto();
    if (!SerialProtocol::canReplyToMeasurement(proto)) {
        log(QStringLiteral("INFO %1").arg(SerialProtocol::replyUnavailableReason(proto)));
        return;
    }

    double forceOut = 0.0;
    double tempOut = 0.0;
    bool hasTempOut = false;
    processMeasurement(m_force, m_hasTemp, m_temp, &forceOut, &hasTempOut, &tempOut);

    const int protoCopy = proto;
    const bool hasTempCopy = hasTempOut;
    const double forceCopy = forceOut;
    const double tempCopy = tempOut;

    // 串口仍推迟到下一事件循环，避免回调栈内重入 SendData
    QTimer::singleShot(0, this, [this, handler, protoCopy, hasTempCopy, forceCopy, tempCopy]() {
        try {
            if (!SerialProtocol::sendProcessed(handler, protoCopy, forceCopy, hasTempCopy, tempCopy))
                return;
            if (hasTempCopy && protoCopy != 1)
                log(QStringLiteral("TX  F=%1 T=%2")
                        .arg(forceCopy, 0, 'g', 8)
                        .arg(tempCopy, 0, 'g', 8));
            else
                log(QStringLiteral("TX  F=%1").arg(forceCopy, 0, 'g', 8));
        } catch (const std::bad_alloc&) {
            log(QStringLiteral("ERR SendData 内存分配失败，已跳过回发"));
        } catch (const std::exception& error) {
            log(QStringLiteral("ERR SendData：%1").arg(QString::fromLocal8Bit(error.what())));
        }
    });
}

// 从串口面板生成完整初始化参数
SerialChannelSettings MainWindow::serialSettings() const
{
    SerialChannelSettings settings;
    settings.portIndex = ui.spinComPort->value();
    settings.baudIndex = ui.cmbBaud->currentIndex();
    settings.dataBitsIndex = ui.cmbDataBits->currentData().toInt();
    settings.parityIndex = ui.cmbParity->currentIndex();
    settings.stopBitsIndex = ui.cmbStopBits->currentIndex();
    settings.protocolIndex = serialProto();
    return settings;
}

// 从网口面板生成完整初始化参数
NetworkChannelSettings MainWindow::networkSettings() const
{
    NetworkChannelSettings settings;
    settings.transferType = m_net.transferType;
    settings.model = m_net.model;
    settings.localIp = ui.editLocalIp->text().trimmed();
    settings.localPort = ui.spinLocalPort->value();
    settings.destinationIp = ui.editDestIp->text().trimmed();
    settings.destinationPort = ui.spinDestPort->value();
    settings.protocolIndex = netProto();
    return settings;
}

// 清除运行状态和最近测量值
void MainWindow::resetRuntimeState()
{
    m_running = false;
    m_hasForce = false;
    m_hasTemp = false;
    m_force = 0.0;
    m_temp = 0.0;
}

// 通过 CommChannelManager 打开当前通道
void MainWindow::onConnectClicked()
{
    resetRuntimeState();
    QString error;
    m_ready = isNetwork()
        ? m_ctrl->channels()->openNetwork(networkSettings(), &error)
        : m_ctrl->channels()->openSerial(serialSettings(), &error);

    if (!m_ready)
        log(QStringLiteral("ERR %1").arg(error));
    if (m_ready && !isNetwork() && serialProto() == 1)
        m_running = true;
    syncUi();
}

// 断开连接
void MainWindow::onDisconnectClicked()
{
    if (m_ctrl) {
        if (isNetwork())
            m_ctrl->channels()->setNetworkCollecting(false);
        m_ctrl->channels()->close();
    }
    m_ready = false;
    resetRuntimeState();
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

    const bool statusNotTemp = !isNetwork() && SerialProtocol::secondValueIsStatus(serialProto());
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
            m_ctrl->channels()->setNetworkCollecting(true);
        m_running = true;
        syncUi();
    } else if (msg == W_CUSTOM_COMM_STOPCALC) {
        if (isNetwork())
            m_ctrl->channels()->setNetworkCollecting(false);
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
