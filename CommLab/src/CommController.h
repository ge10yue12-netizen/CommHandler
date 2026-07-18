// CommController.h — CommHandler 回调线程安全封装（工作线程拷贝，UI 线程收信号）
#pragma once

#include "CommHandler.h"

#include <QObject>
#include <QVariantMap>
#include <QVector>

// 封装 CommHandler 实例及库信号转发
class CommController : public QObject
{
    Q_OBJECT

public:
    // 注册元类型并挂接 emitNewData / emitEventMsg 等库回调
    explicit CommController(QObject* parent = nullptr);
    // 返回内部 CommHandler，供参数设置与发送调用
    CommHandler* handler() { return &m_comm; }
    // 将 W_CUSTOM_* 事件码转为日志显示名
    static QString eventName(int msg);

signals:
    // 已在工作线程完成 memcpy 的测量数据
    void safeDataReceived(QVector<double> values, int type);
    // 库 emitEventMsg 转发的控制事件
    void eventReceived(int ctrlCmd, int viewId, int msg);
    // 库 emitEventMsgAndData 转发的带附加数据事件
    void eventWithDataReceived(int ctrlCmd, int viewId, int msg, QVariantMap extra);

private:
    CommHandler m_comm;
};
