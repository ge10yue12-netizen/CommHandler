#include "NI_DAQ.h"
#include "./include/NIDAQmx.h"
#include "./utils/TimeUtils.h"
#include <cstring>


#define NiDAQErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

// 确保回调函数签名匹配
int32 __cdecl DAQDoneCallback(TaskHandle taskHandle, int32 status, void* callbackData)
{
	int32 error = 0;
	char errBuff[2048] = { '\0' };

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxClearTask(taskHandle);
		//std::cerr << "NIDAQ Error: " << errBuff << std::endl;
	}
	return 0;
}
NI_DAQ::NI_DAQ()
{
	m_artListener = NULL;
	m_showVolt = 0;
	m_iADNum = 0;
}

NI_DAQ::~NI_DAQ()
{
	if (m_artListener != nullptr)
	{
		delete m_artListener;
		m_artListener = NULL;
	}
	if (m_vDATaskData)
	{
		delete[] m_vDATaskData;
		m_vDATaskData = NULL;
	}

	Release();
}

void NI_DAQ::Release()
{
	ReleaseDATask();

	ReleaseADTask();

	//DAQmxStopTask(m_hardTriggerTask);
	//DAQmxClearTask(m_hardTriggerTask);
}

void NI_DAQ::RegisterListener(CARTListener* listener)
{
	m_artListener = listener;
}

bool NI_DAQ::CreateADTask(uint32_t artIndex, bool* bUsedChannel, double* dVolt)
{
	int32_t       error = 0;
	char        errBuff[2048] = { '\0' };

	m_iADNum = 0;
	NiDAQErrChk(DAQmxCreateTask("Task2", &m_voltInTask));
	for (int i = 0; i < 16; i++)
	{
		m_bInputUsed[i] = bUsedChannel[i];
		if (bUsedChannel[i])
		{
			NiDAQErrChk(DAQmxCreateAIVoltageChan(m_voltInTask, NumToName(i), "", DAQmx_Val_RSE, -10, 10, DAQmx_Val_Volts, 0));
			m_iADNum++;
		}
	}
	NiDAQErrChk(DAQmxStartTask(m_voltInTask));
	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}
	if (m_voltInTask)
	{
		//DAQmxStopTask(m_voltInTask);
		//DAQmxClearTask(m_voltInTask);
		ReleaseADTask();
	}
	return false;
}

bool NI_DAQ::CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt)
{
	int32_t       error = 0;
	char        errBuff[2048] = { '\0' };

	NiDAQErrChk(DAQmxCreateTask("Task3", &m_voltOutTask));
	for (int i = 0; i < DACHANNEL_SIZE; i++)
	{
		if (bOpen[i])
		{
			std::string sChannelName = QString("Dev1/ao%1").arg(QString::number(i)).toStdString();
			NiDAQErrChk(DAQmxCreateAOVoltageChan(m_voltOutTask, sChannelName.c_str(), "", -10, 10, DAQmx_Val_Volts, 0));
		}
	}
	NiDAQErrChk(DAQmxStartTask(m_voltOutTask));

	delete[] m_vDATaskData;
	m_vDATaskData = new float64[m_iDANum];

	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}
	if (m_voltOutTask)
	{
		//DAQmxStopTask(m_voltOutTask);
		//DAQmxClearTask(m_voltOutTask);
		ReleaseDATask();
	}
	return false;
}

bool NI_DAQ::CreateDITask(uint32_t artIndex)
{
	int32       error = 0;
	char        errBuff[2048] = { '\0' };

	NiDAQErrChk(DAQmxCreateTask("Task5", &m_DIListenerTask));
	NiDAQErrChk(DAQmxCreateAIVoltageChan(m_DIListenerTask, NumToName(15), "", DAQmx_Val_RSE, -10.0, 10.0, DAQmx_Val_Volts, NULL));
	//DAQmxCfgSampClkTiming(m_ReadTask, "", 20, DAQmxVal_Rising, DAQmxVal_ContSamps, 1);
	NiDAQErrChk(DAQmxStartTask(m_DIListenerTask));

	return true;
Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}

	if (m_DIListenerTask) {
		DAQmxStopTask(m_DIListenerTask);
		//DAQmxClearTask(m_ReadTask);
	}

	return false;
}

