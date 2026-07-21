#pragma once
#include "CommBase.h"

class PulseComm :public CommBase
{
	Q_OBJECT
public:
	PulseComm();
	~PulseComm();

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
	bool Disconnect(int type = 0) override;	// type==0 关闭当前连接   1 关闭所有连接，

	// 发送数据
	//void SendData(void*, size_t size) override;

	void SendData(const std::vector<double>& vData) override;

	// 保存配置
	bool SaveParInfo(const QString& filename) override;

	// 读取配置s
	bool LoadParInfo(const QString& filename) override;

	// 将命令配置保存到本地文件
	bool saveCmdToFile(const std::string& filename) override;

	// 从本地文件读取命令配置
	bool loadCmdFromFile(const std::string& filename) override;
private:
	// 初始化getters以及setters
	void initParameters()override;

	// 初始化成员变量
	void initInfo() override;

public:
	// 发送标定数值
	void SendCaliData();

signals:
	// 解析的命令，发送event信号
	void emitEventMsg(int ctrlCmd, int ViewID, int msg);

private:
	//脉冲卡操作函数
	// 标定线程 
	void UpdatePos();
	//打开
	int OpenAxs();
	//初始化
	void InitAxs();
	//读取运动轴位置
	int ReadPos(byte& RunState);
	//定长运动()
	void DeltMove(int iDir, int iPulseNum);
	//关闭
	void CloseAxs();
private:
	// 外部控制信息
	PulseInfo  m_PulseInfo;
	double m_dTransValue;	// 成员变量

	std::unordered_map<QString, std::function<void(const Any&)>> setters;
	std::unordered_map<QString, std::function<Any()>> getters;
};

