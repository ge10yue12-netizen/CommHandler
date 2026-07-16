// MainWindow.h — MachinePeer 试验机侧：收指令、经串口发测量
#pragma once

#include "PeerConfig.h"
#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QVariantMap>
#include <QVector>

class CommController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenClicked();
    void onCloseClicked();
    void onSafeData(QVector<double> values, int type);
    void onEvent(int ctrlCmd, int viewId, int msg);
    void onEventWithData(int ctrlCmd, int viewId, int msg, QVariantMap extra);
    void onPeerConnected(int iType, const QString& ip, int port);
    void onPeerDisconnected();

private:
    QString connSummary() const;
    void log(const QString& line);
    void applySerialParams();
    void applyCmdNetParams();
    void applyUdpParams();
    void syncUi();
    void sendMeasure();
    bool udp() const { return ui.cmbCommType->currentIndex() == 0; }
    int serialProto() const { return ui.cmbSerialProto->currentData().toInt(); }
    int proto() const
    {
        return udp() ? ui.cmbNetProto->currentData().toInt()
                       : ui.cmbSerialProto->currentData().toInt();
    }

    Ui::MainWindow ui;
    CommController* m_ctrl = nullptr;
    PeerConfig m_cfg;
    bool m_ready = false;
    bool m_serialUp = false;
    bool m_cmdNetUp = false;
};
