// MainWindow.h — MachinePeer 试验机：发指令/测数，并按库出站格式解析软件回包
#pragma once

#include "PeerChannelManager.h"
#include "PeerConfig.h"
#include "ui_MainWindow.h"

#include <QByteArray>
#include <QMainWindow>

// 试验机角色界面；串口与网口互斥；收包解析算好的业务量
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 初始化界面并挂接串口/UDP 收包槽
    explicit MainWindow(QWidget* parent = nullptr);
    // 关闭串口与 UDP 后析构
    ~MainWindow() override;

private slots:
    // 按当前通道打开串口或绑定 UDP
    void onOpenClicked();
    // 关闭当前通道
    void onCloseClicked();
    // 仅发送测数帧（力/温），不发送指令
    void onSendMeasureClicked();
    // 仅发送开始指令
    void onCmdStart();
    // 仅发送停止指令
    void onCmdStop();
    // 仅发送存图指令（串口三思 / 网口触发存图）
    void onCmdSave();
    // 仅发送清零指令（串口三思）
    void onCmdZero();
    // 串口收字节：记 HEX，并按协议解析软件回传
    void onSerialBytesReceived(QByteArray bytes);
    // UDP 收报文：记原文/HEX，并按协议解析软件回传
    void onNetworkDatagramReceived(QByteArray datagram);
    // 通道切换时刷新协议列表与面板
    void onChannelChanged(int);

private:
    // 收发区追加带时戳日志
    void log(const QString& line);
    // 刷新面板可见性与按库能力使能按钮
    void syncUi();
    // 按网口/串口重填协议下拉；itemData 为协议索引
    void refillProtoCombo();
    // true：当前通道为网口；false：串口
    bool isNetwork() const { return ui.cmbCommType->currentIndex() == 0; }
    // 当前协议索引（对齐库 iProtoType / iProtocolType）
    int proto() const { return ui.cmbProto->currentData().toInt(); }
    // 关闭串口与 UDP，并清空粘包缓冲
    void closeAll();
    // 解析力/温编辑框；非法时使用缺省 1.0 / 25.0
    bool readForceTemp(double* force, double* temp) const;
    // 按协议组指令帧并写出；cmd：0开始 1停止 2存图 3清零
    void sendCmd(int cmd);
    // 解析软件回包：ACK / 仅力 / 力+温 分记日志
    void handleSoftReply(const QByteArray& raw);
    // 从串口面板生成完整初始化参数
    PeerSerialSettings serialSettings() const;
    // 从网口面板生成完整初始化参数
    PeerNetworkSettings networkSettings() const;

    Ui::MainWindow ui;
    // 串口/网口初始化、切换和原始收发统一入口
    PeerChannelManager m_channels;
    // 默认网络/串口参数（可由 ini 覆盖）
    PeerConfig m_cfg;
    // true：通道已打开，可发指令/测数
    bool m_ready = false;
    // 串口粘包缓冲（三思软件回包以 0x0D 结尾）
    QByteArray m_serialRxBuf;
};
