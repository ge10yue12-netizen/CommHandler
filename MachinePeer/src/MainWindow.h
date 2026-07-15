#pragma once

#include "PeerConfig.h"
#include "WireCodec.h"
#include "ui_MainWindow.h"

#include <QByteArray>
#include <QComboBox>
#include <QMainWindow>
#include <QSerialPort>
#include <QUdpSocket>

// 试验机主窗：双方皆可收发；主路径为收软件指令并执行，测量值回传对端。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 加载配置、填充控件并进入未打开状态。
    explicit MainWindow(QWidget* parent = nullptr);
    // 关闭串口与 UDP 后销毁。
    ~MainWindow() override;

private slots:
    // 切换串口 / UDP 面板可见性。
    void onCommTypeChanged(int index);
    // 按界面参数打开串口或绑定 UDP。
    void onOpenClicked();
    // 关闭当前通道并清空串口收缓冲。
    void onCloseClicked();
    // 手动向对端发送一组测量值。
    void onSendMeasureClicked();
    // 清空运行记录。
    void onClearLogClicked();
    // 附加能力：本端主动发「开始」指令到对端（双向测试）。
    void onCmdStartClicked();
    // 附加能力：本端主动发「停止」指令。
    void onCmdStopClicked();
    // 附加能力：本端主动发「存图」指令。
    void onCmdSaveClicked();
    // 附加能力：本端主动发「清零」指令。
    void onCmdZeroClicked();
    // 串口有可读数据时切帧并处理。
    void onSerialReadyRead();
    // UDP 有数据报时记录；若为控制指令则执行。
    void onUdpReadyRead();
    // 本地自检：不经端口注入「开始」指令帧。
    void onSelfTestParseClicked();

private:
    // 初始化下拉项，并用 peerconfig 默认值选中。
    void fillCombos();
    // 绑定控件与串口 / UDP 信号。
    void wireSignals();
    // 向运行记录追加一行（含时间戳）。
    void appendLog(const QString& line);
    // 更新状态栏文案。
    void setStatus(const QString& text);
    // 按通道是否已打开启用 / 禁用相关控件。
    void setOpened(bool opened);
    // 根据 COM 序号刷新显示名（序号+1）。
    void updateComEcho();
    // 当前是否为 UDP 模式。
    bool isUdpMode() const;
    // 界面当前三思段长（5/6/8）。
    int currentSegSize() const;
    // 界面当前波特率。
    int currentBaud() const;
    // 界面当前数据位。
    int currentDataBits() const;
    // 界面当前校验字母（N/E/O/S/M）。
    QChar currentParity() const;
    // 界面当前停止位（1/1.5/2）。
    QString currentStopBits() const;
    // 处理一帧串口数据：控制指令则执行，否则记为数据帧。
    void handleSerialFrame(const QByteArray& frame);
    // 执行已识别的控制指令；「开始」且勾选自动回测时发送测量值。
    void executeCommand(WireCodec::SerialCommand cmd, const QString& sourceTag);
    // 经当前通道发送原始字节并记录日志。
    bool sendRaw(const QByteArray& bytes, const QString& kind, const QString& detail);
    // 本端组包并发送控制指令（附加双向能力，非主路径）。
    bool sendOutboundCommand(WireCodec::SerialCommand cmd);
    // 组包并发送界面上的力 / 位移 / 应变。
    bool sendMeasurePayload();
    // 解析界面测量输入；失败时写入 err。
    bool parseMeasureFields(double* force, double* move, double* strain, QString* err) const;
    // 按下拉项的 userData 选中项；找不到则选第一项。
    static void selectComboByData(QComboBox* combo, const QVariant& data);

    Ui::MainWindow ui;
    PeerConfig m_cfg;           // 启动时自 ini 加载的默认参数
    QSerialPort m_serial;       // 串口通道
    QUdpSocket m_udp;           // UDP 通道
    QByteArray m_serialRxBuffer; // 串口未成帧残留
    bool m_opened = false;      // 通道是否已打开
};
