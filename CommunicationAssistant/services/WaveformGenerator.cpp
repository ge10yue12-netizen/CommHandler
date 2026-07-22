#include "WaveformGenerator.h"

#include <QStringList>

#include <cmath>

namespace ca {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

void WaveformGenerator::reset(const Config& cfg)
{
    cfg_ = cfg;
    if (cfg_.channels < 1)
        cfg_.channels = 1;
    if (cfg_.channels > 16)
        cfg_.channels = 16;
    if (cfg_.frequencyHz < 0.0)
        cfg_.frequencyHz = 0.0;
    if (cfg_.noiseStd < 0.0)
        cfg_.noiseStd = 0.0;
    rng_.seed(cfg_.seed == 0 ? 1u : cfg_.seed);
}

QVector<double> WaveformGenerator::sample(double tSec) const
{
    QVector<double> out;
    out.reserve(cfg_.channels);
    std::normal_distribution<double> noise(0.0, cfg_.noiseStd > 0.0 ? cfg_.noiseStd : 1.0);
    for (int ch = 0; ch < cfg_.channels; ++ch) {
        double phase = cfg_.phaseRad;
        if (cfg_.staggerChannels && cfg_.channels > 1)
            phase += (2.0 * kPi * static_cast<double>(ch)) / static_cast<double>(cfg_.channels);
        const double angle = 2.0 * kPi * cfg_.frequencyHz * tSec + phase;
        double v = cfg_.offset + cfg_.amplitude * std::sin(angle);
        if (cfg_.noiseStd > 0.0)
            v += noise(rng_);
        out.push_back(v);
    }
    return out;
}

QString WaveformGenerator::toCsv(const QVector<double>& values)
{
    QStringList parts;
    parts.reserve(values.size());
    for (double v : values)
        parts << QString::number(v, 'f', 6);
    return parts.join(QLatin1Char(','));
}

QByteArray WaveformGenerator::toNativeBytes(const QVector<double>& values, bool asHex)
{
    const QByteArray utf8 = toCsv(values).toUtf8();
    if (!asHex)
        return utf8;
    return utf8.toHex(' ');
}

} // namespace ca
