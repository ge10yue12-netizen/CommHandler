#include "VoltageComm.h"
#include <algorithm> // for std::transform

VoltageComm::VoltageComm()
{
	m_controlBox.OpenCtrlBox();

	initParameters();
	initInfo();

	ConnectSlots();
}

VoltageComm::~VoltageComm()
{
	m_controlBox.CloseCtrlBox();
}

void VoltageComm::SetToDefault()
{
	m_controlBox.CloseCtrlBox();
	initInfo();

	ArtType iArtType;
	switch (m_VoltageInfo.iControllerType)
	{
	case 0:
		iArtType = ART3134A;
		break;
		/*case 1:
			iArtType = ART5640_NET;
			break;*/
	case 1:
		iArtType = NI_;
		break;
	default:
		iArtType = UNKNOWN;
		break;
	}
	m_controlBox.SetBoxType(iArtType);
	m_controlBox.OpenCtrlBox();
}

void VoltageComm::ConnectSlots()
{
	// tcp
	//QObject::connect(&m_controlBox, &NetworkBox::emitNewConn, this, &SocketComm::onNewConnIpAndPortSlots);				// 新客户端连接到本地服务器
	//QObject::connect(&m_controlBox, &Voltage::emitNewMessage, this, &VoltageComm::onRecvDataSlots);				    // 接收到新消息触发
	//QObject::connect(&m_controlBox, &NetworkBox::emitDisConnected, this, &SocketComm::onClientDisConnSlots);			// TCP断开连接			
}

bool VoltageComm::SetSysParInfo(void* ptr)
{
	if (ptr)
	{
		m_VoltageInfo = *(static_cast<VoltageInfo*>(ptr));
		return true;
	}
	return false;
}

bool VoltageComm::GetSysParInfo(void* ptr) const
{
	if (ptr)
	{
		// 将 void* 转换为 SocketInfo* 并赋值
		VoltageInfo* InfoPtr = static_cast<VoltageInfo*>(ptr);
		*InfoPtr = m_VoltageInfo;
		return true;
	}
	return false;
}