bool NI_DAQ::ReleaseADTask()
{
	if (m_voltInTask)
	{
		DAQmxStopTask(m_voltInTask);
		DAQmxClearTask(m_voltInTask);
	}
	return true;
}

bool NI_DAQ::ReleaseDATask()
{
	if (m_voltOutTask)
	{
		DAQmxStopTask(m_voltOutTask);
		DAQmxClearTask(m_voltOutTask);
	}
	return true;
}

bool NI_DAQ::ReleaseHardTriggerTask()
{
	if (m_hardTriggerTask != NULL)
	{
		DAQmxStopTask(m_hardTriggerTask);
		DAQmxClearTask(m_hardTriggerTask);
	}

	return true;
}

bool NI_DAQ::ReleaseDITask()
{
	if (m_DIListenerTask)
	{
		DAQmxStopTask(m_DIListenerTask);
		DAQmxClearTask(m_DIListenerTask);
	}
	return true;
}

bool NI_DAQ::ReadDeviceAD(double* pGainData, bool bVoltInputExe[])
{
	if (m_iADNum <= 0)
		return false;

	int32       error = 0;
	int32       read;
	char        errBuff[2048] = { '\0' };
	double* dGainData = new double[16];
	double* dShowData = new double[16];
	for (int i = 0; i < 16; i++)
	{
		dGainData[i] = 0;
		dShowData[i] = 0;
	}
	NiDAQErrChk(DAQmxReadAnalogF64(m_voltInTask, -1, 10.0, DAQmx_Val_GroupByScanNumber, dGainData, m_iADNum, &read, NULL));

	int index = 0;
	for (int i = 0; i < 16; i++)
	{
		if (!bVoltInputExe[i])
		{
			continue;
		}
		if (index >= m_iADNum)
			continue;
		double value = dGainData[index++];
		pGainData[i] = value;
		dShowData[i] = value;
	}

	delete dGainData;
	dGainData = nullptr;

	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}

	ReleaseADTask();

	delete[] dGainData;
	dGainData = nullptr;

	delete[] dShowData;
	dShowData = nullptr;

	return false;
}

bool NI_DAQ::ReadCtrlAD(double* pGainData)
{
	int32       error = 0;
	int32       read;
	char        errBuff[2048] = { '\0' };


	//LOG(plog::debug) << "开始输入";
	NiDAQErrChk(DAQmxReadAnalogF64(m_DIListenerTask, -1, 10.0, DAQmx_Val_GroupByChannel, pGainData, 1, &read, NULL));
	//LOG(plog::debug) << "输入值" << *pGainData;
	if (read <= 0)
	{
		*pGainData = 0.0;
	}
	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}

	if (m_DIListenerTask) {
		DAQmxStopTask(m_DIListenerTask);
		//DAQmxClearTask(m_ReadTask);
	}
	return false;
}

bool NI_DAQ::VolOut(double dVolValue)
{
	int32_t       error = 0;
	char        errBuff[2048] = { '\0' };

	float64     data[1]{};
	data[0] = dVolValue;

	if (m_voltOutTask)
	{
		NiDAQErrChk(DAQmxWriteAnalogF64(m_voltOutTask, 1, 1, 10, DAQmx_Val_GroupByChannel, data, NULL, NULL));
	}
	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}
}

bool NI_DAQ::VolOut(double dVolArray[], bool bVoltOutputExe[])
{
	int32_t       error = 0;
	char        errBuff[2048] = { '\0' };
	bool bOpened = false, bOpened1 = false;
	int index = 0;
	for (int i = 0; i < DACHANNEL_SIZE; i++)
	{
		if (!bVoltOutputExe[i])
		{
			continue;
		}
		if (i >= 0 && i <= 3)
		{
			bOpened = true;
			//m_vDATaskData[index] = dVolArray[i];
			if (index >= m_iDANum)
				continue;
			m_vDATaskData[index] = dVolArray[i];
			index++;

		}
		else if (i >= 4 && i <= 7)
		{
			bOpened1 = true;
			m_vExtraDATaskData[i] = dVolArray[i];
		}
	}
	NiDAQErrChk(DAQmxWriteAnalogF64(m_voltOutTask, 1, 1, 10, DAQmx_Val_GroupByChannel, m_vDATaskData, NULL, NULL));

	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	}
}

