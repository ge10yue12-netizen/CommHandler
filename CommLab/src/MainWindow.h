// MainWindow.h — CommLab 软件侧：发指令、收试验机串口测量
#pragma once

#include "NetConfig.h"
#include "ProtoGuide.h"
#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QVariantMap>
#include <QVector>

class CommController;

// 软件侧 CommHandler 联调主窗口
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 初始化界面并挂接 CommController 信号
    explicit MainWindow(QWidget* parent = nullptr);
    // 断开库连接
    ~MainWindow() override;

private slots:
    // 连接：串口测量面 + UDP 指令面（或纯网口）
    void onConnectClicked();
    // 断开
    void onDisconnectClicked();
    // 四指令：调库发送
    void sendCmd(ProtoGuide::CommCmd cmd);
    // 库 emitNewData：试验机经串口发来的测量
    void onSafeData(QVector<double> values, int type);
    // 库 emitEventMsg
    void onEvent(int ctrlCmd, int viewId, int msg);
    // 库 emitEventMsgAndData
    void onEventWithData(int ctrlCmd, int viewId, int msg, QVariantMap extra);
    // 库 emitNewConn
    void onPeerConnected(int iType, const QString& ip, int port);
    // 库 emitClientDisConn
    void onPeerDisconnected();

private:
    // 生成连接摘要
    QString connSummary() const;
    // 追加日志
    void log(const QString& line);
    // 写串口参数
    void applySerialParams();
    // 写 UDP JSON 指令面参数
    void applyCmdNetParams();
    // 写纯网口数据面参数
    void applyUdpDataParams();
    // 刷新控件
    void syncUi();
    // 数据面是否网口
    bool udp() const { return ui.cmbCommType->currentIndex() == 0; }
    // 串口协议号
    int serialProto() const { return ui.cmbSerialProto->currentData().toInt(); }
    // 网口协议号（纯网口模式）
    int netProto() const { return ui.cmbNetProto->currentData().toInt(); }

    Ui::MainWindow ui;
    CommController* m_ctrl = nullptr;
    NetConfig m_net;
    bool m_ready = false;
    bool m_serialUp = false;
    bool m_cmdNetUp = false;
};
