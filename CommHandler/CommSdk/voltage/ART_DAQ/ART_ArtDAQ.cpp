// ART_ArtDAQ.cpp : 实现文件
//
#include "ART_ArtDAQ.h"
#include "./utils/TimeUtils.h"
#include <fstream>

#define ArtDAQErrChk(functionCall) if( ArtDAQFailed(error=(functionCall)) ) goto Error; else

int32 ART_CALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData);

TaskHandle  m_lightTask, m_voltOutTask, m_readADTask, m_hardTriggerTask, m_extraVoltOutTask, m_readCITask, m_DIListenerTask;

// CART_ArtDAQ

CART_ArtDAQ::CART_ArtDAQ()
{
	m_bDATaskCreated = false;
	m_bExtraDATaskCreated = false;
	m_artListener = NULL;
	m_dInputVolt = 0;
	m_showImageTs = 0;
	m_pPhysicalChannel = NULL;
	m_pNameToAssignToChannel = NULL;
	m_iDANum = 0;
	m_iADNum = 0;
	m_iExtraNum = 0;
	m_showVolt = 0;
}

CART_ArtDAQ::~CART_ArtDAQ()
{
	Release();
	if (m_vDATaskData)
	{
		delete[] m_vDATaskData;
		m_vDATaskData = NULL;
	}
	if (m_vExtraDATaskData)
	{
		delete[] m_vExtraDATaskData;
		m_vExtraDATaskData = NULL;
	}
}

void CART_ArtDAQ::Release()
{
	ArtDAQ_StopTask(m_lightTask);
	ArtDAQ_ClearTask(m_lightTask);

	ReleaseDATask();

	ReleaseADTask();

	ArtDAQ_StopTask(m_hardTriggerTask);
	ArtDAQ_ClearTask(m_hardTriggerTask);

	ReleaseCITask();
}

void CART_ArtDAQ::RegisterListener(CARTListener* listener)
{
	m_artListener = listener;
}

bool CART_ArtDAQ::CreateADTask(uint32_t artIndex, bool* bOpen, double* dVolt)
{
	int32       error = -1;
	char        errBuff[2048] = { '\0' };

	const char* m_pPhysicalChannel;
	const char* m_pNameToAssignToChannel;
	m_iADNum = 0;
	ArtDAQErrChk(ArtDAQ_CreateTask("Task2", &m_readADTask));
	for (int i = 0; i < 16; i++)
	{
		if (!bOpen[i])
		{
			continue;
		}
		m_pNameToAssignToChannel = m_pPhysicalChannel = NumToName(i, 0);
		if (i == 15)
		{
			ArtDAQErrChk(ArtDAQ_CreateAIVoltageChan(m_readADTask, m_pPhysicalChannel, m_pNameToAssignToChannel, ArtDAQ_Val_RSE, -10.0, 10.0, ArtDAQ_Val_Volts, NULL));
		}
		else
		{
			ArtDAQErrChk(ArtDAQ_CreateAIVoltageChan(m_readADTask, m_pPhysicalChannel, m_pNameToAssignToChannel, ArtDAQ_Val_RSE, dVolt[0], dVolt[1], ArtDAQ_Val_Volts, NULL));
		}
		m_iADNum++;
	}
	//ArtDAQErrChk(ArtDAQ_CfgSampClkTiming(m_readADTask, "", 10000.0, ArtDAQ_Val_Rising, ArtDAQ_Val_ContSamps, 8));
	ArtDAQErrChk(ArtDAQ_StartTask(m_readADTask));

	return TRUE;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);

	if (m_readADTask != 0) {
		ArtDAQ_StopTask(m_readADTask);
		ArtDAQ_ClearTask(m_readADTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return FALSE;
}

bool CART_ArtDAQ::CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt)
{
	int32       error = -1;
	char        errBuff[2048] = { '\0' };

	const char* m_pPhysicalChannel;
	const char* m_pNameToAssignToChannel;
	for (int i = 0; i < DACHANNEL_SIZE; i++)
	{
		if ((i >= 0 && i <= 3) && bOpen[i])
		{
			m_iDANum++;
			m_bDATaskCreated = true;
		}
		else if ((i >= 4 && i <= 7) && bOpen[i])
		{
			m_bExtraDATaskCreated = true;
		}
	}
	if (m_iDANum <= 0)
	{
		return false;
	}
	delete[] m_vDATaskData;
	m_vDATaskData = new float64[m_iDANum];
	if (m_bDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_CreateTask("Task3", &m_voltOutTask));
	}
	if (m_bExtraDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_CreateTask("Task3_1", &m_extraVoltOutTask));
	}

	for (int i = 0; i < DACHANNEL_SIZE; i++)
	{
		if (!bOpen[i])
		{
			continue;
		}
		m_pNameToAssignToChannel = m_pPhysicalChannel = NumToName(i, 1);
		if (i >= 0 && i <= 3)
		{
			ArtDAQErrChk(ArtDAQ_CreateAOVoltageChan(m_voltOutTask, m_pPhysicalChannel, m_pNameToAssignToChannel, dVolt[0], dVolt[1], ArtDAQ_Val_Volts, NULL));
		}
		else if (i >= 4 && i <= 7)
		{
			ArtDAQErrChk(ArtDAQ_CreateAOVoltageChan(m_extraVoltOutTask, m_pPhysicalChannel, m_pNameToAssignToChannel, dVolt[0], dVolt[1], ArtDAQ_Val_Volts, NULL));
			m_iExtraNum++;
		}
	}
	if (m_bDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_StartTask(m_voltOutTask));
	}
	if (m_bExtraDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_StartTask(m_extraVoltOutTask));
	}
	//Context::getInstance()->m_bDATaskCreated = true;
	return TRUE;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);

	if (m_voltOutTask != 0 && m_bDATaskCreated) {
		ArtDAQ_StopTask(m_voltOutTask);
		ArtDAQ_ClearTask(m_voltOutTask);
		m_bDATaskCreated = false;
		m_iDANum = 0;
	}
	if (m_extraVoltOutTask != 0 && m_bExtraDATaskCreated) {
		ArtDAQ_StopTask(m_extraVoltOutTask);
		ArtDAQ_ClearTask(m_extraVoltOutTask);
		m_bExtraDATaskCreated = false;
		m_iExtraNum = 0;
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return FALSE;
}

