#pragma once

#include "LegacyCapability.h"
#include "Result.h"
#include "SessionConfig.h"

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVector>

namespace ca {

/**
 * @brief Legacy 后端抽象：便于 Mock 自测与 CommHandler 实装解耦。
 * @thread 仅在 LegacyWorker 线程创建与调用。
 */
class ILegacyBackend : public QObject
{
    Q_OBJECT
public:
    explicit ILegacyBackend(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~ILegacyBackend() override = default;

    virtual Result configure(const LegacyConfig& config) = 0;
    virtual Result connectDevice() = 0;
    virtual Result disconnectDevice() = 0;
    virtual Result sendValues(const QVector<double>& values) = 0;
    virtual Result sendText(const QString& text) = 0;

signals:
    // 已完成边界检查与拷贝的数值事件（不再持有 DLL 裸指针）
    void valuesReceived(const QVector<double>& values, int type);
    void controlEvent(int ctrlCmd, int viewId, int msg);
    void parameterEvent(int ctrlCmd, int viewId, int msg, const QVariantMap& extra);
    // DLL 上报：有进线字节但当前协议未解出业务数据
    void unparsedRx(const QByteArray& raw);
    void backendError(const QString& code, const QString& message);
    void disconnected();
};

/** 自测用后端：不链接 CommHandler。 */
class MockLegacyBackend : public ILegacyBackend
{
    Q_OBJECT
public:
    explicit MockLegacyBackend(QObject* parent = nullptr);

    Result configure(const LegacyConfig& config) override;
    Result connectDevice() override;
    Result disconnectDevice() override;
    Result sendValues(const QVector<double>& values) override;
    Result sendText(const QString& text) override;

    // 测试注入：模拟 DLL emitNewData 裸指针路径
    void injectRawPointerData(const QVector<double>& values, int type);
    void injectControl(int ctrlCmd, int viewId, int msg);

    int connectCount_ = 0;
    int sendValueCount_ = 0;
    QVector<double> lastSentValues_;
    LegacyConfig config_;
    bool connected_ = false;
};

/**
 * @brief CommHandler 后端：仅在 Worker 线程 new/delete CommHandler。
 * 编译开关：CA_LINK_COMMHANDLER=1 时链接真实 DLL。
 */
class CommHandlerBackend : public ILegacyBackend
{
    Q_OBJECT
public:
    explicit CommHandlerBackend(QObject* parent = nullptr);
    ~CommHandlerBackend() override;

    Result configure(const LegacyConfig& config) override;
    Result connectDevice() override;
    Result disconnectDevice() override;
    Result sendValues(const QVector<double>& values) override;
    Result sendText(const QString& text) override;

private slots:
    void onEmitNewData(void* data, int size, int type);
    void onEmitEventMsg(int ctrlCmd, int viewId, int msg);
    void onEmitEventMsgAndData(const int& ctrlCmd, const int& viewId, const int& msg,
                               const QVariantMap& extra);
    void onEmitUnparsedRx(const QByteArray& raw);
    void onClientDisconnected();

private:
    void destroyHandler();

    LegacyConfig config_;
    void* handler_ = nullptr; // CommHandler* 在 cpp 中转型，避免头文件污染
    bool connected_ = false;
};

} // namespace ca