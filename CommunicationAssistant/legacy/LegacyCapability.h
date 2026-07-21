#pragma once

#include <QHash>
#include <QString>
#include <QVector>

namespace ca {

enum class LegacyCapability
{
    ReceiveValues,
    ReceiveControlEvents,
    ReceiveParameterEvents,
    SendEncodedValues,
    SendTransparentText,
    RequiresPollingPermission,
    RequiresStreamingState,
    RawReceive,
    RawSend
};

enum class LegacyCommKind
{
    Network = 0,
    Serial = 1
};

struct LegacyCapabilityEntry
{
    bool supported = false;
    QString limitation;
    QString verificationNote;
};

struct LegacyCapabilityProfile
{
    LegacyCommKind commKind = LegacyCommKind::Network;
    int protocolIndex = 0;
    QHash<int, LegacyCapabilityEntry> entries;

    bool supports(LegacyCapability c) const
    {
        return entries.value(static_cast<int>(c)).supported;
    }
};

/** emitNewData 复制上限（元素个数，非字节）。 */
constexpr int kMaxLegacyValues = 64;

/** 未解析 RX 单包展示/入队上限（字节）；超出截断并打 truncated 标记。 */
constexpr int kMaxUnparsedBytes = 4096;

/** 未解析 RX 最小上报间隔（毫秒）；窗口内合并为丢弃计数，防 UI 洪泛。 */
constexpr int kUnparsedMinIntervalMs = 100;

/**
 * @brief 按基线 §14 生成二元能力表（仅支持/不支持）。
 * @thread 任意；纯函数。
 */
LegacyCapabilityProfile legacyCapabilityFor(LegacyCommKind kind, int protocolIndex);

QString legacyCapabilityName(LegacyCapability c);
QStringList unsupportedCapabilityTips(const LegacyCapabilityProfile& profile);

} // namespace ca