void VoltageComm::setParameter(const QString& key, const Any& value)
{
	auto it = setters.find(key);
	if (it != setters.end()) {
		it->second(value);
		if (key == "dValueK")
		{
			// 解析 Any 为 std::array<double, 16>
			auto arr = value.any_cast<std::array<double, 16>>();

			// 赋值到 m_dValueK
			std::copy(arr.begin(), arr.end(), std::begin(m_controlBox.m_dValueK));
		}
		if (key == "dValueB")
		{
			// 解析 Any 为 std::array<double, 16>
			auto arr = value.any_cast<std::array<double, 16>>();

			// 赋值到 m_dValueB
			std::copy(arr.begin(), arr.end(), std::begin(m_controlBox.m_dValueB));
		}
		if (key == "iControllerType")
		{
			ArtType iArtType;
			switch (m_VoltageInfo.iControllerType)
			{
			case 0:
				iArtType = ART3134A;
				break;
				/*case 1:
					iArtType = ART5640_NET;
					break;*/
			case 1:
				iArtType = NI_;
				break;
			default:
				iArtType = UNKNOWN;
				break;
			}
			m_controlBox.SetBoxType(iArtType);
			m_controlBox.OpenCtrlBox();
		}
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

Any VoltageComm::getParameter(const QString& key) const
{
	auto it = getters.find(key);
	if (it != getters.end()) {
		return it->second();
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

bool VoltageComm::setExternalFreq(const int& freq)
{
	return false;
}

bool VoltageComm::Connect(int type)
{
	bool bflag = false;
	switch (type)
	{
	case 0:
	{
		bflag = m_controlBox.CreateADTask(0, m_VoltageInfo.bADExe, m_VoltageInfo.dADRange[m_VoltageInfo.iADChannel]);
		//m_VoltageInfo.bADOpen = bflag;
		break;
	}
	case 1:
	{
		bflag = m_controlBox.CreateDATask(m_VoltageInfo.iDAChannel, m_VoltageInfo.bDAExe, m_VoltageInfo.dDAVoltRange[m_VoltageInfo.iDAChannel]);
		//m_VoltageInfo.bDAOpen = bflag;
		break;
	}
	case 2:
	{
		m_controlBox.CreateCITask();
		break;
	}
	case 3:
	{
		m_controlBox.CreateDITask(0);
		break;
	}
	default: break;
	}

	//if (bflag)
	//{
	//	thread_Recv = std::thread(&VoltageComm::onRecvDataSlots, this);
	//	thread_Recv.detach();
	//}
	return bflag;
}

bool VoltageComm::Disconnect(int type)
{
	switch (type)
	{
	case 0:
	{
		m_VoltageInfo.bADExe[m_VoltageInfo.iADChannel] = false;
		bool bOpened = false;
		for (int i = 0; i < 2; i++)
		{
			if (m_VoltageInfo.bADExe[i])
			{
				bOpened = true;
				break;
			}
		}
		//m_VoltageInfo.bADOpen = bOpened;
		m_controlBox.ReleaseADTask();

		/*if (bOpened)
		{
			m_controlBox.CreateADTask(m_VoltageInfo.iADChannel, m_VoltageInfo.bADExe, m_VoltageInfo.dADRange[m_VoltageInfo.iADChannel]);
		}*/
		break;
	}
	case 1:
	{
		m_VoltageInfo.bDAExe[m_VoltageInfo.iDAChannel] = false;
		bool bOpened = false;
		for (int i = 0; i < 2; i++)
		{
			if (m_VoltageInfo.dDAVoltRange[i])
			{
				bOpened = true;
				break;
			}
		}
		//m_VoltageInfo.bDAOpen = bOpened;
		m_controlBox.ReleaseDATask();

		/*	if (bOpened)
			{
				m_controlBox.CreateDATask(m_VoltageInfo.iDAChannel, m_VoltageInfo.bDAExe, m_VoltageInfo.dDAVoltRange[m_VoltageInfo.iDAChannel]);
			}*/
		break;
	}
	case 2:
	{
		m_controlBox.ReleaseCITask();
		break;
	}
	case 3:
	{
		m_controlBox.ReleaseDITask();
		break;
	}
	case 4:
	{
		for (int i = 0; i < 16; i++)
		{
			m_VoltageInfo.bADExe[i] = false;
		}
		m_controlBox.ReleaseADTask();
		break;
	}
	case 5:
	{
		for (int i = 0; i < 16; i++)
		{
			m_VoltageInfo.bDAExe[i] = false;
		}
		m_controlBox.ReleaseDATask();
		break;
	}
	default: break;
	}

	/*if (type == 0)
	{
		m_VoltageInfo.bADExe[m_VoltageInfo.iADChannel] = false;
		bool bOpened = false;
		for (int i = 0; i < 2; i++)
		{
			if (m_VoltageInfo.bADExe[i])
			{
				bOpened = true;
				break;
			}
		}
		m_VoltageInfo.bADOpen = bOpened;
		m_controlBox.ReleaseADTask();

		if (bOpened)
		{
			m_controlBox.CreateADTask(m_VoltageInfo.iADChannel, m_VoltageInfo.bADExe, m_VoltageInfo.dADRange[m_VoltageInfo.iADChannel]);
		}
	}
	else if (type == 1)
	{
		m_VoltageInfo.bDAExe[m_VoltageInfo.iDAChannel] = false;
		bool bOpened = false;
		for (int i = 0; i < 2; i++)
		{
			if (m_VoltageInfo.dDAVoltRange[i])
			{
				bOpened = true;
				break;
			}
		}
		m_VoltageInfo.bDAOpen = bOpened;
		m_controlBox.ReleaseDATask();

		if (bOpened)
		{
			m_controlBox.CreateDATask(m_VoltageInfo.iDAChannel, m_VoltageInfo.bDAExe, m_VoltageInfo.dDAVoltRange[m_VoltageInfo.iDAChannel]);
		}
	}*/

	//m_controlBox.CloseCtrlBox();
	return true;
}

//void VoltageComm::SendData(void* data, size_t size)
//{
//	m_controlBox.WriteDAData(static_cast<double*>(data), m_VoltageInfo.bDAExe);
//}

void VoltageComm::SendData(const std::vector<double>& vData)
{
	if (vData.empty()) return; // 增加数据有效性检查

	const auto& info = m_VoltageInfo;  // 简化变量访问
	int channel = info.iDAChannel;

	double dMinVolt = info.vOutputMinVoltArray[channel];
	double dMaxVolt = info.vOutputMaxVoltArray[channel];
	double dMinValue = info.vMinValueArray[channel];
	double dMaxValue = info.vMaxValueArray[channel];
	double dScale = info.vScaleFactorArray[channel];

	std::vector<double> vDataConverted(vData.size());

	std::transform(vData.begin(), vData.end(), vDataConverted.begin(),
		[&](double val) -> double {
			// 限制输入值范围
			val = std::clamp(val, dMinValue, dMaxValue);

			// 转换为电压值
			double voltage = dScale * (val - dMinValue) + dMinVolt;

			// 再次限制输出电压范围
			return std::clamp(voltage, dMinVolt, dMaxVolt);
		});

	// 调用外部接口，vector转数组使用data()函数
	m_controlBox.WriteDAData(vDataConverted.data(), m_VoltageInfo.bDAExe);
}

bool VoltageComm::SaveParInfo(const QString& filename)
{
	std::ofstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	// 写入简单类型
	//file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iModel), sizeof(m_VoltageInfo.iModel));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iADInputMode), sizeof(m_VoltageInfo.iADInputMode));
	//file.write(reinterpret_cast<const char*>(&m_VoltageInfo.bADOpen), sizeof(m_VoltageInfo.bADOpen));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iADChannel), sizeof(m_VoltageInfo.iADChannel));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iADMode), sizeof(m_VoltageInfo.iADMode));

	// 写入 QString 类型数组 sADName
	for (int i = 0; i < 16; ++i) {
		QByteArray adNameBytes = m_VoltageInfo.sADName[i].toUtf8();
		int adNameLength = adNameBytes.size();
		file.write(reinterpret_cast<const char*>(&adNameLength), sizeof(adNameLength));
		file.write(adNameBytes.data(), adNameLength);
	}

	// 写入其他数组和简单类型
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.iADDataType), sizeof(m_VoltageInfo.iADDataType));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.iADUnit), sizeof(m_VoltageInfo.iADUnit));
	//file.write(reinterpret_cast<const char*>(m_VoltageInfo.bADExe), sizeof(m_VoltageInfo.bADExe));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.bAutoADExe), sizeof(m_VoltageInfo.bAutoADExe));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dADRange), sizeof(m_VoltageInfo.dADRange));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dCompensation), sizeof(m_VoltageInfo.dCompensation));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dVADCaliArray), sizeof(m_VoltageInfo.dVADCaliArray));

	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iThresholdType), sizeof(m_VoltageInfo.iThresholdType));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dAbnormalThreshold), sizeof(m_VoltageInfo.dAbnormalThreshold));
	//file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iForceUnit), sizeof(m_VoltageInfo.iForceUnit));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dValueK), sizeof(m_VoltageInfo.dValueK));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dValueB), sizeof(m_VoltageInfo.dValueB));

	// 写入 DA 数据
	//file.write(reinterpret_cast<const char*>(&m_VoltageInfo.bDAOpen), sizeof(m_VoltageInfo.bDAOpen));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iDAChannel), sizeof(m_VoltageInfo.iDAChannel));
	//file.write(reinterpret_cast<const char*>(m_VoltageInfo.bDAExe), sizeof(m_VoltageInfo.bDAExe));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.bAutoDAExe), sizeof(m_VoltageInfo.bAutoDAExe));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.dDAVoltRange), sizeof(m_VoltageInfo.dDAVoltRange));
	//file.write(reinterpret_cast<const char*>(m_VoltageInfo.strResult), sizeof(m_VoltageInfo.strResult));
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 4; ++j) {
			QByteArray bytes = m_VoltageInfo.strResult[i][j].toUtf8();
			qint32 len = bytes.size();
			file.write(reinterpret_cast<const char*>(&len), sizeof(len));
			file.write(bytes.data(), len);
		}
	}
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.vScaleFactorArray), sizeof(m_VoltageInfo.vScaleFactorArray));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.vOutputMinVoltArray), sizeof(m_VoltageInfo.vOutputMinVoltArray));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.vOutputMaxVoltArray), sizeof(m_VoltageInfo.vOutputMaxVoltArray));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.vMinValueArray), sizeof(m_VoltageInfo.vMinValueArray));
	file.write(reinterpret_cast<const char*>(m_VoltageInfo.vMaxValueArray), sizeof(m_VoltageInfo.vMaxValueArray));

	// 写入剩余简单数据
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iControllerType), sizeof(m_VoltageInfo.iControllerType));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iExternalTrigger), sizeof(m_VoltageInfo.iExternalTrigger));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iTriggerMode), sizeof(m_VoltageInfo.iTriggerMode));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.bTriggerVerify), sizeof(m_VoltageInfo.bTriggerVerify));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.iReadCI), sizeof(m_VoltageInfo.iReadCI));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.bOpenDI), sizeof(m_VoltageInfo.bOpenDI));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.input), sizeof(m_VoltageInfo.input));
	file.write(reinterpret_cast<const char*>(&m_VoltageInfo.output), sizeof(m_VoltageInfo.output));

	file.close();
	return true;
}

