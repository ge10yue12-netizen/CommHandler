#include "NetConfig.h"

#include "CommHandler.h"
#include "UIDef.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>

namespace {

// 构造候选 ini 路径列表：当前目录 → 可执行文件目录。
QStringList candidatePaths()
{
    QStringList paths;
    const QString name = QStringLiteral("netconfig.ini");
    paths << QDir::current().absoluteFilePath(name);
    if (!QCoreApplication::applicationDirPath().isEmpty()) {
        paths << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(name);
    }
    paths.removeDuplicates();
    return paths;
}

// 端口落在 1..65535，否则返回 fallback。
int clampPort(int v, int fallback)
{
    if (v < 1 || v > 65535) {
        return fallback;
    }
    return v;
}

} // namespace

// 加载 netconfig.ini；找不到则返回结构体内置默认。
NetConfig LoadNetConfig()
{
    NetConfig cfg;
    for (const QString& path : candidatePaths()) {
        if (!QFileInfo::exists(path)) {
            continue;
        }
        QSettings ini(path, QSettings::IniFormat);
        ini.setIniCodec("UTF-8");

        ini.beginGroup(QStringLiteral("network"));
        const QString localIp =
            ini.value(QStringLiteral("local_ip"), cfg.localIp).toString().trimmed();
        const QString destIp =
            ini.value(QStringLiteral("dest_ip"), cfg.destIp).toString().trimmed();
        const int localPort =
            clampPort(ini.value(QStringLiteral("local_port"), cfg.localPort).toInt(),
                      cfg.localPort);
        const int destPort =
            clampPort(ini.value(QStringLiteral("dest_port"), cfg.destPort).toInt(),
                      cfg.destPort);
        const int transferType =
            ini.value(QStringLiteral("transfer_type"), cfg.transferType).toInt();
        const int model = ini.value(QStringLiteral("model"), cfg.model).toInt();
        ini.endGroup();

        ini.beginGroup(QStringLiteral("smoke"));
        const int sansiProto =
            ini.value(QStringLiteral("sansi_proto"), cfg.smokeSansiProto).toInt();
        const int jsonProto =
            ini.value(QStringLiteral("json_proto"), cfg.smokeJsonProto).toInt();
        ini.endGroup();

        if (!localIp.isEmpty()) {
            cfg.localIp = localIp;
        }
        if (!destIp.isEmpty()) {
            cfg.destIp = destIp;
        }
        cfg.localPort = localPort;
        cfg.destPort = destPort;
        cfg.transferType = transferType;
        cfg.model = model;
        cfg.smokeSansiProto = sansiProto;
        cfg.smokeJsonProto = jsonProto;
        cfg.sourcePath = QDir::toNativeSeparators(path);
        cfg.fromFile = true;
        return cfg;
    }
    return cfg;
}

void ApplyNetConfigBase(CommHandler* comm, const NetConfig& cfg)
{
    if (!comm) {
        return;
    }
    comm->SetCommType(NETWORK);
    comm->setParameter(QStringLiteral("iTransferType"), cfg.transferType);
    comm->setParameter(QStringLiteral("iModel"), cfg.model);
    comm->setParameter(QStringLiteral("sLocalIP"), cfg.localIp);
    comm->setParameter(QStringLiteral("iLocalPort"), cfg.localPort);
    comm->setParameter(QStringLiteral("sDestIP"), cfg.destIp);
    comm->setParameter(QStringLiteral("iDestPort"), cfg.destPort);
}

void ConfigureLabUdp(CommHandler* comm, int protoType, const NetConfig& cfg)
{
    ApplyNetConfigBase(comm, cfg);
    if (!comm) {
        return;
    }
    comm->setParameter(QStringLiteral("iProtoType"), protoType);
}