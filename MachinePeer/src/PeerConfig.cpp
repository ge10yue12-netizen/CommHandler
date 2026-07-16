#include "PeerConfig.h"
#include "IniUtil.h"

#include <QFileInfo>
#include <QSettings>

// 加载 peerconfig.ini，找不到则用默认值。
PeerConfig LoadPeerConfig()
{
    PeerConfig cfg;
    for (const QString& path : iniPaths(QStringLiteral("peerconfig.ini"))) {
        if (!QFileInfo::exists(path)) continue;
        QSettings ini(path, QSettings::IniFormat);
        ini.setIniCodec("UTF-8");
        ini.beginGroup(QStringLiteral("network"));
        const QString lip = ini.value(QStringLiteral("bind_ip"), cfg.localIp).toString().trimmed();
        const QString dip = ini.value(QStringLiteral("peer_ip"), cfg.destIp).toString().trimmed();
        if (!lip.isEmpty()) cfg.localIp = lip;
        if (!dip.isEmpty()) cfg.destIp = dip;
        cfg.localPort = clampPort(ini.value(QStringLiteral("bind_port"), cfg.localPort).toInt(), cfg.localPort);
        cfg.destPort = clampPort(ini.value(QStringLiteral("peer_port"), cfg.destPort).toInt(), cfg.destPort);
        cfg.transferType = ini.value(QStringLiteral("transfer_type"), cfg.transferType).toInt();
        cfg.model = ini.value(QStringLiteral("model"), cfg.model).toInt();
        ini.endGroup();
        ini.beginGroup(QStringLiteral("serial"));
        cfg.comPort = ini.value(QStringLiteral("port_index"), cfg.comPort).toInt();
        ini.endGroup();
        return cfg;
    }
    return cfg;
}
