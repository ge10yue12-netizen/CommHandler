#pragma once

#include "CommHandler.h"

#include <QObject>
#include <QVector>
#include <QtGlobal>

// CommHandler 安全封装。
// emitNewData 的 void* 在槽函数返回后可能失效；
// 须以 Qt::DirectConnection 立即拷贝，再向界面投递。
class CommController : public QObject
{
    Q_OBJECT

public:
    // 注册元类型并连接库回调（数据 / 事件 / 连接）。
    explicit CommController(QObject* parent = nullptr);

    // 返回内部 CommHandler 实例指针。
    CommHandler* handler();

    // 将已知事件码映射为中文名称；未知码以十六进制表示。
    static QString eventName(int msg);

signals:
    // 已安全拷贝的测量数值。
    void safeDataReceived(QVector<double> values, int type);
    // 库解析到的控制事件。
    void eventReceived(int ctrlCmd, int viewId, int msg);
    // 新对端连接通知。
    void connectionReceived(int iType, QString ip, int port);
    // 对端主动断开。
    void clientDisconnected();
    // 控制器侧日志（空回调提示、异常回调等）。
    void logMessage(QString text);

private:
    CommHandler m_comm;
    qint64 m_lastEmptyCallbackLogMs = 0; // 空数据回调日志节流时间戳
};