bool VoltageComm::LoadParInfo(const QString& filename)
{
	std::ifstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) {
		return false;
	}

	// 读取简单类型
	//file.read(reinterpret_cast<char*>(&m_VoltageInfo.iModel), sizeof(m_VoltageInfo.iModel));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iADInputMode), sizeof(m_VoltageInfo.iADInputMode));
	//file.read(reinterpret_cast<char*>(&m_VoltageInfo.bADOpen), sizeof(m_VoltageInfo.bADOpen));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iADChannel), sizeof(m_VoltageInfo.iADChannel));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iADMode), sizeof(m_VoltageInfo.iADMode));

	// 读取 QString 类型数组 sADName
	for (int i = 0; i < 16; ++i) {
		int adNameLength;
		file.read(reinterpret_cast<char*>(&adNameLength), sizeof(adNameLength)); // 读取长度
		QByteArray adNameBytes(adNameLength, 0);
		file.read(adNameBytes.data(), adNameLength); // 读取内容
		m_VoltageInfo.sADName[i] = QString::fromUtf8(adNameBytes);
	}

	// 读取其他数组和简单类型
	file.read(reinterpret_cast<char*>(m_VoltageInfo.iADDataType), sizeof(m_VoltageInfo.iADDataType));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.iADUnit), sizeof(m_VoltageInfo.iADUnit));
	//file.read(reinterpret_cast<char*>(m_VoltageInfo.bADExe), sizeof(m_VoltageInfo.bADExe));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.bAutoADExe), sizeof(m_VoltageInfo.bAutoADExe));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dADRange), sizeof(m_VoltageInfo.dADRange));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dCompensation), sizeof(m_VoltageInfo.dCompensation));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dVADCaliArray), sizeof(m_VoltageInfo.dVADCaliArray));

	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iThresholdType), sizeof(m_VoltageInfo.iThresholdType));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dAbnormalThreshold), sizeof(m_VoltageInfo.dAbnormalThreshold));
	//file.read(reinterpret_cast<char*>(&m_VoltageInfo.iForceUnit), sizeof(m_VoltageInfo.iForceUnit));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dValueK), sizeof(m_VoltageInfo.dValueK));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dValueB), sizeof(m_VoltageInfo.dValueB));

	// 读取 DA 数据
	//file.read(reinterpret_cast<char*>(&m_VoltageInfo.bDAOpen), sizeof(m_VoltageInfo.bDAOpen));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iDAChannel), sizeof(m_VoltageInfo.iDAChannel));
	//file.read(reinterpret_cast<char*>(m_VoltageInfo.bDAExe), sizeof(m_VoltageInfo.bDAExe));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.bAutoDAExe), sizeof(m_VoltageInfo.bAutoDAExe));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.dDAVoltRange), sizeof(m_VoltageInfo.dDAVoltRange));
	//file.read(reinterpret_cast<char*>(m_VoltageInfo.strResult), sizeof(m_VoltageInfo.strResult));
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 4; ++j) {
			qint32 len;
			file.read(reinterpret_cast<char*>(&len), sizeof(len));
			QByteArray bytes(len, Qt::Uninitialized);
			file.read(bytes.data(), len);
			m_VoltageInfo.strResult[i][j] = QString::fromUtf8(bytes);
		}
	}
	file.read(reinterpret_cast<char*>(m_VoltageInfo.vScaleFactorArray), sizeof(m_VoltageInfo.vScaleFactorArray));

	file.read(reinterpret_cast<char*>(m_VoltageInfo.vOutputMinVoltArray), sizeof(m_VoltageInfo.vOutputMinVoltArray));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.vOutputMaxVoltArray), sizeof(m_VoltageInfo.vOutputMaxVoltArray));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.vMinValueArray), sizeof(m_VoltageInfo.vMinValueArray));
	file.read(reinterpret_cast<char*>(m_VoltageInfo.vMaxValueArray), sizeof(m_VoltageInfo.vMaxValueArray));

	// 读取剩余简单数据
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iControllerType), sizeof(m_VoltageInfo.iControllerType));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iExternalTrigger), sizeof(m_VoltageInfo.iExternalTrigger));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iTriggerMode), sizeof(m_VoltageInfo.iTriggerMode));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.bTriggerVerify), sizeof(m_VoltageInfo.bTriggerVerify));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.iReadCI), sizeof(m_VoltageInfo.iReadCI));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.bOpenDI), sizeof(m_VoltageInfo.bOpenDI));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.input), sizeof(m_VoltageInfo.input));
	file.read(reinterpret_cast<char*>(&m_VoltageInfo.output), sizeof(m_VoltageInfo.output));


	file.close();

	m_VoltageInfo.bADOpen = false;
	m_VoltageInfo.bDAOpen = false;
	//m_VoltageInfo.bTriggerVerify = false;
	m_VoltageInfo.bOpenDI = false;

	//m_VoltageInfo.input.iChannelState = 0;
	//m_VoltageInfo.input.iChannelSize = 1;
	//m_VoltageInfo.input.iTriggerCtrl = 0;
	//m_VoltageInfo.input.iTransferType = 0;
	//for (int i = 0; i < 4; i++)
	//{
	//	m_VoltageInfo.input.iDataType[i] = 0;
	//	m_VoltageInfo.input.iUnit[i] = 0;
	//}

	//m_VoltageInfo.output.iChannelState = 0;
	//m_VoltageInfo.output.iChannelSize = 1;
	//m_VoltageInfo.output.iTriggerCtrl = 0;
	//m_VoltageInfo.output.iTransferType = 0;
	//for (int i = 0; i < 8; i++)
	//{
	//	m_VoltageInfo.output.iDataModel[i] = 0;
	//	m_VoltageInfo.output.iDataType[i] = 0;
	//	m_VoltageInfo.output.iData[i] = 0;
	//	m_VoltageInfo.output.bReverse[i] = 0;
	//	//m_VoltageInfo.output.iUnit[i] = 0;
	//}
	m_controlBox.CloseCtrlBox();
	ArtType iArtType;
	switch (m_VoltageInfo.iControllerType)
	{
	case 0:
		iArtType = ART3134A;
		break;
		/*case 1:
			iArtType = ART5640_NET;
			break;*/
	case 1:
		iArtType = NI_;
		break;
	default:
		iArtType = UNKNOWN;
		break;
	}
	m_controlBox.SetBoxType(iArtType);
	m_controlBox.OpenCtrlBox();

	return true;
}

