#include "PulseComm.h"
#include "pulse/AMCXXE/USB_AMC2XE/Usb_AMC2XE_Dll.h"
#include "pulse/AMCXXE/NET_AMC3XER/NET_AMC3XER.h"
#include <thread>
#include <cstring> 
PulseComm::PulseComm()
{
	initParameters();
	initInfo();
}

PulseComm::~PulseComm()
{
	if (m_PulseInfo.cPulseIPAddress)
	{
		//delete[]m_PulseInfo.cPulseIPAddress;
		m_PulseInfo.cPulseIPAddress = nullptr;
	}
}

void PulseComm::SetToDefault()
{
	initInfo();
}

void PulseComm::ConnectSlots()
{
}

bool PulseComm::SetSysParInfo(void* ptr)
{
	if (ptr)
	{
		m_PulseInfo = *(static_cast<PulseInfo*>(ptr));
		return true;
	}
	return false;
}

bool PulseComm::GetSysParInfo(void* ptr) const
{
	if (ptr)
	{
		// 将 void* 转换为 SocketInfo* 并赋值
		PulseInfo* InfoPtr = static_cast<PulseInfo*>(ptr);
		*InfoPtr = m_PulseInfo;
		return true;
	}
	return false;
}

void PulseComm::setParameter(const QString& key, const Any& value)
{
	auto it = setters.find(key);
	if (it != setters.end()) {
		it->second(value);
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

Any PulseComm::getParameter(const QString& key) const
{
	auto it = getters.find(key);
	if (it != getters.end()) {
		return it->second();
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

bool PulseComm::setExternalFreq(const int& freq)
{
	return false;
}

bool PulseComm::Connect(int type)
{
	int iOpen = OpenAxs();
	if (iOpen == -1)
	{
		return false;
	}
	m_PulseInfo.bPulseOpened = true;
	//初始化
	InitAxs();
	return true;
}

bool PulseComm::Disconnect(int type)
{
	CloseAxs();
	m_PulseInfo.bPulseOpened = false;
	return	true;
}

void PulseComm::SendData(const std::vector<double>& vData)
{
	byte RunState;  //运动状态
	bool bOutputed = false;
	while (!bOutputed)
	{
		int success = ReadPos(RunState);
		if (success != 0)
		{
			m_PulseInfo.bPulseSending = false;
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::PulseView_ID, W_CUSTOM_COMM_UPDATEPULSECTRL);
			return;
		}
		else
		{
			if (RunState == 0)
				bOutputed = true;
		}
	}

	int iTemp = (int)(vData[0] / m_PulseInfo.dAccuracy);
	double dTransValue = iTemp * m_PulseInfo.dAccuracy;
	double diff = dTransValue - m_dTransValue;
	m_dTransValue = dTransValue;
	int iPulseMin = 0, iPulseMax = 0;
	if (0 == m_PulseInfo.iPulseCardType)
	{
		iPulseMin = -200000;
		iPulseMax = 200000;
	}
	else if (1 == m_PulseInfo.iPulseCardType)
	{
		iPulseMin = -2600000;
		iPulseMax = 2600000;
	}

	int newValue = QString::number(diff / m_PulseInfo.dAccuracy).toInt() * 2;
	const 	double& dSendFreq = m_PulseInfo.dFrameRate;
	if (newValue <= -0.1 || newValue >= 0.1)
	{
		if (newValue * dSendFreq < iPulseMin)
		{
			newValue = (int)(iPulseMin / dSendFreq);
		}
		else if (newValue * dSendFreq > iPulseMax)
		{
			newValue = (int)(iPulseMax / dSendFreq);
		}
		int iDir = newValue > 0 ? 0 : 1;
		int iPulseNum = abs(newValue);
		if (iPulseNum % 2 == 1)
		{
			iPulseNum += 1;
		}
		DeltMove(iDir, iPulseNum);
	}
}

bool PulseComm::SaveParInfo(const QString& filename)
{
	std::ofstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing");
		return false;
	}

	// 写入结构体数据
	file.write(reinterpret_cast<const char*>(&m_PulseInfo), sizeof(m_PulseInfo));

	file.close();
	return true;
}

bool PulseComm::LoadParInfo(const QString& filename)
{
	std::ifstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for reading");
		return false;
	}

	// 读取结构体数据
	file.read(reinterpret_cast<char*>(&m_PulseInfo), sizeof(m_PulseInfo));
	file.close();
	m_PulseInfo.bPulseOpened = false;
	m_PulseInfo.bPulseSending = false;

	m_PulseInfo.input.iChannelState = 0;
	m_PulseInfo.input.iChannelSize = 1;
	m_PulseInfo.input.iTriggerCtrl = 0;
	m_PulseInfo.input.iTransferType = 0;
	for (int i = 0; i < 4; i++)
	{
		m_PulseInfo.input.iDataType[i] = 0;
		m_PulseInfo.input.iUnit[i] = 0;
	}

	m_PulseInfo.output.iChannelState = 0;
	m_PulseInfo.output.iChannelSize = 1;
	m_PulseInfo.output.iTriggerCtrl = 0;
	m_PulseInfo.output.iTransferType = 0;
	for (int i = 0; i < 8; i++)
	{
		m_PulseInfo.output.iDataModel[i] = 0;
		m_PulseInfo.output.iDataType[i] = 0;
		m_PulseInfo.output.iData[i] = 0;
		m_PulseInfo.output.bReverse[i] = 0;
		//m_PulseInfo.output.iUnit[i] = 0;
	}

	return true;
}

bool PulseComm::saveCmdToFile(const std::string& filename)
{
	return false;
}

bool PulseComm::loadCmdFromFile(const std::string& filename)
{
	return false;
}

void PulseComm::initParameters()
{
	setters["iPulseCardType"] = [this](const Any& value) { m_PulseInfo.iPulseCardType = value.any_cast<int>(); };
	setters["bPulseOpened"] = [this](const Any& value) { m_PulseInfo.bPulseOpened = value.any_cast<bool>(); };
	setters["bPulseSending"] = [this](const Any& value) { m_PulseInfo.bPulseSending = value.any_cast<bool>(); };
	setters["dAccuracy"] = [this](const Any& value) { m_PulseInfo.dAccuracy = value.any_cast<double>(); };
	setters["dCaliValue"] = [this](const Any& value) { m_PulseInfo.dCaliValue = value.any_cast<double>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.dFrameRate = value.any_cast<double>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.cPulseIPAddress = value.any_cast<char*>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.iUSBVo = value.any_cast<int>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.iUSBVt = value.any_cast<int>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.iNETVo = value.any_cast<int>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.iNETVt = value.any_cast<int>(); };

	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.input = value.any_cast<DataInput>(); };
	setters["dFrameRate"] = [this](const Any& value) { m_PulseInfo.output = value.any_cast<DataOutput>(); };


	getters["iPulseCardType"] = [this]() -> Any { return m_PulseInfo.iPulseCardType; };
	getters["bPulseOpened"] = [this]() -> Any { return m_PulseInfo.bPulseOpened; };
	getters["bPulseSending"] = [this]() -> Any { return m_PulseInfo.bPulseSending; };
	getters["dAccuracy"] = [this]() -> Any { return m_PulseInfo.dAccuracy; };
	getters["dCaliValue"] = [this]() -> Any { return m_PulseInfo.dCaliValue; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.dFrameRate; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.cPulseIPAddress; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.iUSBVo; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.iUSBVt; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.iNETVo; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.iNETVt; };

	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.input; };
	getters["dFrameRate"] = [this]() -> Any { return m_PulseInfo.output; };
}

