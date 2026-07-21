#pragma once
#include "./include/Art_DAQ.h"
#include "ARTBase.h"
#include <mutex>
class ART_ARTNET5640 :public ARTBase
{
public:
	ART_ARTNET5640();
	~ART_ARTNET5640();

	// 释放资源
	virtual void Release();
	//注册ARTListener
	virtual void RegisterListener(CARTListener* listener);
	// 创建AD任务
	virtual bool CreateADTask(uint32_t artIndex, bool* bOpen, double* dVolt);
	// 创建DA任务
	virtual bool CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt);
	// 创建DI任务
	virtual bool CreateDITask(uint32_t artIndex);
	//保存AD数据
	//virtual bool SaveAD(QString sOutFile, int nInputRange, int nGroundingMode = 0);
	// 销毁AD任务
	virtual bool ReleaseADTask();
	// 销毁DA任务
	virtual bool ReleaseDATask();
	// 销毁硬触发任务
	virtual bool ReleaseHardTriggerTask();
	// 销毁DI任务
	virtual bool ReleaseDITask();

	// 设置DO输出数据
	//virtual bool SetDO(BOOL Para[]);

	// 接收AD数据
	virtual bool ReadDeviceAD(double* pGainData, bool bVoltInputExe[]);

	// 接收DI电压数据
	virtual bool ReadCtrlAD(double* pGainData);

	// 输出AD数据
	virtual bool VolOut(double dVolValue);

	// 多路输出
	virtual bool VolOut(double dVolArray[], bool bVoltOutputExe[]);

	//硬触发
	virtual bool HardTrigger(double dValue, double dutyCycle);

	// 获得AD通道数目
	virtual int GetADChannalNum(void);

	// 通道名称转换
	virtual const char* NumToName(int iUsedChannel, int type);

	// 创建CI任务
	virtual bool CreateCITask();

	// 销毁CI任务
	virtual bool ReleaseCITask();

	// 读取CI计数器频率
	uInt32 ReadCIFreq();

private:
	const char* m_pPhysicalChannel;
	const char* m_pNameToAssignToChannel;

	bool m_bDATaskCreated;
	bool m_bExtraDATaskCreated;
	double* m_dInputVolt;
	std::mutex m_mutex;
	uint64_t m_showImageTs;

	float64* m_vDATaskData = nullptr, * m_vExtraDATaskData = nullptr;
	int m_iADNum, m_iDANum, m_iExtraNum;
	std::vector<int> m_vChannelIndex;
	//public:
	CARTListener* m_artListener;
	uint64_t m_showVolt;
};