bool CART_ArtDAQ::CreateDITask(uint32_t artIndex)
{
	int32       error = -1;
	char        errBuff[2048] = { '\0' };

	const char* m_pPhysicalChannel;
	const char* m_pNameToAssignToChannel;
	ArtDAQErrChk(ArtDAQ_CreateTask("Task6", &m_DIListenerTask));

	m_pNameToAssignToChannel = m_pPhysicalChannel = NumToName(15, 0);
	ArtDAQErrChk(ArtDAQ_CreateAIVoltageChan(m_DIListenerTask, m_pPhysicalChannel, m_pNameToAssignToChannel, ArtDAQ_Val_RSE, -10.0, 10.0, ArtDAQ_Val_Volts, NULL));
	ArtDAQErrChk(ArtDAQ_StartTask(m_DIListenerTask));
	return true;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);

	if (m_DIListenerTask != 0) {
		ArtDAQ_StopTask(m_DIListenerTask);
		//ArtDAQ_ClearTask(m_DIListenerTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return false;
}

bool CART_ArtDAQ::ReleaseADTask()
{

	if (m_readADTask != NULL)
	{
		ArtDAQ_WaitUntilTaskDone(m_readADTask, 1);
		ArtDAQ_StopTask(m_readADTask);
		ArtDAQ_ClearTask(m_readADTask);
	}
	m_iADNum = 0;
	//Context::getInstance()->m_bADTaskCreated = false;
	//Context::getInstance()->m_bADVoltRefreshing = false;
	return true;
}

bool CART_ArtDAQ::ReleaseDATask()
{
	if (m_voltOutTask != NULL && m_bDATaskCreated)
	{
		ArtDAQ_StopTask(m_voltOutTask);
		ArtDAQ_ClearTask(m_voltOutTask);
		m_bDATaskCreated = false;
		m_iDANum = 0;
	}
	if (m_extraVoltOutTask != NULL && m_bExtraDATaskCreated)
	{
		ArtDAQ_StopTask(m_extraVoltOutTask);
		ArtDAQ_ClearTask(m_extraVoltOutTask);
		m_bExtraDATaskCreated = false;
		m_iExtraNum = 0;
	}
	//Context::getInstance()->m_bDATaskCreated = false;
	return true;
}