void PulseComm::initInfo()
{
	m_PulseInfo.cPulseIPAddress = new char[16];
	// 使用 strcpy 将字符串复制到目标指针
	strcpy_s(m_PulseInfo.cPulseIPAddress, 16, "192.168.1.30");
	m_PulseInfo.iPulseCardType = 0;
	m_PulseInfo.bPulseOpened = false;
	m_PulseInfo.bPulseSending = false;
	m_PulseInfo.dAccuracy = 0.01;
	m_PulseInfo.dCaliValue = 0.01;
	m_PulseInfo.dFrameRate = 20;
	m_PulseInfo.iUSBVo = 1000;
	m_PulseInfo.iUSBVt = 200000;
	m_PulseInfo.iNETVo = 200000;
	m_PulseInfo.iNETVt = 2600000;

	m_PulseInfo.input.iChannelState = 0;
	m_PulseInfo.input.iChannelSize = 1;
	m_PulseInfo.input.iTriggerCtrl = 0;
	m_PulseInfo.input.iTransferType = 0;
	for (int i = 0; i < 4; i++)
	{
		m_PulseInfo.input.iDataType[i] = 0;
		m_PulseInfo.input.iUnit[i] = 0;
	}

	m_PulseInfo.output.iChannelState = 0;
	m_PulseInfo.output.iChannelSize = 1;
	m_PulseInfo.output.iTriggerCtrl = 0;
	m_PulseInfo.output.iTransferType = 0;
	for (int i = 0; i < 8; i++)
	{
		m_PulseInfo.output.iDataModel[i] = 0;
		m_PulseInfo.output.iDataType[i] = 0;
		m_PulseInfo.output.iData[i] = 0;
		m_PulseInfo.output.bReverse[i] = 0;
		//m_PulseInfo.output.iUnit[i] = 0;
	}

	m_dTransValue = 0;
}

