#pragma once

#include "NetConfig.h"
#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QUdpSocket>
#include <QVector>
#include <QtGlobal>

class CommController;
class CommHandler;

// CommLab 主窗口。布局由 MainWindow.ui 定义，可用 Qt Designer 编辑。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // 写入参数并 Connect。
    void onConnectClicked();
    // Disconnect(0)。
    void onDisconnectClicked();
    // UI：跑 UDP 三思接收自检（iProtoType=3）。
    void onSmokeUdpSansiClicked();
    // UI：跑 UDP JSON 发送与许可自检（iProtoType=0）。
    void onSmokeUdpSendClicked();
    // 解析发送框并调用 SendData(vector)。
    void onSendClicked();
    // 发出 3D0D 开始计算。
    void onCmdStartClicked();
    // 发出 3E0D 停止计算。
    void onCmdStopClicked();
    // 发出 6B0D 自动存图。
    void onCmdSaveClicked();
    // 发出 5A0D 清零。
    void onCmdZeroClicked();
    // 清空运行记录。
    void onClearLogClicked();
    // 将运行记录存为文本文件。
    void onSaveLogClicked();
    // 切换 UDP / 串口面板。
    void onCommTypeChanged(int index);
    // 网口协议变更时刷新发送区可用性。
    void onNetProtoChanged(int index);
    // 同步 bInquireSendFlag 到 DLL。
    void onAllowSendToggled(bool checked);

    // 显示已拷贝的测量回调。
    void onSafeData(QVector<double> values, int type);
    // 显示库事件（开始/停止/清零等）。
    void onEvent(int ctrlCmd, int viewId, int msg);
    // 显示新连接通知。
    void onConn(int iType, QString ip, int port);
    // 显示对端断开。
    void onClientGone();
    // 显示 CommController 侧日志。
    void onControllerLog(QString text);

private:
    // 初始化下拉选项与默认值。
    void fillCombos();
    // 绑定界面控件信号。
    void wireSignals();
    // 向运行记录追加一行（附带时间戳）。
    void appendLog(const QString& line);
    // 向状态栏写一行短提示。
    void setStatus(const QString& text);
    // 刷新收发/错误计数标签。
    void refreshCounters();
    // 根据串口序号刷新「当前端口」显示（序号+1=COMx）。
    void updateComEcho();
    // 按波特率/数据位/校验/停止位刷新线格式摘要。
    void updateLineFormatEcho();
    // 返回当前数据位参数（0→5，1→6，3→8）。
    int currentDataBitsParam() const;
    // 返回当前校验参数（0 无 … 4 标记）。
    int currentParityParam() const;
    // 返回当前停止位参数（0→1，1→1.5，2→2）。
    int currentStopBitsParam() const;
    // 三思组包段长：与 Connect 后 m_iDataBits（Data5/6/8）一致，取值为 5/6/8。
    int currentSansiSegSize() const;
    // 线格式摘要，例如「115200 8N1」。
    QString currentLineFormatText() const;
    // 将串口参数写入 DLL（连接前调用）。
    bool applySerialParams();
    // 将网口参数与 iProtoType 写入 DLL。
    bool applyUdpParams();
    // 按连接态启用/禁用操作区控件。
    void setControlsEnabled(bool connected);
    // 按协议能力刷新发送许可与发送按钮。
    void updateSendUiState();
    // UserRole+1==1 表示当前网口协议支持数值 SendData。
    bool currentNetProtoSupportsSend() const;
    // 串口三思发送后输出预期十六进制帧，供对端对照。
    void appendSerialHexPreview(const QVector<double>& values);
    // 返回可用 CommHandler；不可用时记录错误并返回 nullptr。
    CommHandler* requireHandler(const QString& actionTag);
    // 执行 CLI 同源自检：先断开本窗连接，再调用 SmokeRunner，最后恢复未连接界面。
    void runHeadlessSmoke(const QString& tag, int (*fn)(QString*));
    // 向试验机发出二字节控制指令；UDP 原始写出，串口临时释放库占用后旁路写出再重连。
    bool sendControlCommand(quint8 cmdByte, const QString& name);
    // 组装命令码 + 0x0D。
    static QByteArray packControlFrame(quint8 cmdByte);
    // 串口旁路：Disconnect → QSerialPort 写帧 → Connect。
    bool sendControlCommandViaSerialBypass(const QByteArray& frame, const QString& name);
    // UDP：向界面目的地址 writeDatagram 原始指令帧。
    bool sendControlCommandViaUdpRaw(const QByteArray& frame, const QString& name);
    // 已知事件码对应的线帧说明（收侧提示）。
    static QString serialEventHexHint(int msg);
    // 解析逗号分隔浮点数值；失败时写入 errOut。
    static QVector<double> parseValues(const QString& text, QString* errOut);

    // 单次发送数值个数上限。
    enum { kMaxSendValueCount = 32 };

    Ui::MainWindow ui;
    CommController* m_controller = nullptr;
    NetConfig m_netConfig;
    QUdpSocket m_cmdUdp; // 指令原始 UDP 写出（不依赖 SendData(QString)）

    int m_rxDataCount = 0;
    int m_rxEventCount = 0;
    int m_errorCount = 0;
    int m_txAttemptCount = 0;
};
