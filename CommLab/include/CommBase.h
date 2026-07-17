#pragma once
#include "Any.h"
#include "UIDef.h"
#include <unordered_map>
#include <fstream>
#include <QString>
#include <QObject>
#include <Windows.h>
#include <array>


class CommBase :public QObject
{
	Q_OBJECT
public:
	virtual ~CommBase() {}

	virtual void SetToDefault() = 0;

	// 关联槽函数
	virtual void ConnectSlots() = 0;

	// 设置所有参数
	virtual bool SetSysParInfo(void*) = 0;

	// 获取所有参数
	virtual bool GetSysParInfo(void*) const = 0;

	// 通用参数设置接口
	virtual void setParameter(const QString& key, const Any& value) = 0;

	// 通用参数获取接口
	virtual Any getParameter(const QString& key) const = 0;

	virtual	bool setExternalFreq(const int& freq) = 0;

	// 连接 电压模式: type = 0-AD ,1-DA,2-CI 3-DI
	virtual bool Connect(int type = -1) = 0;

	// 断开连接 // 电压模式 type= type = 0-AD ,1-DA,2-CI 3-DI ,4 - 关闭所有连接
	virtual bool Disconnect(int type = -1) = 0;	// type==0 关闭当前连接   1 关闭所有连接，

	// 发送数据
	//virtual void SendData(void*, size_t size = 0) = 0;

	virtual void SendData(const std::vector<double>& vData) = 0;

	virtual void SendData(const QString& data) {};

	// 保存配置
	virtual bool SaveParInfo(const QString& filename) = 0;

	// 读取配置
	virtual bool LoadParInfo(const QString& filename) = 0;

	// 将命令配置保存到本地文件
	virtual bool saveCmdToFile(const std::string& filename) = 0;

	// 从本地文件读取命令配置
	virtual bool loadCmdFromFile(const std::string& filename) = 0;

private:
	// 初始化getters以及setters
	virtual void initParameters() = 0;

	// 初始化成员变量
	virtual void initInfo() = 0;

public:
	// 重载setParameter以接受具体类型
	template <typename T>
	void setParameter(const QString& key, T value) {
		setParameter(key, Any(value));
	}
	/// TODO....

	// 重载getParameter以返回具体类型
	template <typename T>
	T getParameter(const QString& key) const {
		return getParameter(key).any_cast<Any>();
	}

	/// TODO....
};

