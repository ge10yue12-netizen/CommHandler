// ProtoGuide.h — MachinePeer：协议与串口参数下拉填充
#pragma once

#include <QComboBox>

namespace ProtoGuide {

// 网口协议列表；itemData 对齐库 iProtoType
inline void fillNetProto(QComboBox* net)
{
    net->clear();
    const char* np[] = {"0 JSON", "1 万测", "2 中机", "3 三思", "4 触发存图", "5 福建威盛", "6 纳百川", "7 纳百川线条"};
    for (int i = 0; i < 8; ++i)
        net->addItem(QString::fromUtf8(np[i]), i);
}

// 串口协议列表；itemData 对齐库 iProtocolType
inline void fillSerialProto(QComboBox* ser)
{
    ser->clear();
    const char* sp[] = {"0 三思", "1 科新", "2 时代新材", "3 IEEE", "4 冠腾"};
    for (int i = 0; i < 5; ++i)
        ser->addItem(QString::fromUtf8(sp[i]), i);
}

// 串口物理参数下拉
inline void fillSerialCombos(QComboBox* baud, QComboBox* dataBits, QComboBox* parity, QComboBox* stopBits)
{
    baud->clear();
    const char* bd[] = {"4800", "9600", "19200", "38400", "57600", "115200"};
    for (int i = 0; i < 6; ++i)
        baud->addItem(QString::fromUtf8(bd[i]), i);
    baud->setCurrentIndex(5);
    dataBits->clear();
    dataBits->addItem(QStringLiteral("5"), 0);
    dataBits->addItem(QStringLiteral("6"), 1);
    dataBits->addItem(QStringLiteral("7"), 2);
    dataBits->addItem(QStringLiteral("8"), 3);
    dataBits->setCurrentIndex(3);
    parity->clear();
    parity->addItems({QStringLiteral("N"), QStringLiteral("E"), QStringLiteral("O"), QStringLiteral("S"),
                      QStringLiteral("M")});
    stopBits->clear();
    stopBits->addItems({QStringLiteral("1"), QStringLiteral("1.5"), QStringLiteral("2")});
}

} // namespace ProtoGuide
