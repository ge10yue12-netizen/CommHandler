// CommController.cpp — CommHandler 回调线程安全封装实现
#include "CommController.h"

#include "UIDef.h"

#include <QMetaType>
#include <cstring>

namespace {
// emitNewData 回调允许的最大 double 个数
constexpr int kMaxCallbackDoubles = 10000;
}

// 注册跨线程元类型，并以 DirectConnection 挂接库信号
CommController::CommController(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<QVector<double>>("QVector<double>");
    qRegisterMetaType<QVariantMap>("QVariantMap");

    QObject::connect(&m_comm, &CommHandler::emitNewData, this,
                     [this](void* data, int size, int type) {
                         if (size <= 0 || data == nullptr || size >= kMaxCallbackDoubles)
                             return;
                         QVector<double> values(size);
                         std::memcpy(values.data(), data, static_cast<size_t>(size) * sizeof(double));
                         emit safeDataReceived(values, type);
                     },
                     Qt::DirectConnection);

    // 试验机侧：同线程去重 START/STOP，并同步 bOnlineCollect
    QObject::connect(&m_comm, &CommHandler::emitEventMsg, this,
                     [this](int ctrlCmd, int viewId, int msg) {
                         if (msg == W_CUSTOM_COMM_STARTCALC) {
                             if (m_cmdCollecting)
                                 return;
                             m_cmdCollecting = true;
                             m_comm.SetCommType(NETWORK);
                             m_comm.setParameter(QStringLiteral("bOnlineCollect"), true);
                         } else if (msg == W_CUSTOM_COMM_STOPCALC) {
                             if (!m_cmdCollecting)
                                 return;
                             m_cmdCollecting = false;
                             m_comm.SetCommType(NETWORK);
                             m_comm.setParameter(QStringLiteral("bOnlineCollect"), false);
                         }
                         emit eventReceived(ctrlCmd, viewId, msg);
                     },
                     Qt::DirectConnection);

    QObject::connect(&m_comm, &CommHandler::emitEventMsgAndData, this,
                     [this](int ctrlCmd, int viewId, int msg, const QVariantMap& extra) {
                         emit eventWithDataReceived(ctrlCmd, viewId, msg, extra);
                     });

    QObject::connect(&m_comm, &CommHandler::emitNewConn, this,
                     [this](int iType, QString ip, int port) {
                         emit peerConnected(iType, ip, port);
                     });

    QObject::connect(&m_comm, &CommHandler::emitClientDisConn, this,
                     [this]() { emit peerDisconnected(); });
}

// 将 W_CUSTOM_* 事件宏转为日志用中文名
QString CommController::eventName(int msg)
{
    switch (msg) {
    case W_CUSTOM_COMM_STARTCALC: return QStringLiteral("开始");
    case W_CUSTOM_COMM_STOPCALC: return QStringLiteral("停止");
    case W_CUSTOM_COMM_EXITPROG: return QStringLiteral("退出");
    case W_CUSTOM_COMM_AUTO_SAVE_IMAGE: return QStringLiteral("存图");
    case W_CUSTOM_ZERO_CLEARING: return QStringLiteral("清零");
    case W_CUSTOM_COMM_SINGALTRIIMAGESAVE: return QStringLiteral("触发存图");
    default: return QStringLiteral("0x%1").arg(msg, 0, 16);
    }
}
