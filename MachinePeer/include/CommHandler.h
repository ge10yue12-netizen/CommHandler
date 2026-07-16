#pragma once

// =============================================================================
// CommHandler 门面头（与动态库导出一致）。用法概要：
//   SetCommType → setParameter → Connect → SendData / 监听 emitNewData
// 网口 iProtoType、串口 iProtocolType 真值见仓库《CommHandler-验收问答手册》。
// =============================================================================

#include "CommBase.h"
#include <QtCore/qglobal.h>
#include <QHash>

#ifndef BUILD_STATIC
# if defined(COMMHANDLER_LIB)
#  define COMMHANDLER_EXPORT Q_DECL_EXPORT
# else
#  define COMMHANDLER_EXPORT Q_DECL_IMPORT
# endif
#else
# define COMMHANDLER_EXPORT
#endif

class COMMHANDLER_EXPORT CommHandler : public QObject
{
	Q_OBJECT
public:
	CommHandler(QObject* parent = nullptr);
	~CommHandler();

	void SetToDefault();

	// 关联槽函数
	void ConnectSlots();

	// 设置通讯模式
	void SetCommType(CommType type);

	// 设置 forceUnit 的函数
	void setForceUnit(int unit);

	// 设置所有参数
	bool SetSysParInfo(void*);

	// 获取所有参数
	bool GetSysParInfo(void*) const;

	// 通用参数设置接口
	void setParameter(const QString& key, const Any& value);

	// 通用参数获取接口
	Any getParameter(const QString& key) const;

	// 连接
	bool Connect(int type = -1);

	// 断开连接
	bool Disconnect(int type = 0);	// type==0 关闭当前连接   1 关闭所有连接，

	// 发送数据
	//void SendData(void*, size_t size);

	//void SendData(const QString& data);

	/// 指定某种通讯发送数据包
	//  ptr - 数据结构类型
	//  model - 模块
	///
	void SendData(const std::vector<double>& vData, const CommType&);

	void SendData(const QString& sMsg, const CommType&);

	// 保存配置
	bool SaveParInfo(const QString& filename);

	// 读取配置
	bool LoadParInfo(const QString& filename);

	// 将命令配置保存到本地文件
	bool saveCmdToFile(const QString& filename);

	// 从本地文件读取命令配置
	bool loadCmdFromFile(const QString& filename);
	/// 网口

	/// 串口
	// 发送获取消息指令
	bool SendRequireMsg();

	//数字量输出
	void SendData2ComPort(float fStrain1, float fStrain2, float fMovement1, float fMovement2, float fReserve, float fTime);

	void DataPackAndWrite(const int&);

	void WaitForBytesWritten(const int& ms);

	// 电压
	// 开启硬触发
	bool HardTrigger(double value, double dutyCycle);

	// 释放硬触发
	bool ReleaseHardTrigger();

	bool ReadCIFreq();

	bool ReadADData(double* pdData, double* pGainData, double* truthData, bool bGetVolt = false);
	/// <summary>
	/// 脉冲
	/// </summary>
	// 发送标定数值
	void SendCaliData();

signals:
	// 新数据到来
	void emitNewData(void*, int size, int type);

	// 新的连接到来
	// iType 0- TCP server  1 - Tcp Client 2: Udp - server 3: UDP - client
	void emitNewConn(int iType, QString ip, int port);

	// 第三方主动断开连接，
	void emitClientDisConn();

	// 解析的命令，发送event信号
	void emitEventMsg(int ctrlCmd, int ViewID, int msg);
	void emitEventMsgAndData(const int& ctrlCmd, const int& ViewID, const int& msg, const QVariantMap& extra);

public:
	template <typename T>
	void setParameter(const QString& key, T value)
	{
		try {
			// 首先尝试当前设置的通信方式
			return m_curCommModule->setParameter(key, T(value));
		}
		catch (const std::invalid_argument&) {
			// 如果失败，尝试其他通信方式
			CommType commTypes[] = { NETWORK, SERIAL, VOLTAGE, PULSE };
			for (CommType type : commTypes)
			{
				CommBase* commModule = m_commModules.value(type, nullptr);
				if (!commModule || commModule == m_curCommModule) continue;
				try
				{
					return commModule->setParameter(key, T(value));
				}
				catch (const std::invalid_argument&) {
					// 捕获异常并尝试下一种通信方式
					continue;
				}
			}
		}
		throw std::runtime_error("Operation failed for all communication types.");
	}

	template <typename T>
	T getParameter(const QString& key) const
	{
		try {
			// 首先尝试当前设置的通信方式
			return  m_curCommModule->getParameter(key).any_cast<T>();
		}
		catch (const std::invalid_argument&) {
			// 如果失败，尝试其他通信方式
			CommType commTypes[] = { NETWORK, SERIAL, VOLTAGE, PULSE };
			for (CommType type : commTypes)
			{
				CommBase* commModule = m_commModules.value(type, nullptr);
				if (!commModule || commModule == m_curCommModule) continue;
				try
				{
					return commModule->getParameter(key).any_cast<T>();
				}
				catch (const std::invalid_argument&) {
					// 捕获异常并尝试下一种通信方式
					continue;
				}
			}
		}
		return T();
		//throw std::runtime_error("Operation failed for all communication types.");
	}

private slots:
	// 定义接收子类信号的槽函数
	void onNewDataReceived(void* data, int size, int type);
	void onNewConnection(int iType, QString ip, int port);
	void onClientDisconnected();
	void onEventMessage(int ctrlCmd, int ViewID, int msg);
	void onEventMsgAndData(const int& ctrlCmd, const int& ViewID, const int& msg, const QVariantMap& extra);

private:
	CommBase* m_curCommModule;
	QHash<CommType, CommBase*> m_commModules;

	bool m_bConnect;
};

