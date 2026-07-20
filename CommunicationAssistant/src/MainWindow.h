#pragma once

#include "CaptureManager.h"
#include "LegacySession.h"
#include "NativeSession.h"
#include "SendScheduler.h"
#include "SessionManager.h"

#include "ui_MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
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
 * @brief 调试助手主窗：侧栏连接；主区数据/日志；底部发送工作台（单条+列表）。
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
    void onClearLogClicked();
    void onRecordReceived(const ca::CommRecord& record);
    void onSessionStateChanged(ca::SessionState state);
    void onCommError(const ca::CommError& error);
    void onChannelsChanged();
    void onDisconnectClientClicked();
    void onSchedStartClicked();
    void onSchedPauseClicked();
    void onSchedResumeClicked();
    void onSchedStopClicked();
    void onSchedTaskStopped(const QUuid& taskId, const QString& reason);
    void onSchedTaskFailed(const QUuid& taskId, const QString& code, const QString& message);
    void onCaptureFailed(const QUuid& sessionId, const QString& code, const QString& message);
    void onCaptureStarted(const QUuid& sessionId, const QString& filePath);
    void onLegacyUnresponsive(const QString& message);
    // 发送列表：增删、导入导出、发送选中行
    void onSendListAddRow();
    void onSendListRemoveRows();
    void onSendListImport();
    void onSendListExport();
    void onSendListSendSelected();

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
    void appendLog(const QString& line, const QString& cssClass = QString());
    void appendData(const QString& line);
    void syncUi();
    void updateCounters();
    void refreshClientCombo();
    void refreshLegacyCapabilityTips();
    void updateSendPlaceholders();
    bool isLegacyMode() const;
    // 接收显示是否为 HEX（否则文本）
    bool preferHexDisplay() const;
    // Native 发送是否按 HEX 解析（否则文本）
    bool preferHexSend() const;
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

    // 发送列表表格读写
    void ensureSendListHeaders();
    void addSendListRow(bool enabled, const QString& name, const QString& payload, int intervalMs,
                        int format);
    bool collectEnabledListPayloads(QVector<QByteArray>* outPayloads, QVector<int>* outFormats,
                                    QString* error) const;
    bool importSendListFile(const QString& path, QString* error);
    bool exportSendListFile(const QString& path, QString* error) const;
    static QString formatName(int format);
    static int formatFromName(const QString& name);

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
    QCheckBox* legacyMockCheck_ = nullptr;
    QLabel* legacyCapTipLabel_ = nullptr;

    QLabel* clientSectionLabel_ = nullptr;
    QComboBox* clientCombo_ = nullptr;
    QPushButton* btnDisconnectClient_ = nullptr;

    QPushButton* btnOpen_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QCheckBox* hexDisplayCheck_ = nullptr;
    QCheckBox* captureEnableCheck_ = nullptr;
    QPushButton* btnClear_ = nullptr;

    // 发送工作台
    QTabWidget* sendTabs_ = nullptr;
    QCheckBox* hexSendCheck_ = nullptr;
    QComboBox* legacySendModeCombo_ = nullptr;
    QPlainTextEdit* sendEdit_ = nullptr;
    QPushButton* btnSend_ = nullptr;

    QTableWidget* sendListTable_ = nullptr;
    QPushButton* btnListAdd_ = nullptr;
    QPushButton* btnListRemove_ = nullptr;
    QPushButton* btnListImport_ = nullptr;
    QPushButton* btnListExport_ = nullptr;
    QPushButton* btnListSendSelected_ = nullptr;

    QComboBox* schedModeCombo_ = nullptr;
    QSpinBox* schedIntervalSpin_ = nullptr;
    QSpinBox* schedCountSpin_ = nullptr;
    QPushButton* btnSchedStart_ = nullptr;
    QPushButton* btnSchedPause_ = nullptr;
    QPushButton* btnSchedResume_ = nullptr;
    QPushButton* btnSchedStop_ = nullptr;

    QPlainTextEdit* dataView_ = nullptr;
    QPlainTextEdit* log_ = nullptr;

    QLabel* txCountLabel_ = nullptr;
    QLabel* rxCountLabel_ = nullptr;
    QPushButton* btnResetCount_ = nullptr;

    ca::SessionState lastSessionState_ = ca::SessionState::Created;
    quint64 txBytes_ = 0;
    quint64 rxBytes_ = 0;
    quint64 rxChunks_ = 0;
};