bool VoltageComm::saveCmdToFile(const std::string& filename)
{
	return false;
}

bool VoltageComm::loadCmdFromFile(const std::string& filename)
{
	return false;
}

bool VoltageComm::HardTrigger(double value, double dutyCycle)
{
	return m_controlBox.HardTrigger(value, dutyCycle);
}

bool VoltageComm::ReleaseHardTrigger()
{
	return m_controlBox.ReleaseHardTrigger();
}

bool VoltageComm::ReadCIFreq()
{
	return m_controlBox.ReadCIFreq();
}

bool VoltageComm::ReadADData(double* pdData, double* pGainData, double* truthData, bool bGetVolt)
{
	return m_controlBox.ReadADData(pdData, pGainData, truthData, m_VoltageInfo.bADExe, m_VoltageInfo.dCompensation, bGetVolt);
}

void VoltageComm::initParameters()
{
	// Setters
	//setters["iModel"] = [this](const Any& value) {
	//	m_VoltageInfo.iModel = value.any_cast<int>();
	//	};

	setters["iADInputMode"] = [this](const Any& value) {
		m_VoltageInfo.iADInputMode = value.any_cast<int>();
		};

	setters["bADOpen"] = [this](const Any& value) {
		m_VoltageInfo.bADOpen = value.any_cast<bool>();
		};

	setters["iADChannel"] = [this](const Any& value) {
		m_VoltageInfo.iADChannel = value.any_cast<int>();
		};

	setters["sADName"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<QString, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.sADName));
		};

	setters["iADDataType"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<int, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.iADDataType));
		};

	setters["iADUnit"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<int, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.iADUnit));
		};

	setters["bADExe"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<bool, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.bADExe));
		};

	setters["bAutoADExe"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<bool, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.bAutoADExe));
		};

	setters["dADRange"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<std::array<double, 2>, 16>>();
		for (int i = 0; i < 16; ++i) {
			m_VoltageInfo.dADRange[i][0] = arr[i][0];
			m_VoltageInfo.dADRange[i][1] = arr[i][1];
		}
		};

	setters["dCompensation"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.dCompensation));
		};

	setters["dVADCaliArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<std::array<std::array<double, 2>, 10>, 16>>();
		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 10; ++j) {
				m_VoltageInfo.dVADCaliArray[i][j][0] = arr[i][j][0];
				m_VoltageInfo.dVADCaliArray[i][j][1] = arr[i][j][1];
			}
		}
		};

	setters["iThresholdType"] = [this](const Any& value) {
		m_VoltageInfo.iThresholdType = value.any_cast<int>();
		};

	setters["dAbnormalThreshold"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		m_VoltageInfo.dAbnormalThreshold[0] = arr[0];
		m_VoltageInfo.dAbnormalThreshold[1] = arr[1];
		};

	setters["dValueK"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.dValueK));
		};

	setters["dValueB"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 16>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.dValueB));
		};

	setters["bDAOpen"] = [this](const Any& value) {
		m_VoltageInfo.bDAOpen = value.any_cast<bool>();
		};

	setters["iDAChannel"] = [this](const Any& value) {
		m_VoltageInfo.iDAChannel = value.any_cast<int>();
		};

	setters["bDAExe"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<bool, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.bDAExe));
		};

	setters["bAutoDAExe"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<bool, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.bAutoDAExe));
		};

	setters["dDAVoltRange"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<std::array<double, 2>, 2>>();
		for (int i = 0; i < 2; ++i) {
			m_VoltageInfo.dDAVoltRange[i][0] = arr[i][0];
			m_VoltageInfo.dDAVoltRange[i][1] = arr[i][1];
		}
		};

	/*setters["strResult"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<std::array<QString, 4>, 2>>();
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 4; ++j) {
				m_VoltageInfo.strResult[i][j] = arr[i][j];
			}
		}
		};*/

	setters["vScaleFactorArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.vScaleFactorArray));
		};

	setters["vOutputMinVoltArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.vOutputMinVoltArray));
		};

	setters["vOutputMaxVoltArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.vOutputMaxVoltArray));
		};

	setters["vMinValueArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.vMinValueArray));
		};

	setters["vMaxValueArray"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<double, 2>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_VoltageInfo.vMaxValueArray));
		};

	setters["iControllerType"] = [this](const Any& value) {
		m_VoltageInfo.iControllerType = value.any_cast<int>();
		};

	setters["iExternalTrigger"] = [this](const Any& value) {
		m_VoltageInfo.iExternalTrigger = value.any_cast<int>();
		};

	setters["iTriggerMode"] = [this](const Any& value) {
		m_VoltageInfo.iTriggerMode = value.any_cast<int>();
		};

	setters["bTriggerVerify"] = [this](const Any& value) {
		m_VoltageInfo.bTriggerVerify = value.any_cast<bool>();
		};

	setters["iReadCI"] = [this](const Any& value) {
		m_VoltageInfo.iReadCI = value.any_cast<int>();
		};

	/*setters["iForceUnit"] = [this](const Any& value) {
		m_VoltageInfo.iForceUnit = value.any_cast<int>();
		};*/

	setters["bOpenDI"] = [this](const Any& value) {
		m_VoltageInfo.bOpenDI = value.any_cast<bool>();
		};

	setters["input"] = [this](const Any& value) {
		m_VoltageInfo.input = value.any_cast<DataInput>();
		};

	setters["output"] = [this](const Any& value) {
		m_VoltageInfo.output = value.any_cast<DataOutput>();
		};

	// Getters
	/*getters["iModel"] = [this]() -> Any {
		return m_VoltageInfo.iModel;
		};*/

	getters["iADInputMode"] = [this]() -> Any {
		return m_VoltageInfo.iADInputMode;
		};

	getters["bADOpen"] = [this]() -> Any {
		return m_VoltageInfo.bADOpen;
		};

	getters["iADChannel"] = [this]() -> Any {
		return m_VoltageInfo.iADChannel;
		};

	getters["sADName"] = [this]() -> Any {
		std::array<QString, 16> arr;
		std::copy(std::begin(m_VoltageInfo.sADName), std::end(m_VoltageInfo.sADName), arr.begin());
		return arr;
		};

	getters["iADDataType"] = [this]() -> Any {
		std::array<int, 16> arr;
		std::copy(std::begin(m_VoltageInfo.iADDataType), std::end(m_VoltageInfo.iADDataType), arr.begin());
		return arr;
		};

	getters["iADUnit"] = [this]() -> Any {
		std::array<int, 16> arr;
		std::copy(std::begin(m_VoltageInfo.iADUnit), std::end(m_VoltageInfo.iADUnit), arr.begin());
		return arr;
		};

	getters["bADExe"] = [this]() -> Any {
		std::array<bool, 16> arr;
		std::copy(std::begin(m_VoltageInfo.bADExe), std::end(m_VoltageInfo.bADExe), arr.begin());
		return arr;
		};

	getters["bAutoADExe"] = [this]() -> Any {
		std::array<bool, 16> arr;
		std::copy(std::begin(m_VoltageInfo.bAutoADExe), std::end(m_VoltageInfo.bAutoADExe), arr.begin());
		return arr;
		};

	getters["dADRange"] = [this]() -> Any {
		std::array<std::array<double, 2>, 16> arr;
		for (int i = 0; i < 16; ++i) {
			arr[i][0] = m_VoltageInfo.dADRange[i][0];
			arr[i][1] = m_VoltageInfo.dADRange[i][1];
		}
		return arr;
		};

	getters["dCompensation"] = [this]() -> Any {
		std::array<double, 16> arr;
		std::copy(std::begin(m_VoltageInfo.dCompensation), std::end(m_VoltageInfo.dCompensation), arr.begin());
		return arr;
		};

	getters["dVADCaliArray"] = [this]() -> Any {
		std::array<std::array<std::array<double, 2>, 10>, 16> arr;
		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 10; ++j) {
				arr[i][j][0] = m_VoltageInfo.dVADCaliArray[i][j][0];
				arr[i][j][1] = m_VoltageInfo.dVADCaliArray[i][j][1];
			}
		}
		return arr;
		};

	getters["iThresholdType"] = [this]() -> Any {
		return m_VoltageInfo.iThresholdType;
		};

	getters["dAbnormalThreshold"] = [this]() -> Any {
		std::array<double, 2> arr;
		arr[0] = m_VoltageInfo.dAbnormalThreshold[0];
		arr[1] = m_VoltageInfo.dAbnormalThreshold[1];
		return arr;
		};

	getters["dValueK"] = [this]() -> Any {
		std::array<double, 16> arr;
		std::copy(std::begin(m_VoltageInfo.dValueK), std::end(m_VoltageInfo.dValueK), arr.begin());
		return arr;
		};

	getters["dValueB"] = [this]() -> Any {
		std::array<double, 16> arr;
		std::copy(std::begin(m_VoltageInfo.dValueB), std::end(m_VoltageInfo.dValueB), arr.begin());
		return arr;
		};

	getters["bDAOpen"] = [this]() -> Any {
		return m_VoltageInfo.bDAOpen;
		};

	getters["iDAChannel"] = [this]() -> Any {
		return m_VoltageInfo.iDAChannel;
		};

	getters["bDAExe"] = [this]() -> Any {
		std::array<bool, 2> arr;
		std::copy(std::begin(m_VoltageInfo.bDAExe), std::end(m_VoltageInfo.bDAExe), arr.begin());
		return arr;
		};

	getters["bAutoDAExe"] = [this]() -> Any {
		std::array<bool, 2> arr;
		std::copy(std::begin(m_VoltageInfo.bAutoDAExe), std::end(m_VoltageInfo.bAutoDAExe), arr.begin());
		return arr;
		};

	getters["dDAVoltRange"] = [this]() -> Any {
		std::array<std::array<double, 2>, 2> arr;
		for (int i = 0; i < 2; ++i) {
			arr[i][0] = m_VoltageInfo.dDAVoltRange[i][0];
			arr[i][1] = m_VoltageInfo.dDAVoltRange[i][1];
		}
		return arr;
		};

	/*getters["strResult"] = [this]() -> Any {
		std::array<std::array<QString, 4>, 2> arr{};
		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 4; ++j) {
				arr[i][j] = m_VoltageInfo.strResult[i][j];
			}
		}
		return arr;
		};*/

	getters["vScaleFactorArray"] = [this]() -> Any {
		std::array<double, 2> arr;
		std::copy(std::begin(m_VoltageInfo.vScaleFactorArray), std::end(m_VoltageInfo.vScaleFactorArray), arr.begin());
		return arr;
		};

	getters["vOutputMinVoltArray"] = [this]() -> Any {
		std::array<double, 2> arr;
		std::copy(std::begin(m_VoltageInfo.vOutputMinVoltArray), std::end(m_VoltageInfo.vOutputMinVoltArray), arr.begin());
		return arr;
		};

	getters["vOutputMaxVoltArray"] = [this]() -> Any {
		std::array<double, 2> arr;
		std::copy(std::begin(m_VoltageInfo.vOutputMaxVoltArray), std::end(m_VoltageInfo.vOutputMaxVoltArray), arr.begin());
		return arr;
		};

	getters["vMinValueArray"] = [this]() -> Any {
		std::array<double, 2> arr;
		std::copy(std::begin(m_VoltageInfo.vMinValueArray), std::end(m_VoltageInfo.vMinValueArray), arr.begin());
		return arr;
		};

	getters["vMaxValueArray"] = [this]() -> Any {
		std::array<double, 2> arr;
		std::copy(std::begin(m_VoltageInfo.vMaxValueArray), std::end(m_VoltageInfo.vMaxValueArray), arr.begin());
		return arr;
		};

	getters["iControllerType"] = [this]() -> Any {
		return m_VoltageInfo.iControllerType;
		};

	getters["iExternalTrigger"] = [this]() -> Any {
		return m_VoltageInfo.iExternalTrigger;
		};

	getters["iTriggerMode"] = [this]() -> Any {
		return m_VoltageInfo.iTriggerMode;
		};

	getters["bTriggerVerify"] = [this]() -> Any {
		return m_VoltageInfo.bTriggerVerify;
		};

	getters["iReadCI"] = [this]() -> Any {
		return m_VoltageInfo.iReadCI;
		};

	/*getters["iForceUnit"] = [this]() -> Any {
		return m_VoltageInfo.iForceUnit;
		};*/

	getters["bOpenDI"] = [this]() -> Any {
		return m_VoltageInfo.bOpenDI;
		};

	getters["input"] = [this]() -> Any {
		return m_VoltageInfo.input;
		};

	getters["output"] = [this]() -> Any {
		return m_VoltageInfo.output;
		};
}

