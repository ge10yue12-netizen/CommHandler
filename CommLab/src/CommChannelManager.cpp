// CommChannelManager.cpp — CommLab 串口/网口初始化流程实现
#include "CommChannelManager.h"

#include "NetworkProtocol.h"
#include "SerialProtocol.h"

#include <exception>

// 绑定唯一 CommHandler 实例
CommChannelManager::CommChannelManager(CommHandler* handler)
    : m_handler(handler)
{
}

// 切换 CommHandler 当前模块并保证旧连接已关闭
void CommChannelManager::prepare(CommType type)
{
    close();
    m_type = type;
    if (m_handler)
        m_handler->SetCommType(type);
}

// 关闭旧通道、初始化串口参数并连接
bool CommChannelManager::openSerial(const SerialChannelSettings& settings, QString* error)
{
    if (!m_handler) {
        if (error)
            *error = QStringLiteral("CommHandler 未初始化");
        return false;
    }

    try {
        prepare(SERIAL);
        SerialProtocol::configure(m_handler, settings.portIndex, settings.baudIndex,
                                  settings.dataBitsIndex, settings.parityIndex,
                                  settings.stopBitsIndex, settings.protocolIndex);
        m_open = m_handler->Connect();
    } catch (const std::exception& exception) {
        if (error)
            *error = QString::fromLocal8Bit(exception.what());
        m_open = false;
    }

    if (!m_open && error && error->isEmpty())
        *error = QStringLiteral("串口连接失败");
    return m_open;
}

// 关闭旧通道、初始化网口参数并连接
bool CommChannelManager::openNetwork(const NetworkChannelSettings& settings, QString* error)
{
    if (!m_handler) {
        if (error)
            *error = QStringLiteral("CommHandler 未初始化");
        return false;
    }

    try {
        prepare(NETWORK);
        NetworkProtocol::configure(m_handler, settings.transferType, settings.model,
                                   settings.localIp, settings.localPort,
                                   settings.destinationIp, settings.destinationPort,
                                   settings.protocolIndex);
        m_open = m_handler->Connect();
    } catch (const std::exception& exception) {
        if (error)
            *error = QString::fromLocal8Bit(exception.what());
        m_open = false;
    }

    if (!m_open && error && error->isEmpty())
        *error = QStringLiteral("网口连接失败");
    return m_open;
}

// 断开当前通道并清除连接状态
void CommChannelManager::close()
{
    if (m_handler) {
        m_handler->SetCommType(m_type);
        m_handler->Disconnect(0);
    }
    m_open = false;
}

// 设置当前通道业务回发门控
void CommChannelManager::setReplyEnabled(bool enabled)
{
    if (!m_handler)
        return;
    m_handler->SetCommType(m_type);
    m_handler->setParameter(QStringLiteral("bInquireSendFlag"), enabled);
}

// 设置网口在线采集状态
void CommChannelManager::setNetworkCollecting(bool collecting)
{
    if (!m_handler || m_type != NETWORK)
        return;
    m_handler->SetCommType(NETWORK);
    m_handler->setParameter(QStringLiteral("bOnlineCollect"), collecting);
}