bool NI_DAQ::HardTrigger(double dValue, double dutyCycle)
{
	int32         error = 0;
	float64 freq = float64(dValue);
	char        errBuff[2048] = { '\0' };

	if (m_hardTriggerTask)
	{
		DAQmxStopTask(m_hardTriggerTask);
		DAQmxClearTask(m_hardTriggerTask);
	}

	NiDAQErrChk(DAQmxCreateTask("Task4", &m_hardTriggerTask));
	NiDAQErrChk(DAQmxCreateCOPulseChanFreq(m_hardTriggerTask, "Dev1/ctr0", "", DAQmx_Val_Hz, DAQmx_Val_High, 0.0, float64(freq), dutyCycle));
	NiDAQErrChk(DAQmxCfgImplicitTiming(m_hardTriggerTask, DAQmx_Val_ContSamps, 1000));
	NiDAQErrChk(DAQmxRegisterDoneEvent(m_hardTriggerTask, 0, DAQDoneCallback, NULL));
	NiDAQErrChk(DAQmxStartTask(m_hardTriggerTask));


	return true;

Error:
	if (DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		//std::cerr << "NIDAQ Error: " << errBuff << std::endl;
	}
	if (m_hardTriggerTask)
	{
		DAQmxStopTask(m_hardTriggerTask);
		DAQmxClearTask(m_hardTriggerTask);
	}

	return false;
}

int NI_DAQ::GetADChannalNum(void)
{
	return ADNUM;
}

const char* NI_DAQ::NumToName(int iUsedChannel)
{
	const char* ChannelName = "";
	switch (iUsedChannel)
	{
	case 0:
		ChannelName = "Dev1/ai0";
		break;
	case 1:
		ChannelName = "Dev1/ai1";
		break;
	case 2:
		ChannelName = "Dev1/ai2";
		break;
	case 3:
		ChannelName = "Dev1/ai3";
		break;
	case 4:
		ChannelName = "Dev1/ai4";
		break;
	case 5:
		ChannelName = "Dev1/ai5";
		break;
	case 6:
		ChannelName = "Dev1/ai6";
		break;
	case 7:
		ChannelName = "Dev1/ai7";
		break;
	case 8:
		ChannelName = "Dev1/ai8";
		break;
	case 9:
		ChannelName = "Dev1/ai9";
		break;
	case 10:
		ChannelName = "Dev1/ai10";
		break;
	case 11:
		ChannelName = "Dev1/ai11";
		break;
	case 12:
		ChannelName = "Dev1/ai12";
		break;
	case 13:
		ChannelName = "Dev1/ai13";
		break;
	case 14:
		ChannelName = "Dev1/ai14";
		break;
	case 15:
		ChannelName = "Dev1/ai15";
		break;
	}
	return ChannelName;
}

bool NI_DAQ::CreateCITask()
{
	int         error = 0;
	char        errBuff[2048] = { '\0' };

	/*********************************************/
	// ArtDAQ Configure Code
	/*********************************************/
	NiDAQErrChk(DAQmxCreateTask("Task5", &m_readCITask));
	NiDAQErrChk(DAQmxCreateCICountEdgesChan(m_readCITask, "Dev1/ctr0", "", DAQmx_Val_Rising, 0, DAQmx_Val_CountUp));

	/*********************************************/
	// ArtDAQ Start Code
	/*********************************************/
	NiDAQErrChk(DAQmxStartTask(m_readCITask));

	return true;
Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	if (m_readCITask != 0) {
		/*********************************************/
		// ArtDAQ Stop Code
		/*********************************************/
		DAQmxStopTask(m_readCITask);
		DAQmxClearTask(m_readCITask);
	}
	return false;
}

bool NI_DAQ::ReleaseCITask()
{
	if (m_readCITask != NULL)
	{
		DAQmxStopTask(m_readCITask);
		DAQmxClearTask(m_readCITask);
	}
	return true;
}

uInt32 NI_DAQ::ReadCIFreq()
{
	int         error = 0;
	uInt32		data;
	char        errBuff[2048] = { '\0' };

	NiDAQErrChk(DAQmxReadCounterScalarU32(m_readCITask, 1, &data, 0));

	return data;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	if (m_readCITask != 0) {
		/*********************************************/
		// ArtDAQ Stop Code
		/*********************************************/
		//DAQmxStopTask(m_readCITask);
		//DAQmxClearTask(m_readCITask);
	}
	return 0;
}
