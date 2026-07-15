#include "CommController.h"

#include "UIDef.h"

#include <QDateTime>
#include <QMetaType>
#include <cstring>

namespace {
// emitNewData 长度上限：超过视为异常包并丢弃。
constexpr int kMaxCallbackDoubles = 10000;
// 空数据回调日志最短间隔（毫秒），避免刷屏。
constexpr qint64 kEmptyCallbackLogIntervalMs = 1000;
} // namespace

// 注册元类型并连接库回调（数据 / 事件 / 连接）。
CommController::CommController(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<QVector<double>>("QVector<double>");

    // DirectConnection：在发射线程立即拷贝数值；禁止将原始 void* 排队投递给界面。
    QObject::connect(&m_comm, &CommHandler::emitNewData, this,
                     [this](void* data, int size, int type) {
                         // 空长度：常见于连接/许可或解析失败时的库内部回调，不是有效测量。
                         if (size <= 0) {
                             const qint64 now = QDateTime::currentMSecsSinceEpoch();
                             if (now - m_lastEmptyCallbackLogMs >= kEmptyCallbackLogIntervalMs) {
                                 m_lastEmptyCallbackLogMs = now;
                                 emit logMessage(
                                     QStringLiteral("[收] 忽略空数据回调（length=%1，非有效测量值）")
                                         .arg(size));
                             }
                             return;
                         }
                         if (data == nullptr || size >= kMaxCallbackDoubles) {
                             emit logMessage(
                                 QStringLiteral("[警告] 丢弃无效数据回调：指针=%1，长度=%2")
                                     .arg(reinterpret_cast<quintptr>(data))
                                     .arg(size));
                             return;
                         }
                         QVector<double> values;
                         values.resize(size);
                         std::memcpy(values.data(), data,
                                     static_cast<size_t>(size) * sizeof(double));
                         emit safeDataReceived(values, type);
                     },
                     Qt::DirectConnection);

    QObject::connect(&m_comm, &CommHandler::emitEventMsg, this,
                     [this](int ctrlCmd, int viewId, int msg) {
                         emit eventReceived(ctrlCmd, viewId, msg);
                     });

    QObject::connect(&m_comm, &CommHandler::emitNewConn, this,
                     [this](int iType, QString ip, int port) {
                         emit connectionReceived(iType, ip, port);
                     });

    QObject::connect(&m_comm, &CommHandler::emitClientDisConn, this,
                     [this]() { emit clientDisconnected(); });
}

// 返回内部 CommHandler 实例指针。
CommHandler* CommController::handler()
{
    return &m_comm;
}

// 将已知事件码映射为中文名称；未知码以十六进制表示。
QString CommController::eventName(int msg)
{
    switch (msg) {
    case W_CUSTOM_COMM_STARTCALC:
        return QStringLiteral("开始计算");
    case W_CUSTOM_COMM_STOPCALC:
        return QStringLiteral("停止计算");
    case W_CUSTOM_COMM_EXITPROG:
        return QStringLiteral("退出程序");
    case W_CUSTOM_COMM_AUTO_SAVE_IMAGE:
        return QStringLiteral("自动存图");
    case W_CUSTOM_ZERO_CLEARING:
        return QStringLiteral("清零");
    case W_CUSTOM_COMM_SINGALTRIIMAGESAVE:
        return QStringLiteral("信号触发存图");
    default:
        return QStringLiteral("未知事件(0x%1)").arg(msg, 0, 16);
    }
}
