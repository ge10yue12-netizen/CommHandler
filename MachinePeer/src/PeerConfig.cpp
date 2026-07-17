// PeerConfig.cpp — 从 peerconfig.ini 加载试验机侧默认参数
#include "PeerConfig.h"
#include "IniUtil.h"

#include <QFileInfo>
#include <QSettings>

// 首个存在的 ini 路径生效；[network]/[serial] 分段读取
PeerConfig LoadPeerConfig()
{
    PeerConfig cfg;
    for (const QString& path : iniPaths(QStringLiteral("peerconfig.ini"))) {
        if (!QFileInfo::exists(path))
            continue;
        QSettings ini(path, QSettings::IniFormat);
        ini.setIniCodec("UTF-8");
        ini.beginGroup(QStringLiteral("network"));
        const QString lip = ini.value(QStringLiteral("bind_ip"), cfg.localIp).toString().trimmed();
        const QString dip = ini.value(QStringLiteral("peer_ip"), cfg.destIp).toString().trimmed();
        if (!lip.isEmpty())
            cfg.localIp = lip;
        if (!dip.isEmpty())
            cfg.destIp = dip;
        cfg.localPort = clampPort(ini.value(QStringLiteral("bind_port"), cfg.localPort).toInt(), cfg.localPort);
        cfg.destPort = clampPort(ini.value(QStringLiteral("peer_port"), cfg.destPort).toInt(), cfg.destPort);
        ini.endGroup();
        ini.beginGroup(QStringLiteral("serial"));
        cfg.comPort = ini.value(QStringLiteral("port_index"), cfg.comPort).toInt();
        ini.endGroup();
        return cfg;
    }
    return cfg;
}