bool CART_ArtDAQ::ReleaseHardTriggerTask()
{
	if (m_hardTriggerTask != NULL)
	{
		ArtDAQ_StopTask(m_hardTriggerTask);
		ArtDAQ_ClearTask(m_hardTriggerTask);
	}

	return true;
}

bool CART_ArtDAQ::ReleaseDITask()
{
	if (m_DIListenerTask != NULL)
	{
		ArtDAQ_StopTask(m_DIListenerTask);
		ArtDAQ_ClearTask(m_DIListenerTask);
	}
	return true;
}

// 光源
//bool CART_ArtDAQ::SetDO(BOOL Para[])
//{
//	int32       error = -1;
//	uInt8       data[8] = {};
//	data[0] = Para[0];
//	data[1] = Para[1];
//	data[2] = Para[2];
//	data[3] = Para[3];
//	data[4] = Para[4];
//	data[5] = Para[5];
//	data[6] = Para[6];
//	data[7] = Para[7];
//	char        errBuff[2048] = { '\0' };
//
//	ArtDAQErrChk(ArtDAQ_CreateTask("Task1", &m_lightTask));
//	ArtDAQErrChk(ArtDAQ_CreateDOChan(m_lightTask, "Dev1/port0/line0:7", "", ArtDAQ_Val_ChanForAllLines));
//	ArtDAQErrChk(ArtDAQ_StartTask(m_lightTask));
//	ArtDAQErrChk(ArtDAQ_WriteDigitalLines(m_lightTask, 1, 1, 10.0, ArtDAQ_Val_GroupByChannel, data, NULL, NULL));
//
//	ArtDAQErrChk(ArtDAQ_StopTask(m_lightTask));
//	ArtDAQErrChk(ArtDAQ_ClearTask(m_lightTask));
//
//	return true;
//
//Error:
//	if (ArtDAQFailed(error))
//		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
//
//	if (m_lightTask != 0) {
//		ArtDAQ_StopTask(m_lightTask);
//		//ArtDAQ_ClearTask(m_lightTask);
//	}
//	if (ArtDAQFailed(error))
//	{
//		printf("ArtDAQ_ Error: %s\n", errBuff);
//	}
//	return false;
//}

// 接收AD数据
bool CART_ArtDAQ::ReadDeviceAD(double* pGainData, bool bVoltInputExe[])
{
	if (m_iADNum <= 0)
		return false;
	int32       error = -1;
	int32       read = 0;
	char        errBuff[2048] = { '\0' };
	double* dGainData = new double[16];
	double* dShowData = new double[16];
	for (int i = 0; i < 16; i++)
	{
		dGainData[i] = 0;
		dShowData[i] = 0;
	}
	qDebug() << "开始输入";
	ArtDAQErrChk(ArtDAQ_ReadAnalogF64(m_readADTask, -1, 10.0, ArtDAQ_Val_GroupByScanNumber, dGainData, m_iADNum, &read, NULL));
	//***重新对应通道数据***
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
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	qDebug() << "输入错误";


	if (m_readADTask != 0) {
		ArtDAQ_StopTask(m_readADTask);
		//ArtDAQ_ClearTask(m_readADTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_Error: %s\n", errBuff);
	}
	delete[] dGainData;
	dGainData = nullptr;
	delete[] dShowData;
	dShowData = nullptr;
	return false;
}

