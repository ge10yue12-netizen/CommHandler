#pragma once

class QString;

// 网口三思接收自检：合法 24 字节帧与非法长度。返回 0 表示通过。
int RunUdpSansiSmoke(QString* detailOut = nullptr);

// 网口 JSON 发送与发送许可自检。返回 0 表示通过。
int RunUdpJsonSendSmoke(QString* detailOut = nullptr);

// 串口三思组包预览自检（不依赖真实串口）。返回 0 表示通过。
int RunSerialPreviewSmoke(QString* detailOut = nullptr);