void VoltageComm::initInfo()
{
	// 初始化 iModel
	//m_VoltageInfo.iModel = 0;

	// 初始化 AD 部分
	m_VoltageInfo.iADInputMode = 0;
	m_VoltageInfo.bADOpen = false;
	m_VoltageInfo.iADChannel = 0;
	m_VoltageInfo.iADMode = 0;
	std::fill(std::begin(m_VoltageInfo.sADName), std::end(m_VoltageInfo.sADName), QString(""));
	m_VoltageInfo.sADName[0] = QString::fromLocal8Bit("力值");
	m_VoltageInfo.sADName[1] = QString::fromLocal8Bit("位移");
	m_VoltageInfo.sADName[2] = QString::fromLocal8Bit("速度");
	m_VoltageInfo.sADName[3] = QString::fromLocal8Bit("温度");
	m_VoltageInfo.sADName[4] = QString::fromLocal8Bit("激励数据");
	m_VoltageInfo.sADName[5] = QString::fromLocal8Bit("加速度");
	m_VoltageInfo.sADName[6] = QString::fromLocal8Bit("角速度");
	std::fill(std::begin(m_VoltageInfo.iADDataType), std::end(m_VoltageInfo.iADDataType), 0);
	std::fill(std::begin(m_VoltageInfo.iADUnit), std::end(m_VoltageInfo.iADUnit), 0);
	std::fill(std::begin(m_VoltageInfo.bADExe), std::end(m_VoltageInfo.bADExe), false);
	std::fill(std::begin(m_VoltageInfo.bAutoADExe), std::end(m_VoltageInfo.bAutoADExe), false);
	for (auto& range : m_VoltageInfo.dADRange) {
		range[0] = -10.0;
		range[1] = 10.0;
	}
	std::fill(std::begin(m_VoltageInfo.dCompensation), std::end(m_VoltageInfo.dCompensation), 0.0);
	for (auto& cali : m_VoltageInfo.dVADCaliArray) {
		for (auto& point : cali) {
			point[0] = 0.0;
			point[1] = 0.0;
		}
		cali[0][0] = 1.0;
		cali[0][1] = 1.0;
	}
	m_VoltageInfo.iThresholdType = 0;
	m_VoltageInfo.dAbnormalThreshold[0] = 0.1;
	m_VoltageInfo.dAbnormalThreshold[1] = 200;
	std::fill(std::begin(m_VoltageInfo.dValueK), std::end(m_VoltageInfo.dValueK), 1.0);
	std::fill(std::begin(m_VoltageInfo.dValueB), std::end(m_VoltageInfo.dValueB), 0.0);

	// 初始化 DA 部分
	m_VoltageInfo.bDAOpen = false;
	m_VoltageInfo.iDAChannel = 0;
	std::fill(std::begin(m_VoltageInfo.bDAExe), std::end(m_VoltageInfo.bDAExe), false);
	std::fill(std::begin(m_VoltageInfo.bAutoDAExe), std::end(m_VoltageInfo.bAutoDAExe), false);
	for (auto& range : m_VoltageInfo.dDAVoltRange) {
		range[0] = -10.0;
		range[1] = 10.0;
	}

	for (auto& str : m_VoltageInfo.strResult) {
		str[0] = "\0";
		str[1] = "\0";
		str[2] = "\0";
		str[3] = "\0";
	}

	std::fill(std::begin(m_VoltageInfo.vScaleFactorArray), std::end(m_VoltageInfo.vScaleFactorArray), 1.0);
	std::fill(std::begin(m_VoltageInfo.vOutputMinVoltArray), std::end(m_VoltageInfo.vOutputMinVoltArray), 0.0);
	std::fill(std::begin(m_VoltageInfo.vOutputMaxVoltArray), std::end(m_VoltageInfo.vOutputMaxVoltArray), 10.0);
	std::fill(std::begin(m_VoltageInfo.vMinValueArray), std::end(m_VoltageInfo.vMinValueArray), 0.0);
	std::fill(std::begin(m_VoltageInfo.vMaxValueArray), std::end(m_VoltageInfo.vMaxValueArray), 10.0);

	m_VoltageInfo.input.iChannelState = 0;
	m_VoltageInfo.input.iChannelSize = 1;
	m_VoltageInfo.input.iTriggerCtrl = 0;
	m_VoltageInfo.input.iTransferType = 0;
	for (int i = 0; i < 4; i++)
	{
		m_VoltageInfo.input.iDataType[i] = 0;
		m_VoltageInfo.input.iUnit[i] = 0;
	}

	m_VoltageInfo.output.iChannelState = 0;
	m_VoltageInfo.output.iChannelSize = 1;
	m_VoltageInfo.output.iTriggerCtrl = 0;
	m_VoltageInfo.output.iTransferType = 0;
	for (int i = 0; i < 8; i++)
	{
		m_VoltageInfo.output.iDataModel[i] = 0;
		m_VoltageInfo.output.iDataType[i] = 0;
		m_VoltageInfo.output.iData[i] = 0;
		m_VoltageInfo.output.bReverse[i] = 0;
		//m_VoltageInfo.output.iUnit[i] = 0;
	}

	// 初始化其他参数
	m_VoltageInfo.iControllerType = 0;
	m_VoltageInfo.iExternalTrigger = 0;
	m_VoltageInfo.iTriggerMode = 0;
	m_VoltageInfo.bTriggerVerify = false;
	m_VoltageInfo.iReadCI = 0;
	//m_VoltageInfo.iForceUnit = 0;
	m_VoltageInfo.bOpenDI = false;
}