bool CART_ArtDAQ::ReadCtrlAD(double* pGainData)
{
	int32       error = 0;
	int32       read;
	char        errBuff[2048] = { '\0' };


	qDebug() << "开始输入";
	ArtDAQErrChk(ArtDAQ_ReadAnalogF64(m_DIListenerTask, -1, 10.0, ArtDAQ_Val_GroupByChannel, pGainData, 1, &read, NULL));
	qDebug() << "输入值" << *pGainData;
	if (read <= 0)
	{
		*pGainData = 0.0;
	}
	return true;
Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	qDebug() << "输入DI错误";


	if (m_DIListenerTask != 0) {
		ArtDAQ_StopTask(m_DIListenerTask);
		//ArtDAQ_ClearTask(m_ReadTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return false;
}

// 获得AD通道数目
int CART_ArtDAQ::GetADChannalNum(void)
{
	return ADNUM;
}

// 输出AD数据
bool CART_ArtDAQ::VolOut(double dVolValue)
{
	int32       error = -1;
	float64     data[1]{};
	char        errBuff[2048] = { '\0' };

	//for (int i = 0; i < 100; i++)
	data[0] = dVolValue;
	qDebug() << "开始输出";

	//ArtDAQ_StartTask(m_voltOutTask);
	if (m_bDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_WriteAnalogF64(m_voltOutTask, 1, 0, 10.0, ArtDAQ_Val_GroupByChannel, data, NULL, NULL));
	}
	else if (m_bExtraDATaskCreated)
	{
		ArtDAQErrChk(ArtDAQ_WriteAnalogF64(m_extraVoltOutTask, 1, 0, 10.0, ArtDAQ_Val_GroupByChannel, data, NULL, NULL));
	}
	qDebug() << dVolValue;
	//ArtDAQErrChk(ArtDAQ_StopTask(m_voltOutTask));
	qDebug() << "输出成功";
	return true;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	if (m_voltOutTask != 0) {
		ArtDAQ_StopTask(m_voltOutTask);
		qDebug() << "输出失败";
		//ArtDAQ_ClearTask(m_voltOutTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return false;
}

bool CART_ArtDAQ::VolOut(double dVolArray[], bool bVoltOutputExe[])
{
	int32       error = -1;
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
	if (bOpened)
	{
		ArtDAQErrChk(ArtDAQ_WriteAnalogF64(m_voltOutTask, 1, 0, 10.0, ArtDAQ_Val_GroupByScanNumber, m_vDATaskData, NULL, NULL));
	}
	if (bOpened1)
	{
		ArtDAQErrChk(ArtDAQ_WriteAnalogF64(m_extraVoltOutTask, 1, 0, 10.0, ArtDAQ_Val_GroupByScanNumber, m_vExtraDATaskData, NULL, NULL));
	}
	//qDebug() << data[0] << '\n' << data[1];
	//ArtDAQErrChk(ArtDAQ_StopTask(m_voltOutTask));
	qDebug() << "输出成功";
	return true;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	if (m_voltOutTask != 0) {
		ArtDAQ_StopTask(m_voltOutTask);
		qDebug() << "输出失败";
		//ArtDAQ_ClearTask(m_voltOutTask);
	}
	if (ArtDAQFailed(error))
	{
		printf("ArtDAQ_ Error: %s\n", errBuff);
	}
	return false;
}

extern int32 ART_CALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData)
{
	int32   error = -1;
	char    errBuff[2048] = { '\0' };

	// Check to see if an error stopped the task.
	ArtDAQErrChk(status);

Error:
	if (ArtDAQFailed(error)) {
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
		ArtDAQ_ClearTask(taskHandle);
		printf("ArtDAQ Error: %s\n", errBuff);
	}
	return 0;
}

const char* CART_ArtDAQ::NumToName(int iUsedChannel, int type)
{
	const char* pChannelName = "";
	switch (type)
	{
	case 0:
		switch (iUsedChannel)
		{
		case 0:
			pChannelName = "Dev1/ai0";
			break;
		case 1:
			pChannelName = "Dev1/ai1";
			break;
		case 2:
			pChannelName = "Dev1/ai2";
			break;
		case 3:
			pChannelName = "Dev1/ai3";
			break;
		case 4:
			pChannelName = "Dev1/ai4";
			break;
		case 5:
			pChannelName = "Dev1/ai5";
			break;
		case 6:
			pChannelName = "Dev1/ai6";
			break;
		case 7:
			pChannelName = "Dev1/ai7";
			break;
		case 8:
			pChannelName = "Dev1/ai8";
			break;
		case 9:
			pChannelName = "Dev1/ai9";
			break;
		case 10:
			pChannelName = "Dev1/ai10";
			break;
		case 11:
			pChannelName = "Dev1/ai11";
			break;
		case 12:
			pChannelName = "Dev1/ai12";
			break;
		case 13:
			pChannelName = "Dev1/ai13";
			break;
		case 14:
			pChannelName = "Dev1/ai14";
			break;
		case 15:
			pChannelName = "Dev1/ai15";
			break;
		default:
			break;
		}
		break;
	case 1:
		switch (iUsedChannel)
		{
		case 0:
			pChannelName = "Dev1/ao0";
			break;
		case 1:
			pChannelName = "Dev1/ao1";
			break;
		case 2:
			pChannelName = "Dev1/ao2";
			break;
		case 3:
			pChannelName = "Dev1/ao3";
			break;
		case 4:
			pChannelName = "Dev2/ao0";
			break;
		case 5:
			pChannelName = "Dev2/ao1";
			break;
		case 6:
			pChannelName = "Dev2/ao2";
			break;
		case 7:
			pChannelName = "Dev2/ao3";
			break;
		default:
			break;
		}
		break;
	}

	return pChannelName;
}

