#pragma once

#include <vector>
#include <mutex>
#include "ARTBase.h"

#ifndef ADNUM
#define ADNUM 16
#endif // !ADNUM
typedef double             float64;
typedef void* TaskHandle;

class NI_DAQ : public ARTBase
{
public:
	NI_DAQ();
	virtual ~NI_DAQ();

	// 释放资源
	virtual void Release();

	//注册ARTListener
	virtual void RegisterListener(CARTListener* listener);

	//创建AD任务
	virtual bool CreateADTask(uint32_t artIndex, bool* bUsedChannel, double* dVolt);

	//创建DA任务
	virtual bool CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt);

	//创建DI监听任务
	virtual bool CreateDITask(uint32_t artIndex);

	//销毁AD任务
	virtual bool ReleaseADTask();

	//销毁DA任务
	virtual bool ReleaseDATask();

	// 销毁硬触发任务
	virtual bool ReleaseHardTriggerTask();

	//销毁DI监听任务
	virtual bool ReleaseDITask();

	////控制灯光
	//virtual bool SetDO(bool isOpen);

	// 接收AD数据
	virtual bool ReadDeviceAD(double* pGainData, bool bVoltInputExe[]);

	// 监听控制电压值
	virtual bool ReadCtrlAD(double* pGainData);

	//// 输出DA数据
	virtual bool VolOut(double dVolValue);

	// 多路输出
	virtual bool VolOut(double dVolArray[], bool bVoltOutputExe[]);

	//硬触发
	virtual bool HardTrigger(double dValue, double dutyCycle);

	// 获得AD通道数目
	virtual int GetADChannalNum(void);

	//通道名称转换
	virtual const char* NumToName(int iUsedChannel);

	// 创建CI任务
	virtual bool CreateCITask();

	// 销毁CI任务
	virtual bool ReleaseCITask();

	// 读取CI计数器频率
	virtual uInt32 ReadCIFreq();

private:
	TaskHandle m_voltInTask, m_voltOutTask, m_hardTriggerTask, m_DIListenerTask, m_readCITask;

	bool m_bInputUsed[16] = {};
	int m_iADNum, m_iDANum;

	float64* m_vDATaskData = nullptr, * m_vExtraDATaskData = nullptr;
public:
	CARTListener* m_artListener;
	uint64_t m_showVolt;
	std::mutex m_readADmutex;
};