void PulseComm::SendCaliData()
{
	int iPulseNum = m_PulseInfo.dCaliValue / m_PulseInfo.dAccuracy * 2;
	// 发送标定数值
	DeltMove(0, iPulseNum);
	std::thread(&PulseComm::UpdatePos, this).detach();
}

void PulseComm::UpdatePos()
{
	if (!m_PulseInfo.bPulseOpened)
	{
		emit emitEventMsg(0, CommViewID::PulseView_ID, W_CUSTOM_COMM_PULSEBUTTON);
		return;
	}
	byte RunState;  //运动状态
	bool bOutputed = false;
	while (!bOutputed)
	{
		int success = ReadPos(RunState);
		if (!success)
		{
			if (RunState == 0)
				bOutputed = true;
		}
		else
		{
			emit emitEventMsg(m_PulseInfo.bPulseOpened, CommViewID::PulseView_ID, W_CUSTOM_COMM_PULSEBUTTON);
			return;
		}
	}
	//标定完成后回复初始位
	InitAxs();
	emit emitEventMsg(m_PulseInfo.bPulseOpened, CommViewID::PulseView_ID, W_CUSTOM_COMM_PULSEBUTTON);
	emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::PulseView_ID, W_CUSTOM_COMM_PULSECALIDONE);
}

int PulseComm::OpenAxs()
{
	int iOpen = 0;
	if (m_PulseInfo.iPulseCardType == 0)
	{
		iOpen = OpenUSB_2XE();
	}
	else if (m_PulseInfo.iPulseCardType == 1)
	{
		iOpen = SOCKET_init();
	}
	return iOpen;
}

void PulseComm::InitAxs()
{
	//Set_Axs函数调用 2 次，该位参数第一次设为 0，第二次设为 1，由 0 转 1 的上升沿瞬间，会使得逻辑位置计数值初始化为 0
	if (m_PulseInfo.iPulseCardType == 0)
	{
		Set_Axs_2XE(0, 0, 0, 0, 0, 0);
		Set_Axs_2XE(0, 0, 1, 0, 0, 0);
	}
	else if (m_PulseInfo.iPulseCardType == 1)
	{
		Set_Axs((char*)m_PulseInfo.cPulseIPAddress, 0, 0, 0, 0, 0);
		Set_Axs((char*)m_PulseInfo.cPulseIPAddress, 0, 1, 0, 0, 0);
	}
}

int PulseComm::ReadPos(byte& RunState)
{
	byte IOState;  //极限位置状态
	byte CEMG;  //公共急停输入 CEMG 状态
	uint iPos;   //逻辑位置
	int success = 0;
	if (m_PulseInfo.iPulseCardType == 0)
	{
		success = Read_Position_2XE(0, 0, &iPos, &RunState, &IOState, &CEMG);
	}
	else if (m_PulseInfo.iPulseCardType == 1)
	{
		success = Read_Position((char*)m_PulseInfo.cPulseIPAddress, 0, &iPos, &RunState, &IOState, &CEMG);
	}
	return success;
}

void PulseComm::DeltMove(int iDir, int iPulseNum)
{
	int iRet = 0;
	if (0 == m_PulseInfo.iPulseCardType)
	{
		iRet = DeltMov_2XE(0, 0, 0, iDir, 6, m_PulseInfo.iUSBVo, m_PulseInfo.iUSBVt, iPulseNum, 0, 1, 1);
	}
	else if (1 == m_PulseInfo.iPulseCardType)
	{
		iRet = DeltMov((char*)m_PulseInfo.cPulseIPAddress, 0, 0, iDir, 5, m_PulseInfo.iNETVo, m_PulseInfo.iNETVt, iPulseNum, 0, 1, 4, 0, 0);
	}
}

void PulseComm::CloseAxs()
{
	if (m_PulseInfo.iPulseCardType == 0)
	{
		CloseUSB_2XE();
	}
	else if (m_PulseInfo.iPulseCardType == 1)
	{
		SOCKET_delete();
	}
}
