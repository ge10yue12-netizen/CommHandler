// CommChannelManager.h — CommLab 串口/网口连接初始化与模式切换统一入口
#pragma once

#include "CommHandler.h"

#include <QString>

// 串口连接所需的完整参数快照
struct SerialChannelSettings
{
    int portIndex = 0;
    int baudIndex = 1;
    int dataBitsIndex = 3;
    int parityIndex = 0;
    int stopBitsIndex = 0;
    int protocolIndex = 0;
};

// 网口连接所需的完整参数快照
struct NetworkChannelSettings
{
    int transferType = 0;
    int model = 0;
    QString localIp;
    int localPort = 0;
    QString destinationIp;
    int destinationPort = 0;
    int protocolIndex = 0;
};

// 统一管理 CommHandler 当前通道，确保串口和网口互斥
class CommChannelManager
{
public:
    // 绑定唯一 CommHandler 实例
    explicit CommChannelManager(CommHandler* handler);

    // 关闭旧通道、初始化串口参数并连接
    bool openSerial(const SerialChannelSettings& settings, QString* error);

    // 关闭旧通道、初始化网口参数并连接
    bool openNetwork(const NetworkChannelSettings& settings, QString* error);

    // 断开当前通道并清除连接状态
    void close();

    // 设置当前通道业务回发门控
    void setReplyEnabled(bool enabled);

    // 设置网口在线采集状态
    void setNetworkCollecting(bool collecting);

    // 返回当前是否已经连接
    bool isOpen() const { return m_open; }

    // 返回当前是否为网口
    bool isNetwork() const { return m_type == NETWORK; }

private:
    // 切换 CommHandler 当前模块并保证旧连接已关闭
    void prepare(CommType type);

    CommHandler* m_handler = nullptr;
    CommType m_type = NETWORK;
    bool m_open = false;
};
