// NetConfig.cpp — 从 netconfig.ini 加载软件侧网口参数
#include "NetConfig.h"
#include "IniUtil.h"

#include <QFileInfo>
#include <QSettings>

// 首个存在的 ini 路径生效；键名对齐 [network] 段
NetConfig LoadNetConfig()
{
    NetConfig cfg;
    for (const QString& path : iniPaths(QStringLiteral("netconfig.ini"))) {
        if (!QFileInfo::exists(path))
            continue;
        QSettings ini(path, QSettings::IniFormat);
        ini.setIniCodec("UTF-8");
        ini.beginGroup(QStringLiteral("network"));
        const QString lip = ini.value(QStringLiteral("local_ip"), cfg.localIp).toString().trimmed();
        const QString dip = ini.value(QStringLiteral("dest_ip"), cfg.destIp).toString().trimmed();
        if (!lip.isEmpty())
            cfg.localIp = lip;
        if (!dip.isEmpty())
            cfg.destIp = dip;
        cfg.localPort = clampPort(ini.value(QStringLiteral("local_port"), cfg.localPort).toInt(), cfg.localPort);
        cfg.destPort = clampPort(ini.value(QStringLiteral("dest_port"), cfg.destPort).toInt(), cfg.destPort);
        cfg.transferType = ini.value(QStringLiteral("transfer_type"), cfg.transferType).toInt();
        cfg.model = ini.value(QStringLiteral("model"), cfg.model).toInt();
        ini.endGroup();
        return cfg;
    }
    return cfg;
}
