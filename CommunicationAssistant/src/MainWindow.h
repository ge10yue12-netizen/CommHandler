#pragma once

#include "CaptureManager.h"
#include "LegacySession.h"
#include "NativeSession.h"
#include "SendScheduler.h"
#include "SessionManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUuid>
#include <QWidget>

/**
 * @brief 调试助手主窗：Native（串口/TCP/UDP）与 Legacy DLL；能力表驱动 Legacy 控件。
 * @thread UI 线程。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onWorkModeChanged(int index);
    void onTransportChanged(int index);
    void onLegacyCommOrProtocolChanged();
    void onOpenClicked();
    void onCloseClicked();
    void onSendClicked();
    void onClearLogClicked();
    void onRecordReceived(const ca::CommRecord& record);
    void onSessionStateChanged(ca::SessionState state);
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

private:
    void applyStyle();
    void buildUi();
    void appendLog(const QString& line, const QString& cssClass = QString());
    void syncUi();
    void updateCounters();
    void refreshClientCombo();
    void refreshLegacyCapabilityTips();
    bool isLegacyMode() const;
    ca::ICommSession* activeSession();
    const ca::ICommSession* activeSession() const;
    ca::SessionConfig buildConfig() const;
    QByteArray buildPayload(QString* error) const;
    QVector<QByteArray> buildSchedulePayloads(QString* error) const;
    void fillChannelOnRequest(ca::SendRequest* req) const;
    ca::Result buildLegacySendRequest(ca::SendRequest* req, QString* error) const;

    ca::SessionManager manager_;
    ca::NativeSession session_;
    ca::LegacySession legacySession_;
    ca::SendScheduler scheduler_;
    ca::CaptureManager capture_;
    QUuid activeSchedTaskId_;

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
    QCheckBox* hexSendCheck_ = nullptr;
    QCheckBox* captureEnableCheck_ = nullptr;
    QPushButton* btnClear_ = nullptr;

    QComboBox* schedModeCombo_ = nullptr;
    QSpinBox* schedIntervalSpin_ = nullptr;
    QSpinBox* schedCountSpin_ = nullptr;
    QPushButton* btnSchedStart_ = nullptr;
    QPushButton* btnSchedPause_ = nullptr;
    QPushButton* btnSchedResume_ = nullptr;
    QPushButton* btnSchedStop_ = nullptr;

    QPlainTextEdit* log_ = nullptr;
    QPlainTextEdit* sendEdit_ = nullptr;
    QPushButton* btnSend_ = nullptr;

    QLabel* txCountLabel_ = nullptr;
    QLabel* rxCountLabel_ = nullptr;
    QPushButton* btnResetCount_ = nullptr;

    quint64 txBytes_ = 0;
    quint64 rxBytes_ = 0;
    quint64 rxChunks_ = 0;
};
