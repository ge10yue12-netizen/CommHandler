#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

#include <cstdint>
#include <random>

namespace ca {

/**
 * @brief 多通道正弦波形采样（可选高斯噪声）；输出业务数值，不组协议线帧。
 * @thread 由 UI/调度线程调用；非线程安全，单任务独占实例。
 */
class WaveformGenerator
{
public:
    /** 波形参数；通道间相位可均匀错开。 */
    struct Config
    {
        double amplitude = 10.0;
        double frequencyHz = 1.0;
        double phaseRad = 0.0;
        double offset = 0.0;
        double noiseStd = 0.0;
        int channels = 2;
        bool staggerChannels = true;
        quint32 seed = 1;
    };

    WaveformGenerator() = default;

    // 重置参数与随机源
    void reset(const Config& cfg);

    const Config& config() const { return cfg_; }

    /**
     * @brief 在时刻 tSec 采样各通道：offset + A*sin(2πft+φ[+噪声])。
     */
    QVector<double> sample(double tSec) const;

    // 业务 CSV（兼容数值入参 / 原生文本）
    static QString toCsv(const QVector<double>& values);

    // 原生载荷：UTF-8 CSV；asHex=true 时转为空格分隔 HEX
    static QByteArray toNativeBytes(const QVector<double>& values, bool asHex);

private:
    Config cfg_{};
    mutable std::mt19937 rng_{1};
};

} // namespace ca
