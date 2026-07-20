#pragma once

#include "ResourceClaim.h"
#include "Result.h"
#include "SessionConfig.h"

namespace ca {

/**
 * @brief 从 SessionConfig 推导 ResourceClaim；失败时 outClaim 内容未定义。
 *
 * @thread 无锁纯函数，任意线程可调用。
 * UI 必须通过 configure→claimFor 链路获得 claim，禁止绕过配置发明占用。
 */
inline Result claimFor(const SessionConfig& config, ResourceClaim* outClaim)
{
    if (!outClaim)
        return Result::fail(QStringLiteral("invalid_argument"), QStringLiteral("outClaim 为空指针"));

    if (config.mode == WorkMode::LegacyDll) {
        ResourceClaim claim;
        if (config.transport.legacy.commType == 1) {
            claim.transport = TransportKind::Serial;
            claim.serialPort = normalizeSerialPortName(config.transport.legacy.portName);
            claim.exclusive = true;
            if (claim.serialPort.isEmpty())
                return Result::fail(QStringLiteral("invalid_serial"), QStringLiteral("串口名为空"));
        } else {
            claim.transport = TransportKind::TcpClient;
            claim.remoteAddress = config.transport.legacy.remoteAddress.trimmed();
            claim.remotePort = config.transport.legacy.remotePort;
            claim.localAddress = config.transport.legacy.localAddress.trimmed();
            claim.localPort = config.transport.legacy.localPort;
            claim.exclusive = false;
            if (claim.remoteAddress.isEmpty() || claim.remotePort == 0)
                return Result::fail(QStringLiteral("invalid_legacy_net"), QStringLiteral("需要远端地址与端口"));
        }
        *outClaim = claim;
        return Result::success();
    }

    ResourceClaim claim;
    claim.transport = config.transport.kind;

    switch (config.transport.kind) {
    case TransportKind::Serial:
        claim.serialPort = normalizeSerialPortName(config.transport.serial.portName);
        if (claim.serialPort.isEmpty())
            return Result::fail(QStringLiteral("invalid_serial"), QStringLiteral("串口名为空"));
        claim.exclusive = true;
        break;
    case TransportKind::TcpClient:
        claim.remoteAddress = config.transport.tcpClient.remoteAddress.trimmed();
        claim.remotePort = config.transport.tcpClient.remotePort;
        claim.localAddress = config.transport.tcpClient.localAddress.trimmed();
        claim.localPort = config.transport.tcpClient.localPort;
        claim.exclusive = false;
        if (claim.remoteAddress.isEmpty() || claim.remotePort == 0)
            return Result::fail(QStringLiteral("invalid_tcp_client"), QStringLiteral("需要远端地址与端口"));
        break;
    case TransportKind::TcpServer:
        claim.localAddress = config.transport.tcpServer.localAddress.trimmed();
        claim.localPort = config.transport.tcpServer.localPort;
        claim.addressReuse = config.transport.tcpServer.addressReuse;
        claim.exclusive = true;
        if (claim.localPort == 0)
            return Result::fail(QStringLiteral("invalid_tcp_server"), QStringLiteral("需要本地端口"));
        break;
    case TransportKind::Udp:
        claim.localAddress = config.transport.udp.localAddress.trimmed();
        claim.localPort = config.transport.udp.localPort;
        claim.remoteAddress = config.transport.udp.remoteAddress.trimmed();
        claim.remotePort = config.transport.udp.remotePort;
        claim.addressReuse = config.transport.udp.addressReuse;
        claim.exclusive = true;
        if (claim.localPort == 0)
            return Result::fail(QStringLiteral("invalid_udp"), QStringLiteral("需要本地端口"));
        break;
    }

    *outClaim = claim;
    return Result::success();
}

} // namespace ca