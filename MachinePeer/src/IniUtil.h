// IniUtil.h — MachinePeer：ini 路径搜索与端口钳制
#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

// 返回工作目录与 exe 目录下的 ini 候选绝对路径
inline QStringList iniPaths(const QString& name)
{
    QStringList paths;
    paths << QDir::current().absoluteFilePath(name);
    if (!QCoreApplication::applicationDirPath().isEmpty())
        paths << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(name);
    return paths;
}

// 将端口钳制到 [1,65535]；越界则回退 fb
inline int clampPort(int v, int fb)
{
    return (v >= 1 && v <= 65535) ? v : fb;
}
