#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

// ini 候选路径：工作目录与 exe 目录。
inline QStringList iniPaths(const QString& name)
{
    QStringList paths;
    paths << QDir::current().absoluteFilePath(name);
    if (!QCoreApplication::applicationDirPath().isEmpty())
        paths << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(name);
    return paths;
}

// 端口钳制到合法范围。
inline int clampPort(int v, int fb) { return (v >= 1 && v <= 65535) ? v : fb; }
