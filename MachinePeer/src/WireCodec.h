#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// 线协议编解码：试验机直接面向字节，不经过 CommHandler。
namespace WireCodec {

// 串口控制指令（末字节均为 0x0D）。
enum class SerialCommand {
    Unknown = 0,
    StartCalc,  // 3D 0D 开始计算
    StopCalc,   // 3E 0D 停止计算
    SaveImage,  // 6B 0D 自动存图
    ZeroClear,  // 5A 0D 清零
};

// 组串口三思数值帧：每值 segSize 字节，帧末 0x0D；非法参数返回空。
QByteArray packSansiSerial(const QVector<double>& values, int segSize = 8);

// 组 UDP 三思测量包：力 / 位移 / 应变三个 double，共 24 字节。
QByteArray packSansiUdp(double force, double displacement, double strain);

// 从累计缓冲按 0x0D 切出完整帧，并从前端移除已消费字节。
QVector<QByteArray> takeFramesEndingCr(QByteArray* buffer);

// 识别是否为二字节控制指令；否则返回 Unknown。
SerialCommand classifySerialCommand(const QByteArray& frame);

// 控制指令的中文名称。
QString serialCommandName(SerialCommand cmd);

// 组控制指令原始字节（命令码 + 0x0D）。
QByteArray packSerialCommand(SerialCommand cmd);

// 字节序列转空格分隔大写十六进制文本。
QString toHexSpaced(const QByteArray& bytes);

} // namespace WireCodec
