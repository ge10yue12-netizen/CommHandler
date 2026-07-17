// MainWindow.h — CommLab 软件主窗口：试验机指令/测数入站，处理后经库 SendData 回发
#pragma once

#include "NetConfig.h"
#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QVariantMap>
#include <QVector>

class CommController;

// 软件侧界面与业务编排；同一时刻仅连接串口或网口之一
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 初始化界面、填充协议参数并挂接 CommController 信号
    explicit MainWindow(QWidget* parent = nullptr);
    // 断开当前库连接后析构
    ~MainWindow() override;

private slots:
    // 按当前通道建立 CommHandler 连接
    void onConnectClicked();
    // 清除计算状态并断开库连接
    void onDisconnectClicked();
    // 库 emitNewData 入站测数；处理后按能力调用 SendData 回发
    void onSafeData(QVector<double> values, int type);
    // 库 emitEventMsg 入站指令（开始/停止等）
    void onEvent(int ctrlCmd, int viewId, int msg);
    // 带附加字段的库事件
    void onEventWithData(int ctrlCmd, int viewId, int msg, QVariantMap extra);

private:
    // 收发区追加带时戳日志
    void log(const QString& line);
    // 按连接状态与通道类型刷新控件
    void syncUi();
    // true：当前为网口（与串口互斥）
    bool isNetwork() const { return ui.cmbCommType->currentIndex() == 0; }
    // 串口协议索引 iProtocolType
    int serialProto() const { return ui.cmbSerialProto->currentData().toInt(); }
    // 网口协议索引 iProtoType
    int netProto() const { return ui.cmbNetProto->currentData().toInt(); }
    // 处理缓存测数并 SendData；无测力或未开始则不调用 SendData
    void processAndReply();

    Ui::MainWindow ui;
    // 库回调封装
    CommController* m_ctrl = nullptr;
    // 网口 ini 默认参数
    NetConfig m_net;
    // true：已 Connect
    bool m_ready = false;
    // true：已开始（可回发）
    bool m_running = false;
    // 最近入站力
    double m_force = 0.0;
    // 最近入站温（仅 m_hasTemp 为 true 时有效，禁止用缺省值回发）
    double m_temp = 0.0;
    // true：本轮已收到可用于回发的力
    bool m_hasForce = false;
    // true：本轮已收到真实温度通道（非状态位、非缺省）
    bool m_hasTemp = false;
};
