#pragma once

#include "CaptureManager.h"
#include "LegacySession.h"
#include "NativeSession.h"
#include "SendScheduler.h"
#include "SessionManager.h"
#include "WaveformGenerator.h"

#include "ui_MainWindow.h"

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QUuid>
#include <QWidget>

#include "Result.h"

/**
 * @brief 调试助手主窗：侧栏连接；主区数据/日志；底部发送工作台（单条+列表+波形）。
 * @thread UI 线程。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 加载 .ui、填充动态项并挂接会话信号
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onWorkModeChanged(int index);
    void onTransportChanged(int index);
    void onLegacyCommTypeChanged();
    void onLegacyProtocolChanged();
    void onOpenClicked();
    void onCloseClicked();
    void onSendClicked();
    void onClearDataClicked();
    void onClearLogClicked();
    void onRecordReceived(const ca::CommRecord& record);
    void onSessionStateChanged(ca::SessionState state);
    void onCommError(const ca::CommError& error);
    void onChannelsChanged();
    void onDisconnectClientClicked();
    void onSchedStartClicked();
    void onWaveSchedStartClicked();
    void onSchedPauseClicked();
    void onSchedResumeClicked();
    void onSchedStopClicked();
    void onSchedTaskStopped(const QUuid& taskId, const QString& reason);
    void onSchedTaskFailed(const QUuid& taskId, const QString& code, const QString& message);
    void onCaptureFailed(const QUuid& sessionId, const QString& code, const QString& message);
    void onCaptureStarted(const QUuid& sessionId, const QString& filePath);
    void onLegacyUnresponsive(const QString& message);
    // 发送列表：增删清空、导入导出、右键菜单
    void onSendListAddRow();
    void onSendListRemoveRows();
    void onSendListClearAll();
    void onSendListImport();
    void onSendListExport();
    void onSendListContextMenu(const QPoint& pos);
private:
    // 列表行「格式」列取值
    enum SendPayloadFormat {
        FormatAuto = 0,
        FormatValues = 1,
        FormatText = 2,
        FormatHex = 3
    };

    void applyStyle();
    // 自 MainWindow.ui 加载壳层，再填充端口/协议等动态项并连接信号
    void buildUi();
    // 将 ui_ 控件指针绑定到既有成员，供业务逻辑沿用
    void bindUiWidgets();
    // 填充组合框、默认值与表格头等运行时内容
    void populateDynamicUi();
    // 动态创建「波形发送」页（不依赖 Designer 手改易碎布局）
    void buildWaveformTab();
    // 原生接收/发送辅助选项（HEX+ASCII、发送后清空）
    void buildNativeAssistOptions();
    void appendLog(const QString& line, const QString& cssClass = QString());
    void appendData(const QString& line);
    void syncUi();
    void updateCounters();
    void refreshClientCombo();
    void refreshLegacyCapabilityTips();
    void updateSendPlaceholders();
    // 按调度模式显隐间隔/次数控件
    void updateSchedControlsVisibility();
    // 十六进制发送勾选优先：禁用以太网 Legacy 模式下拉，避免双源冲突
    void updateSendFormatMutex();
    // 原生通道：勾选/取消「十六进制发送」时，单条编辑框文本⇄HEX 互转（仅 UI，不改传输模块）
    void syncNativeSendEditForHexMode(bool hexOn);
    // 已连接时修改工作模式/连接参数：关闭后按新配置自动重开
    void requestApplyConnectedConfig(const QString& reason);
    // 按当前 UI 配置打开会话（供首次打开与热重连共用）
    void openWithCurrentUiConfig();
    bool isLegacyMode() const;
    // 仅接收展示：勾选则 RX 字节以 HEX 显示，否则原样文本（不影响发送）
    bool preferHexDisplay() const;
    // 接收展示附加可读 ASCII（常见调试助手双栏风格）
    bool preferHexAsciiDisplay() const;
    // 仅发送：勾选则按 HEX 解析发出，且 TX 记录按 HEX 展示（不影响接收）
    bool preferHexSend() const;
    // 单条发送成功后是否清空编辑框
    bool clearAfterSend() const;
    // 当前 Legacy 协议显示名（如「串口 0 三思」）
    QString currentLegacyProtocolLabel() const;
    // 按当前协议推荐波形通道数
    int recommendedWaveChannels() const;
    ca::ICommSession* activeSession();
    const ca::ICommSession* activeSession() const;
    ca::SessionConfig buildConfig() const;
    QByteArray buildPayload(QString* error) const;
    QVector<QByteArray> buildSchedulePayloads(QString* error) const;
    void fillChannelOnRequest(ca::SendRequest* req) const;
    // 按指定文本与格式构造 Legacy 发送请求
    ca::Result buildLegacySendRequestFrom(const QString& text, int format, ca::SendRequest* req,
                                          QString* error) const;
    ca::Result buildLegacySendRequest(ca::SendRequest* req, QString* error) const;
    QString formatRecordForDisplay(const ca::CommRecord& record) const;
    static QString sessionStateTip(ca::SessionState state);
    static QString bytesAsPrintableAscii(const QByteArray& bytes);

    // 发送列表表格读写（间隔统一使用下方默认间隔）
    void ensureSendListHeaders();
    void addSendListRow(bool enabled, const QString& name, const QString& payload, int format);
    bool collectEnabledListPayloads(QVector<QByteArray>* outPayloads, QVector<int>* outFormats,
                                    QString* error) const;
    bool importSendListFile(const QString& path, QString* error);
    bool exportSendListFile(const QString& path, QString* error) const;
    static QString formatName(int format);
    static int formatFromName(const QString& name);
    // 行是否勾选启用
    bool isSendListRowEnabled(int row) const;
    // 收集勾选行索引（升序）
    QList<int> enabledSendListRows() const;
    ca::SessionManager manager_;
    ca::NativeSession session_;
    ca::LegacySession legacySession_;
    ca::SendScheduler scheduler_;
    ca::CaptureManager capture_;
    QUuid activeSchedTaskId_;

    // Designer 生成的界面实例
    Ui::MainWindow ui_;

    QComboBox* workModeCombo_ = nullptr;
    QComboBox* transportCombo_ = nullptr;
    QStackedWidget* paramStack_ = nullptr;

    QComboBox* portCombo_ = nullptr;
    QComboBox* baudCombo_ = nullptr;
    QComboBox* dataBitsCombo_ = nullptr;
    QComboBox* parityCombo_ = nullptr;
    QComboBox* stopBitsCombo_ = nullptr;

    QLineEdit* tcpHostEdit_ = nullptr;
    QSpinBox* tcpPortSpin_ = nullptr;

    QLineEdit* tcpServerAddrEdit_ = nullptr;
    QSpinBox* tcpServerPortSpin_ = nullptr;

    QLineEdit* udpLocalAddrEdit_ = nullptr;
    QSpinBox* udpLocalPortSpin_ = nullptr;
    QLineEdit* udpRemoteAddrEdit_ = nullptr;
    QSpinBox* udpRemotePortSpin_ = nullptr;

    QComboBox* legacyCommTypeCombo_ = nullptr;
    QComboBox* legacyProtocolCombo_ = nullptr;
    QStackedWidget* legacyEndpointStack_ = nullptr;
    QComboBox* legacyTransferCombo_ = nullptr;
    QComboBox* legacyModelCombo_ = nullptr;
    QLineEdit* legacyLocalAddrEdit_ = nullptr;
    QSpinBox* legacyLocalPortSpin_ = nullptr;
    QLineEdit* legacyRemoteAddrEdit_ = nullptr;
    QSpinBox* legacyRemotePortSpin_ = nullptr;
    QComboBox* legacyPortCombo_ = nullptr;
    QComboBox* legacyBaudCombo_ = nullptr;
    QComboBox* legacyDataBitsCombo_ = nullptr;
    QComboBox* legacyParityCombo_ = nullptr;
    QComboBox* legacyStopBitsCombo_ = nullptr;
    QCheckBox* legacyMockCheck_ = nullptr;
    QLabel* legacyCapTipLabel_ = nullptr;

    QLabel* clientSectionLabel_ = nullptr;
    QComboBox* clientCombo_ = nullptr;
    QPushButton* btnDisconnectClient_ = nullptr;

    QPushButton* btnOpen_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QCheckBox* hexDisplayCheck_ = nullptr;
    QCheckBox* hexAsciiCheck_ = nullptr;
    QCheckBox* clearAfterSendCheck_ = nullptr;
    QCheckBox* captureEnableCheck_ = nullptr;
    QPushButton* btnClearData_ = nullptr;
    QPushButton* btnClearLog_ = nullptr;

    // 发送工作台
    QTabWidget* sendTabs_ = nullptr;
    QCheckBox* hexSendCheck_ = nullptr;
    QComboBox* legacySendModeCombo_ = nullptr;
    QPlainTextEdit* sendEdit_ = nullptr;
    QPushButton* btnSend_ = nullptr;

    QTableWidget* sendListTable_ = nullptr;
    QPushButton* btnListAdd_ = nullptr;
    QPushButton* btnListRemove_ = nullptr;
    QPushButton* btnListClearAll_ = nullptr;
    QPushButton* btnListImport_ = nullptr;
    QPushButton* btnListExport_ = nullptr;

    QComboBox* schedModeCombo_ = nullptr;
    QSpinBox* schedIntervalSpin_ = nullptr;
    QSpinBox* schedCountSpin_ = nullptr;
    QPushButton* btnSchedStart_ = nullptr;
    QPushButton* btnSchedPause_ = nullptr;
    QPushButton* btnSchedResume_ = nullptr;
    QPushButton* btnSchedStop_ = nullptr;

    // 波形页
    QDoubleSpinBox* waveAmpSpin_ = nullptr;
    QDoubleSpinBox* waveFreqSpin_ = nullptr;
    QDoubleSpinBox* wavePhaseSpin_ = nullptr;
    QDoubleSpinBox* waveOffsetSpin_ = nullptr;
    QDoubleSpinBox* waveNoiseSpin_ = nullptr;
    QSpinBox* waveChannelsSpin_ = nullptr;
    QSpinBox* waveSeedSpin_ = nullptr;
    QSpinBox* waveIntervalSpin_ = nullptr;
    QSpinBox* waveCountSpin_ = nullptr;
    QCheckBox* waveNativeHexCheck_ = nullptr;
    QCheckBox* waveInfiniteCheck_ = nullptr;
    QPushButton* btnWaveStart_ = nullptr;
    QPushButton* btnWavePause_ = nullptr;
    QPushButton* btnWaveResume_ = nullptr;
    QPushButton* btnWaveStop_ = nullptr;
    QLabel* waveHintLabel_ = nullptr;
    // 去掉数值框上下加减箭头
    static void polishPlainSpin(QAbstractSpinBox* box);

    QPlainTextEdit* dataView_ = nullptr;
    QPlainTextEdit* log_ = nullptr;

    QLabel* txCountLabel_ = nullptr;
    QLabel* rxCountLabel_ = nullptr;
    QPushButton* btnResetCount_ = nullptr;

    ca::SessionState lastSessionState_ = ca::SessionState::Created;
    // 连接中改配置：先异步关闭，Closed 后再按 UI 重开
    bool pendingReopenAfterConfig_ = false;
    quint64 txBytes_ = 0;
    quint64 rxBytes_ = 0;
    quint64 rxChunks_ = 0;
    qint64 lastRxMs_ = 0; // 0=从未收到；UTC epoch ms
};
