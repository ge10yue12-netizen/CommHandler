#ifndef VOLTAGEBOX_H
#define VOLTAGEBOX_H

#include <cstdint>

enum ArtType
{
	UNKNOWN = -1,
	ART3134A,
	ART5640_NET,
	NI_,
};

class ARTBase;
class CARTListener;
class  Voltage
{
public:
	Voltage();
	~Voltage();

	//注册ARTListener
	void RegisterListener(CARTListener* listener);
	//////////////////////////////////////////////////////////////////////////
	// 设置类型
	void SetBoxType(const ArtType&);
	// 打开控制箱
	bool OpenCtrlBox();
	// 关闭控制箱
	bool CloseCtrlBox();
	// 切换控制箱
	bool ChangeCtrlBox();

	bool ReadADData(double* pdData, double* pGainData, double* truthData, bool bVoltInputExe[], double dCompensation[], bool bGetVolt = false);
	bool WriteDAData(double* pdData, bool bVoltOutputExe[]);
	bool ReadCtrlAD(double* pdData);


	bool CreateADTask(uint32_t artIndex, bool* bOpen, double* dVolt);
	bool CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt);
	bool CreateCITask();
	bool CreateDITask(uint32_t artIndex);

	bool ReleaseADTask();
	bool ReleaseDATask();
	bool ReleaseHardTrigger();
	bool ReleaseCITask();
	bool ReleaseDITask();

	bool HardTrigger(double dValue, double dutyCycle = 0.11111);
	bool ReadCIFreq();

	// 是否打开
	//bool m_bOpen;
	ArtType m_artType;

	//比例系数
	double m_dValueK[16];
	//补偿参数
	double m_dValueB[16];
	// 读取电压
	bool m_bReadAD;

private:
	ARTBase* m_mArtDevice;
};

#endif // VOLTAGEBOX_H
