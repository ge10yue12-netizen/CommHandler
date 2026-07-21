#pragma once
#include "CommBase.h"
#include "VoltageBox.h"

class VoltageComm :public CommBase
{
	Q_OBJECT
public:
	VoltageComm();
	~VoltageComm();

	void SetToDefault();

	// 关联槽函数
	void ConnectSlots() override;

	// 设置所有参数
	bool SetSysParInfo(void*) override;

	// 获取所有参数
	bool GetSysParInfo(void*) const override;

	// 通用参数设置接口
	void setParameter(const QString& key, const Any& value) override;

	// 通用参数获取接口
	Any getParameter(const QString& key) const override;

	// 设置外部设备发送的力值频率
	bool setExternalFreq(const int& freq) override;

	// 连接 
	bool Connect(int type = -1) override;

	// 断开连接
	bool Disconnect(int type = 0) override;

	// 发送数据
	// void SendData(void*, size_t size) override;

	void SendData(const std::vector<double>& vData) override;

	// 保存配置
	bool SaveParInfo(const QString& filename) override;

	// 读取配置
	bool LoadParInfo(const QString& filename) override;

	// 将命令配置保存到本地文件
	bool saveCmdToFile(const std::string& filename) override;

	// 从本地文件读取命令配置
	bool loadCmdFromFile(const std::string& filename) override;

	// 开启硬触发
	bool HardTrigger(double value, double dutyCycle);

	// 释放硬触发
	bool ReleaseHardTrigger();

	bool ReadCIFreq();

	bool ReadADData(double* pdData, double* pGainData, double* truthData, bool bGetVolt = false);

private:
	// 初始化getters以及setters
	void initParameters()override;

	// 初始化成员变量
	void initInfo() override;

signals:
	// 新数据到来
	void emitNewData(double*, int size, int type);

private:
	// 外部控制信息
	VoltageInfo m_VoltageInfo;

	std::unordered_map<QString, std::function<void(const Any&)>> setters;
	std::unordered_map<QString, std::function<Any()>> getters;

	Voltage m_controlBox;
};

