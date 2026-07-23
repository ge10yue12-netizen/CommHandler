#pragma once

#include "LegacyCapability.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace ca {

// 线帧预览结果：对齐 CommHandler 写出语义（库不回传原始字节时供数据显示）
struct LegacyWirePreview
{
    QByteArray bytes;          // 预估发出字节；空表示无法可靠重建
    bool binary = false;       // true：数据显示默认按 HEX
    QString note;              // 无法重建或补充说明
};

// 按协议重建「实际发出」载荷预览（镜像 DLL 编码，不改 DLL）
LegacyWirePreview previewLegacySendWire(LegacyCommKind kind, int protocolIndex,
                                        const QVector<double>& values, int dataBits,
                                        const QString& textPayload = QString());

// 能力区：标准收发数据边界（位数/定宽/路数）
QStringList legacyDataBoundaryTips(LegacyCommKind kind, int protocolIndex, int dataBits);

} // namespace ca
