#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

// 串口三思发送帧预期预览（不调用 DLL）。
// 组包规则与 SerialPortComm::SendData 协议 0 对齐，供与串口调试助手对照。
namespace SerialFramePreview {

// 每值占用 segSize 字节（Connect 后 Data5/6/8 对应 5/6/8），末尾追加 0x0D。
QByteArray packSansiSsck(const QVector<double>& values, int segSize = 8);

// 字节序列转为空格分隔的大写十六进制文本。
QString toHexSpaced(const QByteArray& bytes);

// 预览自检。失败返回错误说明，通过返回空字符串。
QString selfCheckOrEmpty();

} // namespace SerialFramePreview