bool CART_ArtDAQ::CreateCITask()
{
	int         error = 0;
	char        errBuff[2048] = { '\0' };

	/*********************************************/
	// ArtDAQ Configure Code
	/*********************************************/
	ArtDAQErrChk(ArtDAQ_CreateTask("Task5", &m_readCITask));
	ArtDAQErrChk(ArtDAQ_CreateCICountEdgesChan(m_readCITask, "Dev1/ctr0", "", ArtDAQ_Val_Rising, 0, ArtDAQ_Val_CountUp));

	/*********************************************/
	// ArtDAQ Start Code
	/*********************************************/
	ArtDAQErrChk(ArtDAQ_StartTask(m_readCITask));

	return true;
Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	if (m_readCITask != 0) {
		/*********************************************/
		// ArtDAQ Stop Code
		/*********************************************/
		ArtDAQ_StopTask(m_readCITask);
		ArtDAQ_ClearTask(m_readCITask);
	}
	return false;
}

bool CART_ArtDAQ::ReleaseCITask()
{
	if (m_readCITask != NULL)
	{
		ArtDAQ_StopTask(m_readCITask);
		ArtDAQ_ClearTask(m_readCITask);
	}
	return true;
}

uInt32 CART_ArtDAQ::ReadCIFreq()
{
	int         error = 0;
	uInt32		data;
	char        errBuff[2048] = { '\0' };

	ArtDAQErrChk(ArtDAQ_ReadCounterScalarU32(m_readCITask, 1, &data, 0));

	return data;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);
	//if (m_readCITask != 0) {
	//	/*********************************************/
	//	// ArtDAQ Stop Code
	//	/*********************************************/
	//	ArtDAQ_StopTask(m_readCITask);
	//	ArtDAQ_ClearTask(m_readCITask);
	//}
	return 0;

}

// 硬触发
bool CART_ArtDAQ::HardTrigger(double dValue, double dutyCycle)
{
	int         error;
	float64 freq = float64(dValue);
	//float64 duty = 0.5;
	//float64 frequency[1000] = { freq };
	//float64 dutyCycle[1000] = { duty };
	//for (int i = 0; i < 1000; i++)
	//for (int i = 0; i < 1000; i++)
	//for (int i = 0; i < 1000; i++)
	//for (int i = 0; i < 1000; i++)
	//{
	//	frequency[i] = freq;
	//	dutyCycle[i] = duty;
	//}
	char        errBuff[2048] = { '\0' };
	//TaskHandle m_hardTriggerTask;
	if (m_hardTriggerTask)
	{
		ArtDAQ_StopTask(m_hardTriggerTask);
		ArtDAQ_ClearTask(m_hardTriggerTask);
	}
	ArtDAQErrChk(ArtDAQ_CreateTask("Task4", &m_hardTriggerTask));
	ArtDAQErrChk(ArtDAQ_CreateCOPulseChanFreq(m_hardTriggerTask, "Dev1/ctr0", "", ArtDAQ_Val_Hz, ArtDAQ_Val_High, 0.0, freq, dutyCycle));//6相机DaHua-CXP占空比为0.09,通用0.1111
	ArtDAQErrChk(ArtDAQ_CfgImplicitTiming(m_hardTriggerTask, ArtDAQ_Val_ContSamps, 1000));
	ArtDAQErrChk(ArtDAQ_RegisterDoneEvent(m_hardTriggerTask, 0, DoneCallback, NULL));
	ArtDAQErrChk(ArtDAQ_StartTask(m_hardTriggerTask));

	return true;

Error:
	if (ArtDAQFailed(error))
		ArtDAQ_GetExtendedErrorInfo(errBuff, 2048);

	if (m_hardTriggerTask != 0) {
		ArtDAQ_StopTask(m_hardTriggerTask);
		ArtDAQ_ClearTask(m_hardTriggerTask);
	}
	if (ArtDAQFailed(error))
		printf("ArtDAQ_ Error: %s\n", errBuff);

	return false;
}
