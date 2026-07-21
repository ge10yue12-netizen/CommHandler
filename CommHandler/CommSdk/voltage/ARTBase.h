#pragma once
#include <QString>
#include <QDebug>

#ifndef DACHANNEL_SIZE
#define DACHANNEL_SIZE 2
#endif // !DACHANNEL_SIZE

typedef unsigned long       uInt32;
class CARTListener
{
public:
	//需要注意dValue需要在外部释放
	virtual bool OnInputVoltEdited(double* dValue) = 0;
};

class ARTBase
{
public:
	// 释放资源
	virtual void Release() = 0;

	//注册ARTListener
	virtual void RegisterListener(CARTListener* listener) = 0;

	// 设置DO输出数据
	//virtual bool SetDO(bool Para[]);

	//保存AD数据
	//virtual bool SaveAD(QString sOutFile, int nInputRange, int nGroundingMode = 0) = 0;

	// 接收AD数据
	virtual bool ReadDeviceAD(double* pGainData, bool m_bVoltInputExe[]) = 0;
	// 接收DI电压数据
	virtual bool ReadCtrlAD(double* pGainData) = 0;

	//创建AD任务
	virtual bool CreateADTask(uint32_t artIndex, bool* bOpen, double* dVolt) = 0;
	//创建DA任务
	virtual bool CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt) = 0;
	// 创建DI任务
	virtual bool CreateDITask(uint32_t artIndex) = 0;

	//销毁AD任务
	virtual bool ReleaseADTask() = 0;

	//销毁DA任务
	virtual bool ReleaseDATask() = 0;

	// 销毁硬触发任务
	virtual bool ReleaseHardTriggerTask() = 0;

	// 销毁DI任务
	virtual bool ReleaseDITask() = 0;

	// 输出AD数据
	virtual bool VolOut(double dVolValue) = 0;

	// 多路输出
	virtual bool VolOut(double dVolArray[], bool m_bVoltOutputExe[]) = 0;

	//硬触发
	virtual bool HardTrigger(double dValue, double dutyCycle) = 0;

	// 获得AD通道数目
	virtual int GetADChannalNum(void) = 0;

	// 创建CI任务
	virtual bool CreateCITask() = 0;

	// 销毁CI任务
	virtual bool ReleaseCITask() = 0;

	// 读取CI计数器频率
	virtual uInt32 ReadCIFreq() = 0;
};
