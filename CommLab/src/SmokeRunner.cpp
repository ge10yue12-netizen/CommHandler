#include "SmokeRunner.h"

#include "CommController.h"
#include "CommHandler.h"
#include "NetConfig.h"
#include "SerialFramePreview.h"
#include "UdpPeerSimulator.h"
#include "UIDef.h"

#include <QEventLoop>
#include <QHostAddress>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QTimer>
#include <QVector>
#include <cmath>
#include <exception>
#include <vector>

namespace {

constexpr int kSettleMs = 200;
constexpr int kWaitOkMs = 1500;
constexpr int kWaitBadMs = 600;
constexpr int kWaitGateMs = 400;

// 事件循环空转等待 ms 毫秒。
void sleepMs(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

class SmokeSessionGuard
{
public:
    // 作用域结束时 Disconnect 并对端 stop。
    SmokeSessionGuard(CommHandler* comm, UdpPeerSimulator* peer)
        : m_comm(comm)
        , m_peer(peer)
    {
    }

    ~SmokeSessionGuard()
    {
        if (m_comm) {
            m_comm->Disconnect(0);
        }
        if (m_peer) {
            m_peer->stop();
        }
    }

    SmokeSessionGuard(const SmokeSessionGuard&) = delete;
    SmokeSessionGuard& operator=(const SmokeSessionGuard&) = delete;

private:
    CommHandler* m_comm = nullptr;
    UdpPeerSimulator* m_peer = nullptr;
};

} // namespace

// 网口三思：合法 24B 须回调力值，非法长度不得回调。
int RunUdpSansiSmoke(QString* detailOut)
{
    try {
        constexpr double expectForce = 100.5;
        constexpr double expectMove = 1.0;
        constexpr double expectStrain = 0.5;
        constexpr int badByteLen = 10;

        const NetConfig net = LoadNetConfig();
        CommController controller;
        UdpPeerSimulator peer;
        CommHandler* comm = controller.handler();
        ConfigureLabUdp(comm, net.smokeSansiProto, net);

        SmokeSessionGuard guard(comm, nullptr);

        if (!comm->Connect()) {
            if (detailOut) {
                *detailOut = QStringLiteral("连接失败");
            }
            return 1;
        }

        sleepMs(kSettleMs);

        int hit = 0;
        int dataCount = 0;
        QObject::connect(&controller, &CommController::safeDataReceived,
                         [&](const QVector<double>& values, int) {
                             ++dataCount;
                             if (values.size() == 1
                                 && std::fabs(values.at(0) - expectForce) < 1e-6) {
                                 ++hit;
                             }
                         });

        QEventLoop waitOk;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &waitOk, &QEventLoop::quit);
        const QMetaObject::Connection c =
            QObject::connect(&controller, &CommController::safeDataReceived, &waitOk,
                             [&](const QVector<double>& values, int) {
                                 if (values.size() == 1
                                     && std::fabs(values.at(0) - expectForce) < 1e-6) {
                                     waitOk.quit();
                                 }
                             });

        const QByteArray good =
            UdpPeerSimulator::makeSansiPacket(expectForce, expectMove, expectStrain);
        if (peer.sendRaw(good, QHostAddress(net.localIp),
                         static_cast<quint16>(net.localPort)) < 0) {
            QObject::disconnect(c);
            if (detailOut) {
                *detailOut = QStringLiteral("UDP 发送失败");
            }
            return 2;
        }

        timer.start(kWaitOkMs);
        waitOk.exec();
        QObject::disconnect(c);

        if (hit < 1) {
            if (detailOut) {
                *detailOut = QStringLiteral("%1 毫秒内未收到合法数据回调").arg(kWaitOkMs);
            }
            return 3;
        }

        const int before = dataCount;
        peer.sendRaw(QByteArray(badByteLen, 'x'), QHostAddress(net.localIp),
                     static_cast<quint16>(net.localPort));
        sleepMs(kWaitBadMs);

        if (dataCount != before) {
            if (detailOut) {
                *detailOut = QStringLiteral("非法长度报文仍触发回调");
            }
            return 4;
        }

        if (detailOut) {
            *detailOut = QStringLiteral("合法帧已解析，非法长度已静默丢弃（%1:%2）")
                             .arg(net.localIp)
                             .arg(net.localPort);
        }
        return 0;
    } catch (const std::exception& ex) {
        if (detailOut) {
            *detailOut = QStringLiteral("异常：%1").arg(QString::fromLocal8Bit(ex.what()));
        }
        return 99;
    } catch (...) {
        if (detailOut) {
            *detailOut = QStringLiteral("异常：未知");
        }
        return 99;
    }
}

// 网口 JSON：许可关不得出站，许可开对端须收到。
int RunUdpJsonSendSmoke(QString* detailOut)
{
    try {
        const NetConfig net = LoadNetConfig();
        CommController controller;
        UdpPeerSimulator peer;

        if (!peer.startListen(static_cast<quint16>(net.destPort))) {
            if (detailOut) {
                *detailOut = QStringLiteral("对端绑定目的端口失败");
            }
            return 1;
        }

        CommHandler* comm = controller.handler();
        ConfigureLabUdp(comm, net.smokeJsonProto, net);
        SmokeSessionGuard guard(comm, &peer);

        if (!comm->Connect()) {
            if (detailOut) {
                *detailOut = QStringLiteral("连接失败");
            }
            return 2;
        }

        sleepMs(kSettleMs);

        const std::vector<double> payload{1.0, 2.0, 3.0};

        peer.clearReceived();
        comm->setParameter(QStringLiteral("bInquireSendFlag"), false);
        comm->SendData(payload, NETWORK);
        sleepMs(kWaitGateMs);
        if (peer.receivedCount() != 0) {
            if (detailOut) {
                *detailOut = QStringLiteral("发送许可关闭后仍写出数据");
            }
            return 3;
        }

        peer.clearReceived();
        comm->setParameter(QStringLiteral("bInquireSendFlag"), true);
        comm->SendData(payload, NETWORK);
        const bool got = peer.waitReceived(1, kWaitOkMs);

        if (!got) {
            if (detailOut) {
                *detailOut = QStringLiteral("发送许可启用后对端未收到数据");
            }
            return 4;
        }

        if (detailOut) {
            *detailOut = QStringLiteral("JSON 发送与发送许可行为符合约定");
        }
        return 0;
    } catch (const std::exception& ex) {
        if (detailOut) {
            *detailOut = QStringLiteral("异常：%1").arg(QString::fromLocal8Bit(ex.what()));
        }
        return 99;
    } catch (...) {
        if (detailOut) {
            *detailOut = QStringLiteral("异常：未知");
        }
        return 99;
    }
}

// 本地三思组包预览自检（不依赖串口/DLL）。
int RunSerialPreviewSmoke(QString* detailOut)
{
    const QString err = SerialFramePreview::selfCheckOrEmpty();
    if (!err.isEmpty()) {
        if (detailOut) {
            *detailOut = err;
        }
        return 1;
    }
    const QByteArray packed = SerialFramePreview::packSansiSsck(QVector<double>{1.0}, 8);
    if (detailOut) {
        *detailOut = QStringLiteral("三思预览通过（段长 8/6/5）：%1")
                         .arg(SerialFramePreview::toHexSpaced(packed));
    }
    return 0;
}
