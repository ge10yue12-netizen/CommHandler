#include "SerialPortComm.h"
#include <string>
#include <QVariantMap>

SerialPortComm::SerialPortComm()
{
	initParameters();
	initInfo();

	ConnectSlots();
}

SerialPortComm::~SerialPortComm()
{}

void SerialPortComm::SetToDefault()
{
	initInfo();
}

void SerialPortComm::ConnectSlots()
{
	// IN
	QObject::connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialPortComm::onRecvDataSlots);
}

bool SerialPortComm::SetSysParInfo(void* ptr)
{
	if (ptr)
	{
		m_SerialPortInfo = *(static_cast<SerialPortInfo*>(ptr));
		return true;
	}
	return false;
}

bool SerialPortComm::GetSysParInfo(void* ptr) const
{
	if (ptr)
	{
		// 将 void* 转换为 SocketInfo* 并赋值
		SerialPortInfo* InfoPtr = static_cast<SerialPortInfo*>(ptr);
		*InfoPtr = m_SerialPortInfo;
		return true;
	}
	return false;
}

void SerialPortComm::setParameter(const QString& key, const Any& value)
{
	auto it = setters.find(key);
	if (it != setters.end()) {
		it->second(value);
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

Any SerialPortComm::getParameter(const QString& key) const
{
	auto it = getters.find(key);
	if (it != getters.end()) {
		return it->second();
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

bool SerialPortComm::setExternalFreq(const int& freq)
{
	return false;
}

bool SerialPortComm::Connect(int type)
{
	Q_UNUSED(type);
	if (m_SerialPortInfo.bComOpened || m_serialPort.isOpen())
	{
		return false;
	}

	QString strPortName = QString("COM%1").arg(QString::number(m_SerialPortInfo.iComPort + 1));
	qint32 qComBaud = QSerialPort::Baud9600;
	switch (m_SerialPortInfo.iComBaud)
	{
	case 0: qComBaud = QSerialPort::Baud4800; break;
	case 1: qComBaud = QSerialPort::Baud9600; break;
	case 2: qComBaud = QSerialPort::Baud19200; break;
	case 3: qComBaud = QSerialPort::Baud38400; break;
	case 4: qComBaud = QSerialPort::Baud57600; break;
	case 5: qComBaud = QSerialPort::Baud115200; break;
	}
	QSerialPort::DataBits databits = QSerialPort::Data8;
	switch (m_SerialPortInfo.iDataBits)
	{
	case 0:	databits = QSerialPort::Data5; break;
	case 1:	databits = QSerialPort::Data6; break;
	case 3:	databits = QSerialPort::Data8; break;
	default:
		break;
	}
	QSerialPort::Parity parity = QSerialPort::NoParity;
	switch (m_SerialPortInfo.iParity)
	{
	case 0:	parity = QSerialPort::NoParity; break;
	case 1:	parity = QSerialPort::EvenParity; break;
	case 2:	parity = QSerialPort::OddParity; break;
	case 3:	parity = QSerialPort::SpaceParity; break;
	case 4:	parity = QSerialPort::MarkParity; break;
	default:
		break;
	}
	QSerialPort::StopBits stopbit = QSerialPort::OneStop;
	switch (m_SerialPortInfo.iStopBits)
	{
	case 0:	stopbit = QSerialPort::OneStop; break;
	case 1:	stopbit = QSerialPort::OneAndHalfStop; break;
	case 2:	stopbit = QSerialPort::TwoStop; break;
	default:
		break;
	}
	m_iDataBits = databits;
	m_serialPort.setPortName(strPortName);
	m_serialPort.setBaudRate(qComBaud);
	m_serialPort.setDataBits(databits);
	m_serialPort.setParity(parity);
	m_serialPort.setStopBits(stopbit);
	m_serialPort.setFlowControl(QSerialPort::NoFlowControl);
	if (!m_serialPort.open(QIODevice::OpenModeFlag::ReadWrite) || !m_serialPort.isOpen())
	{
		m_SerialPortInfo.bComOpened = false;
		return false;
	}
	m_serialPort.setRequestToSend(false);
	m_serialPort.setDataTerminalReady(false);
	m_SerialPortInfo.bComOpened = true;
	return true;
}

bool SerialPortComm::Disconnect(int type)
{
	Q_UNUSED(type);
	if (m_serialPort.isOpen())
		m_serialPort.close();
	m_SerialPortInfo.bComOpened = false;
	return true;
}

//void SerialPortComm::SendData(void* data, size_t size)
//{
//	// 创建 QByteArray
//	QByteArray byteArray(reinterpret_cast<const char*>(data), (int)size);
//	m_serialPort.write(byteArray);
//
//	//int estimatedTime = (size * 10 * 1000) / m_SerialPortInfo.iTsComBaud; // 计算毫秒级发送时间
//	//int timeout = qMax(estimatedTime * 2, 100); // 设定至少100ms，避免过短
//	m_serialPort.waitForBytesWritten(100);
//}

void SerialPortComm::SendData(const std::vector<double>& vData)
{
	if (!m_SerialPortInfo.bInquireSendFlag)
	{
		return;
	}
	QByteArray byteArray{};
	switch (m_SerialPortInfo.iProtocolType)
	{
		//case 0:
		//{
		//	// 联恒
		//	std::vector<quint8> moduleId, dataTypeId;
		//	auto data = m_SerLhgkProtocol.buildModuleDataFrame(moduleId, dataTypeId, vData);
		//	byteArray = QByteArray(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));;
		//	break;
		//}
	case 0:
	{
		int size = m_iDataBits * vData.size() + 1;
		unsigned char* pDataPackage = new unsigned char[size];
		for (int i = 0; i < vData.size(); i++)
		{
			FloatToHex(pDataPackage, i, vData.at(i), m_iDataBits);
		}
		pDataPackage[m_iDataBits * (int)vData.size()] = 0x0D;
		byteArray = QByteArray((const char*)pDataPackage, size);
		delete[]pDataPackage;
		pDataPackage = nullptr;
		break;
	}
	case 1:
	{
		unsigned char* pDataPackage = new unsigned char[14];
		FixedDoubleToHex(pDataPackage, 0, vData.at(0));
		byteArray = QByteArray((const char*)pDataPackage, 14);
		delete[]pDataPackage;
		pDataPackage = nullptr;
		break;
	}
	case 2:
	{
		break;
	}
	case 3:
	{
		if (vData.size() < 5)
		{
			return;
		}

		double* dData = new double[4];
		for (int i = 0; i < 4; i++)
		{
			dData[i] = vData.at(i);
		}

		SendData2ComPort(dData[0], dData[1], dData[2], dData[3], 0, vData[4]);

		delete[]dData;
		dData = nullptr;

		m_serialPort.write((const char*)DataPack(), 56);
		//m_serialPort.waitForBytesWritten(50);
		break;
	}
	case 4:
	{
		QString frame = QStringLiteral("R%1#N%2#")
			.arg(formatValue(vData.at(0)))
			.arg(formatValue(vData.at(1)));

		// 转成 ASCII（Latin1 是 ASCII 的超集）
		byteArray = frame.toLatin1();
		break;
	}
	case 5:
	{
		// 联恒光科串口（BASELINE V1.2.5 / iProtocolType=5）：至少力、温两路
		if (vData.size() < 2)
		{
			return;
		}
		const auto frame = m_SerLhgkProtocol.buildExternalDataFrame(vData.at(0), vData.at(1));
		byteArray = QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<int>(frame.size()));
		break;
	}

	default:
		break;
	}

	if (3 != m_SerialPortInfo.iProtocolType)
	{
		m_serialPort.write(byteArray);
	}

	if (0 == m_SerialPortInfo.output.iTransferType)
	{
		m_serialPort.waitForBytesWritten(50);
	}
	else if (1 == m_SerialPortInfo.output.iTransferType)
	{
		m_serialPort.waitForBytesWritten(-1);
		m_SerialPortInfo.bInquireSendFlag = false;
	}

}

bool SerialPortComm::SaveParInfo(const QString& filename)
{
	std::ofstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) {
		//throw std::runtime_error("Failed to open file for writing");
		return false;
	}

	// 写入结构体数据
	file.write(reinterpret_cast<const char*>(&m_SerialPortInfo), sizeof(m_SerialPortInfo));

	file.close();
	return true;
}

bool SerialPortComm::LoadParInfo(const QString& filename)
{
	std::string file_ = filename.toLocal8Bit().constData();
	std::ifstream file(file_, std::ios::binary);
	if (!file.is_open()) {
		//throw std::runtime_error("Failed to open file for reading");
		return false;
	}

	// 读取结构体数据
	file.read(reinterpret_cast<char*>(&m_SerialPortInfo), sizeof(m_SerialPortInfo));

	m_SerialPortInfo.bComOpened = false;
	m_SerialPortInfo.bInDataCombinating = false;
	m_SerialPortInfo.bInquireSendFlag = false;
	m_SerialPortInfo.isCollecting = false;

	//m_SerialPortInfo.input.iChannelState = 0;
	//m_SerialPortInfo.input.iChannelSize = 1;
	//m_SerialPortInfo.input.iTriggerCtrl = 0;
	//m_SerialPortInfo.input.iTransferType = 0;
	//for (int i = 0; i < 4; i++)
	//{
	//	m_SerialPortInfo.input.iDataType[i] = 0;
	//	m_SerialPortInfo.input.iUnit[i] = 0;
	//}

	//m_SerialPortInfo.output.iChannelState = 0;
	//m_SerialPortInfo.output.iChannelSize = 1;
	//m_SerialPortInfo.output.iTriggerCtrl = 0;
	//m_SerialPortInfo.output.iTransferType = 0;
	//for (int i = 0; i < 8; i++)
	//{
	//	m_SerialPortInfo.output.iDataModel[i] = 0;
	//	m_SerialPortInfo.output.iDataType[i] = 0;
	//	m_SerialPortInfo.output.iData[i] = 0;
	//	m_SerialPortInfo.output.bReverse[i] = 0;
	//	//m_SerialPortInfo.output.iUnit[i] = 0;
	//}

	file.close();
	return true;
}

bool SerialPortComm::saveCmdToFile(const std::string& filename)
{
	//return m_commConfig->saveToFile(filename); // 加载配置文件
	return true;
}

bool SerialPortComm::loadCmdFromFile(const std::string& filename)
{
	//return m_commConfig->loadFromFile(filename); // 加载配置文件
	return true;
}

void SerialPortComm::initParameters()
{
	// 串口参数
	//setters["iEncryptionMode"] = [this](const Any& value) { m_SerialPortInfo.iEncryptionMode = value.any_cast<int>(); };
	setters["iComPort"] = [this](const Any& value) { m_SerialPortInfo.iComPort = value.any_cast<int>(); };
	setters["iComBaud"] = [this](const Any& value) { m_SerialPortInfo.iComBaud = value.any_cast<int>(); };
	setters["iParity"] = [this](const Any& value) { m_SerialPortInfo.iParity = value.any_cast<int>(); };
	setters["iDataBits"] = [this](const Any& value) { m_SerialPortInfo.iDataBits = value.any_cast<int>(); };
	setters["iStopBits"] = [this](const Any& value) { m_SerialPortInfo.iStopBits = value.any_cast<int>(); };
	setters["iProtocolType"] = [this](const Any& value) {
		m_SerialPortInfo.iProtocolType = value.any_cast<int>();
		// 协议切换清空拼帧缓冲，避免旧半包污染
		m_lhgkRxBuffer.clear();
	};

	setters["dInForceRange"] = [this](const Any& value) { m_SerialPortInfo.dInForceRange = value.any_cast<double>(); };
	setters["bHexSetZero"] = [this](const Any& value) { m_SerialPortInfo.bHexSetZero = value.any_cast<bool>(); };
	setters["m_bInDataCombinating"] = [this](const Any& value) { m_SerialPortInfo.bInDataCombinating = value.any_cast<bool>(); };
	setters["bInquireSendFlag"] = [this](const Any& value) { m_SerialPortInfo.bInquireSendFlag = value.any_cast<bool>(); };
	setters["isCollecting"] = [this](const Any& value) { m_SerialPortInfo.isCollecting = value.any_cast<bool>(); };
	setters["input"] = [this](const Any& value) { m_SerialPortInfo.input = value.any_cast<DataInput>(); };
	setters["output"] = [this](const Any& value) { m_SerialPortInfo.output = value.any_cast<DataOutput>(); };



	// 获取值的对应函数
	//getters["iEncryptionMode"] = [this]() -> Any { return m_SerialPortInfo.iEncryptionMode; };
	getters["iComPort"] = [this]() -> Any { return m_SerialPortInfo.iComPort; };
	getters["iComBaud"] = [this]() -> Any { return m_SerialPortInfo.iComBaud; };
	getters["iParity"] = [this]() -> Any { return m_SerialPortInfo.iParity; };
	getters["iDataBits"] = [this]() -> Any { return m_SerialPortInfo.iDataBits; };
	getters["iStopBits"] = [this]() -> Any { return m_SerialPortInfo.iStopBits; };
	getters["iProtocolType"] = [this]() -> Any { return m_SerialPortInfo.iProtocolType; };
	getters["dInForceRange"] = [this]() -> Any { return m_SerialPortInfo.dInForceRange; };
	getters["m_bInDataCombinating"] = [this]() -> Any { return m_SerialPortInfo.bHexSetZero; };
	getters["m_bInDataCombinating"] = [this]() -> Any { return m_SerialPortInfo.bInDataCombinating; };
	getters["bInquireSendFlag"] = [this]() -> Any { return m_SerialPortInfo.bInquireSendFlag; };
	getters["isCollecting"] = [this]() -> Any { return m_SerialPortInfo.isCollecting; };
	getters["input"] = [this]() -> Any { return m_SerialPortInfo.input; };
	getters["output"] = [this]() -> Any { return m_SerialPortInfo.output; };
}


void SerialPortComm::initInfo()
{
	// 
	m_SerialPortInfo.bComOpened = false;
	//m_SerialPortInfo.iEncryptionMode = 1;
	m_SerialPortInfo.iComPort = 0;
	m_SerialPortInfo.iComBaud = 1;
	m_SerialPortInfo.iParity = 0;
	m_SerialPortInfo.iDataBits = 3;
	m_SerialPortInfo.iStopBits = 0;
	m_SerialPortInfo.iProtocolType = 0;
	m_SerialPortInfo.dInForceRange = 100;
	m_SerialPortInfo.bHexSetZero = false;
	m_SerialPortInfo.bInDataCombinating = false;
	m_SerialPortInfo.bInquireSendFlag = false;
	m_SerialPortInfo.isCollecting = false;

	m_SerialPortInfo.input.iChannelState = 0;
	m_SerialPortInfo.input.iChannelSize = 1;
	m_SerialPortInfo.input.iTriggerCtrl = 0;
	m_SerialPortInfo.input.iTransferType = 0;
	for (int i = 0; i < 4; i++)
	{
		m_SerialPortInfo.input.iDataType[i] = 0;
		m_SerialPortInfo.input.iUnit[i] = 0;
	}

	m_SerialPortInfo.output.iChannelState = 0;
	m_SerialPortInfo.output.iChannelSize = 1;
	m_SerialPortInfo.output.iTriggerCtrl = 0;
	m_SerialPortInfo.output.iTransferType = 0;
	for (int i = 0; i < 8; i++)
	{
		m_SerialPortInfo.output.iDataModel[i] = 0;
		m_SerialPortInfo.output.iDataType[i] = 0;
		m_SerialPortInfo.output.iData[i] = 1;
		m_SerialPortInfo.output.bReverse[i] = 0;
		//m_SerialPortInfo.output.iUnit[i] = 0;
	}

	m_iDataBits = 0;
}

void SerialPortComm::MsgDataInput(QString sData, double* dData, int& size)
{
	switch (m_SerialPortInfo.iProtocolType)
	{
	case 0:
	{
		QByteArray ba = sData.toLatin1();
		bool ok = false;
		int iForce = ba.toInt(&ok, 16);
		double dForce = iForce / 1000000.0 / 100.0 * m_SerialPortInfo.dInForceRange;
		if (ok)
		{
			dData[0] = dForce;
			dData[1] = DeviceDataStruct::COMDATA_STATUS_ENABLE;
			size = 2;
		}
		break;
	}
	case 1:
	{
		QByteArray ba = sData.toLatin1();
		{
			dData[0] = HexToFloat(ba);
			dData[1] = DeviceDataStruct::COMDATA_STATUS_ENABLE;
			size = 2;
		}
		break;
	}
	case 3:
	{
		// 获得力信号
		QString strValue = sData.mid(0, 16);
		QString strTemp = ASCDecode(strValue);
		dData[0] = GetData(strTemp);

		// 位移
		strValue = sData.mid(16, 16);
		strTemp = ASCDecode(strValue);
		dData[1] = GetData(strTemp);

		// 小变形
		strValue = sData.mid(32, 16);
		strTemp = ASCDecode(strValue);
		dData[2] = GetData(strTemp);

		// 大变形
		strValue = sData.mid(48, 16);
		strTemp = ASCDecode(strValue);
		dData[3] = GetData(strTemp);

		// 时间
		strValue = sData.mid(80, 16);
		strTemp = ASCDecode(strValue);
		dData[4] = GetData(strTemp);

		dData[5] = DeviceDataStruct::COMDATA_STATUS_ENABLE;
		size = 6;
		break;
	}
	default:
		break;
	}
}

bool SerialPortComm::MatchCtrlData(QString sData)
{
	int iLength = sData.size();
	for (int i = 0; i < iLength; i++)
	{
		switch (i)
		{
		case 0:
		{
			if (sData.at(i) != '7')
				return false;
		}
		break;
		case 1:
		{
			if (sData.at(i) != 'B')
				return false;
		}
		break;
		case 2:
		{
			if (sData.at(i) != '5')
				return false;
		}
		break;
		case 3:
		{
			if (sData.at(i) != '1')
				return false;
		}
		break;
		case 4:
		{
			if (sData.at(i) != '4')
				return false;
		}
		break;
		case 5:
		{
			if (sData.at(i) != 'C')
				return false;
		}
		break;
		case 6:
		{
			if (sData.at(i) != '4')
				return false;
		}
		break;
		case 7:
		{
			if (sData.at(i) != '9')
				return false;
		}
		break;
		case 8:
		{
			if (sData.at(i) != '5')
				return false;
		}
		break;
		case 9:
		{
			if (sData.at(i) != 'B')
				return false;
		}
		break;
		case 10:
		{
			if (sData.at(i) != '3')
				return false;
		}
		break;
		case 11:
		{
			if (sData.at(i) != '1' && sData.at(i) != '2')
				return false;
		}
		break;
		case 12:
		{
			if (sData.at(i) != '5')
				return false;
		}
		break;
		case 13:
		{
			if (sData.at(i) != 'D')
				return false;
		}
		break;
		case 14:
		{
			if (sData.at(i) != '7')
				return false;
		}
		break;
		case 15:
		{
			if (sData.at(i) != 'D')
				return false;
		}
		break;
		default:
			break;
		}
	}
	return true;
}

int SerialPortComm::Str2Hex(const char* str, int nLen, char* data)
{
	int t, t1;
	int ilen = 0;
	for (int i = 0; i < nLen;)
	{
		char l, h = str[i];
		if (h == ' ')
		{
			i++;
			continue;
		}
		i++;
		if (i >= nLen)
			break;
		l = str[i];
		t = _HexChar(h);
		t1 = _HexChar(l);
		if ((t == 16) || (t1 == 16))
			break;
		else
			t = t * 16 + t1;
		i++;
		data[ilen] = (char)t;
		ilen++;
	}
	return ilen;
}

QString SerialPortComm::ASCDecode(QString& strData)
{
	QString strtmp, strd;
	strd = "";
	TCHAR data[2];
	int nLength = strData.length() / 2;
	for (int i = 0; i < nLength; i++)
	{
		strtmp = strData.mid(2 * i, 2);
		if (strtmp[0] != "3")
		{
			Str2Hex_t(strtmp, 2, data);
			strd += data[0];
		}
		else
		{
			strd += strtmp[1];
		}
	}
	return strd;
}

double SerialPortComm::GetData(QString& strHex)
{
	QString strBin;
	strBin = HexToBin(strHex);
	QString strSign, strIndex, strData;
	strSign = strBin.left(1);
	strIndex = strBin.mid(1, 8);
	int nIndex = int(BinToDec(strIndex, strIndex.length() - 1) - 127);
	strData = "1" + strBin.right(strBin.length() - 9);
	double dRe = BinToDec(strData, nIndex);
	if (strSign == "1")
	{
		dRe = -1 * dRe;
	}
	return dRe;
}

int SerialPortComm::Str2Hex_t(const QString str, int nLen, TCHAR* data)
{
	int t, t1;
	int ilen = 0;
	for (int i = 0; i < nLen;)
	{
		QString l, h = str[i];
		if (h == " ")
		{
			i++;
			continue;
		}
		i++;
		if (i >= nLen)
			break;
		l = str[i];
		t = _HexFromQString(h);
		t1 = _HexFromQString(l);
		if ((t == 16) || (t1 == 16))
			break;
		else
			t = t * 16 + t1;
		i++;
		data[ilen] = (TCHAR)t;
		ilen++;
	}
	return ilen;
}

QString SerialPortComm::HexToBin(QString& strHex)
{
	int iLen = strHex.length();
	QString strResult = "";
	for (int i = 0; i < iLen; i++)
	{
		int hex = strHex.mid(i, 1).toInt(0, 16);
		switch (hex)
		{
		case 0:
			strResult += "0000";
			break;
		case 1:
			strResult += "0001";
			break;
		case 2:
			strResult += "0010";
			break;
		case 3:
			strResult += "0011";
			break;
		case 4:
			strResult += "0100";
			break;
		case 5:
			strResult += "0101";
			break;
		case 6:
			strResult += "0110";
			break;
		case 7:
			strResult += "0111";
			break;
		case 8:
			strResult += "1000";
			break;
		case 9:
			strResult += "1001";
			break;
		case 10:
			strResult += "1010";
			break;
		case 11:
			strResult += "1011";
			break;
		case 12:
			strResult += "1100";
			break;
		case 13:
			strResult += "1101";
			break;
		case 14:
			strResult += "1110";
			break;
		case 15:
			strResult += "1111";
			break;
		default:
			break;
		}
	}
	return strResult;
}

double SerialPortComm::BinToDec(QString& strBin, long nIndex)
{
	double dRe = 0;
	int iLen = strBin.length();
	for (int i = 0; i < iLen; i++)
	{
		if (strBin[i] == "0")
			continue;
		else
		{
			double dk = 1;
			dk = dk * pow(2.0, (int)(nIndex - i));
			dRe += dk;
		}
	}
	return dRe;
}

int SerialPortComm::_HexChar(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - 0x30;
	else if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	else if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	else
		return 0x10;
}

int SerialPortComm::_HexFromQString(QString str)
{
	if ((str >= 48) && (str <= 57))
		return str.toInt();
	else if ((str >= 65) && (str <= 70))
		return str.toInt() - 55;
	else if ((str >= 97) && (str <= 102))
		return str.toInt() - 87;
	else
		return 0x10;
}

void SerialPortComm::SendData2ComPort(float fStrain1, float fStrain2, float fMovement1, float fMovement2, float fReserve, float fTime)
{
	unsigned char* pdata1 = (unsigned char*)&fStrain1;//把float类型的指针强制转换为unsigned char型
	for (int i = 0; i < 4; i++)
	{
		m_tmpS1[i] = *pdata1++;//把相应地址中的数据保存到unsigned char数组中
	}

	unsigned char* pdata2 = (unsigned char*)&fStrain2;
	for (int i = 0; i < 4; i++)
	{
		m_tmpS2[i] = *pdata2++;
	}

	unsigned char* pdata3 = (unsigned char*)&fMovement1;
	for (int i = 0; i < 4; i++)
	{
		m_tmpS3[i] = *pdata3++;
	}

	unsigned char* pdata4 = (unsigned char*)&fMovement2;
	for (int i = 0; i < 4; i++)
	{
		m_tmpS4[i] = *pdata4++;
	}

	unsigned char* pdata5 = (unsigned char*)&fReserve;
	for (int i = 0; i < 4; i++)
	{
		m_tmpS5[i] = *pdata5++;
	}

	unsigned char* pdata6 = (unsigned char*)&fTime;
	for (int i = 0; i < 4; i++)
	{
		m_tmpS6[i] = *pdata6++;
	}

	float tmpG = (float)((int)m_tmpS1[0] + (int)m_tmpS1[1] + (int)m_tmpS1[2] + (int)m_tmpS1[3]
		+ (int)m_tmpS2[0] + (int)m_tmpS2[1] + (int)m_tmpS2[2] + (int)m_tmpS2[3]
		+ (int)m_tmpS3[0] + (int)m_tmpS3[1] + (int)m_tmpS3[2] + (int)m_tmpS3[3]
		+ (int)m_tmpS4[0] + (int)m_tmpS4[1] + (int)m_tmpS4[2] + (int)m_tmpS4[3]
		+ (int)m_tmpS5[0] + (int)m_tmpS5[1] + (int)m_tmpS5[2] + (int)m_tmpS5[3]
		+ (int)m_tmpS6[0] + (int)m_tmpS6[1] + (int)m_tmpS6[2] + (int)m_tmpS6[3]);

	unsigned char* pdata7 = (unsigned char*)&tmpG;
	for (int i = 0; i < 2; i++)
	{
		m_tmpS7[i] = *pdata7++;
	}
}

double SerialPortComm::OutputVoltage(int iType, double dVol)
{
	switch (iType)
	{
	case 0:
		if (dVol <= -5)
		{
			dVol = -5;
		}
		if (dVol >= 5)
		{
			dVol = 5;
		}
		break;
	case 1:
		if (dVol <= -10)
		{
			dVol = -10;
		}
		if (dVol >= 10)
		{
			dVol = 10;
		}
		break;
	case 2:
		if (dVol <= 0)
		{
			dVol = 0;
		}
		if (dVol >= 5)
		{
			dVol = 5;
		}
		break;
	default:
		break;
	}
	return dVol;
}

unsigned char* SerialPortComm::DataPack()
{
	//应变1，应变2，位移1，位移2，扩展，时间
	////第一行：应变1
	HandleArray(m_tmpS1, 0);

	////第二行：应变2
	HandleArray(m_tmpS2, 8);

	////第三行：位移1
	HandleArray(m_tmpS3, 16);

	////第四行：位移2
	HandleArray(m_tmpS4, 24);

	////第五行：拓展
	HandleArray(m_tmpS5, 32);

	////第六行：时间
	HandleArray(m_tmpS6, 40);

	////第七行：校验和
	std::string str1 = ByteArrayToHex(m_tmpS7, 1);
	std::string str2 = ByteArrayToHex(m_tmpS7, 0);
	if (str1 != "")
	{
		m_sX1[48] = str1.substr(0, 1);
		m_sX1[49] = str1.substr(1, 1);
	}
	else
	{
		m_sX1[48] = "0";
		m_sX1[49] = str1.substr(0, 1);
	}
	if (str2 != "")
	{
		m_sX1[50] = str2.substr(0, 1);
		m_sX1[51] = str2.substr(1, 1);
	}
	else
	{
		m_sX1[50] = "0";
		m_sX1[51] = str2.substr(0, 1);
	}

	//十六进制数据加密
	for (int i = 0; i < 52; i++)
	{
		if (m_sX1[i] == "0")
			m_sX2[i + 2] = 0x30;
		else if (m_sX1[i] == "1")
			m_sX2[i + 2] = 0x31;
		else if (m_sX1[i] == "2")
			m_sX2[i + 2] = 0x32;
		else if (m_sX1[i] == "3")
			m_sX2[i + 2] = 0x33;
		else if (m_sX1[i] == "4")
			m_sX2[i + 2] = 0x34;
		else if (m_sX1[i] == "5")
			m_sX2[i + 2] = 0x35;
		else if (m_sX1[i] == "6")
			m_sX2[i + 2] = 0x36;
		else if (m_sX1[i] == "7")
			m_sX2[i + 2] = 0x37;
		else if (m_sX1[i] == "8")
			m_sX2[i + 2] = 0x38;
		else if (m_sX1[i] == "9")
			m_sX2[i + 2] = 0x39;
		else if (m_sX1[i] == "a")
			m_sX2[i + 2] = 0x41;
		else if (m_sX1[i] == "b")
			m_sX2[i + 2] = 0x42;
		else if (m_sX1[i] == "c")
			m_sX2[i + 2] = 0x43;
		else if (m_sX1[i] == "d")
			m_sX2[i + 2] = 0x44;
		else if (m_sX1[i] == "e")
			m_sX2[i + 2] = 0x45;
		else if (m_sX1[i] == "f")
			m_sX2[i + 2] = 0x46;
	}

	//封装发送数据包

	m_sX2[0] = 0x24;
	m_sX2[1] = 0x1;
	m_sX2[54] = 0xD;
	m_sX2[55] = 0xA;
	return m_sX2;
}

bool SerialPortComm::DataPackAndWrite(int value)
{
	return m_serialPort.write((const  char*)DataPack(), value);
}

void SerialPortComm::WaitForBytesWritten(const int& ms)
{
	m_serialPort.waitForBytesWritten(ms);
}

void SerialPortComm::FloatToHex(unsigned char* str, int iRowNum, float fValue, int iSegSize)
{
	str[iRowNum * iSegSize] = fValue >= 0 ? 0x2B : 0x2D;
	// 保留位数
	int n = iSegSize - 3;
	for (int i = 1; i < 4; i++)
	{
		if (abs(fValue) >= pow(10, i))
		{
			n = iSegSize - (i + 3);
		}
	}
	float val = GetNFromData(abs(fValue), n);
	std::string data = std::to_string(val);
	for (int i = 0; i < iSegSize - 1; i++)
	{
		int value1 = (int)data[i];
		switch (value1)
		{
		case 46:
			str[iRowNum * iSegSize + i + 1] = 0x2E;
			break;
		case 48:
			str[iRowNum * iSegSize + i + 1] = 0x30;
			break;
		case 49:
			str[iRowNum * iSegSize + i + 1] = 0x31;
			break;
		case 50:
			str[iRowNum * iSegSize + i + 1] = 0x32;
			break;
		case 51:
			str[iRowNum * iSegSize + i + 1] = 0x33;
			break;
		case 52:
			str[iRowNum * iSegSize + i + 1] = 0x34;
			break;
		case 53:
			str[iRowNum * iSegSize + i + 1] = 0x35;
			break;
		case 54:
			str[iRowNum * iSegSize + i + 1] = 0x36;
			break;
		case 55:
			str[iRowNum * iSegSize + i + 1] = 0x37;
			break;
		case 56:
			str[iRowNum * iSegSize + i + 1] = 0x38;
			break;
		case 57:
			str[iRowNum * iSegSize + i + 1] = 0x39;
			break;
		default:
			str[iRowNum * iSegSize + i + 1] = 0x2A;//*号表示错误
			break;
		}
		//LOG(plog::debug) << "data:---------------------" << data;
		//LOG(plog::debug) << "str1:---------------------" << str1;
		//LOG(plog::debug) << "str:---------------------" << str;
	}
}

unsigned char* SerialPortComm::FloatToHex(float fValue1, float fValue2)
{
	unsigned char str[15]{};
	str[0] = fValue1 >= 0 ? 0x2B : 0x2D;
	str[7] = fValue2 >= 0 ? 0x2B : 0x2D;
	str[14] = 0x0D;
	std::string data1 = std::to_string(fValue1);
	std::string data2 = std::to_string(fValue2);
	for (int i = 1; i < 7; i++)
	{
		int value = (int)data1[i - 1];
		char* str1 = nullptr;
		itoa(value, str1, 16);
		str[i] = *str1;
	}
	for (int i = 8; i < 14; i++)
	{
		int value = (int)data2[i - 1];
		char* str1 = nullptr;
		itoa(value, str1, 16);
		str[i] = *str1;
	}

	return str;
}

void SerialPortComm::FixedDoubleToHex(unsigned char* str, int iRowNum, double dValue)
{
	// 科新 14B：前两字节固定 02 4C（与收端 rawHex 匹配「024C」及 MachinePeer 黄金帧对齐）
	str[iRowNum * 13 + 0] = 0x02;
	str[iRowNum * 13 + 1] = 0x4C;
	QString sData = QString::number(dValue, 'f', 5);
	//小数点位置
	int iIndex = sData.indexOf(".");
	if (iIndex <= 0)
	{
		for (int i = iRowNum * 13 + 2; i < iRowNum * 13 + 13; i++)
		{
			if (i == iRowNum * 13 + 7)
			{
				str[i] = 0x2E;
				continue;
			}
			str[i] = 0x30;
		}
	}
	else
	{
		int iStartPos = 0;
		if (iIndex > 4)
		{
			iStartPos = iIndex - 5;
		}
		QString sInteger = sData.mid(iStartPos, iIndex - iStartPos);
		if (dValue < 0)
		{
			sInteger = sInteger.mid(1);
		}
		QString sDecimal = "";
		int iLength = sData.size();
		if (iIndex + 1 < iLength)
		{
			sDecimal = sData.mid(iIndex + 1);
		}
		while (sInteger.size() < 5)
		{
			sInteger = "0" + sInteger;
		}
		while (sDecimal.size() < 5)
		{
			sDecimal = sDecimal + "0";
		}
		for (int i = iRowNum * 13 + 2; i < iRowNum * 13 + 7; i++)
		{
			int value = (int)sInteger.at(i - iRowNum * 13 - 2).toLatin1();
			if (dValue < 0 && i == iRowNum * 13 + 2)
			{
				str[i] = 0x2D;
			}
			else
			{
				str[i] = value;
			}
		}
		str[iRowNum * 13 + 7] = 0x2E;
		for (int i = iRowNum * 13 + 8; i < iRowNum * 13 + 13; i++)
		{
			int value = (int)sDecimal.at(i - iRowNum * 13 - 8).toLatin1();
			str[i] = value;
		}
	}
	//#
	str[iRowNum * 13 + 13] = 0x23;
}

QString SerialPortComm::formatValue(double value)
{
	// 符号位
	QChar sign = (value >= 0.0) ? QChar('+') : QChar('-');
	double absVal = std::fabs(value);


	// absVal 要变成 "000.0000"：宽度 8，小数 4 位，左侧补 0
	// arg(absVal, 8, 'f', 4, '0') 的含义：
	//  - 宽度 8
	//  - 'f'：固定小数
	//  - 4：保留 4 位小数
	//  - '0'：宽度不足用 '0' 填充
	QString numPart = QString("%1").arg(absVal, 8, 'f', 4, QLatin1Char('0'));

	// 拼上符号：±000.0000
	return QString("%1%2").arg(sign).arg(numPart);
}

double SerialPortComm::HexToFloat(QByteArray array)
{
	QByteArray ba;
	QByteArray byteTemp;
	std::string temp;
	//十六进制字符串解密
	for (int i = 0; i < array.size();)
	{
		byteTemp.clear();
		byteTemp.append(array[i]);
		byteTemp.append(array[i + 1]);
		i = i + 2;

		temp = byteTemp.toStdString();
		if (temp == "2D")
		{
			ba.append("-");
		}
		else if (temp == "2E")
		{
			ba.append(".");
		}
		else if (temp == "30")
		{
			ba.append("0");
		}
		else if (temp == "31")
		{
			ba.append("1");
		}
		else if (temp == "32")
		{
			ba.append("2");
		}
		else if (temp == "33")
		{
			ba.append("3");
		}
		else if (temp == "34")
		{
			ba.append("4");
		}
		else if (temp == "35")
		{
			ba.append("5");
		}
		else if (temp == "36")
		{
			ba.append("6");
		}
		else if (temp == "37")
		{
			ba.append("7");
		}
		else if (temp == "38")
		{
			ba.append("8");
		}
		else if (temp == "39")
		{
			ba.append("9");
		}
		/*	switch (temp)
			{
			case 0x2E:
				ba.append(".");
				break;
			case 0x30:
				ba.append("0");
				break;
			case 0x31:
				ba.append("1");
				break;
			case 0x32:
				ba.append("2");
				break;
			case 0x33:
				ba.append("3");
				break;
			case 0x34:
				ba.append("4");
				break;
			case 0x35:
				ba.append("5");
				break;
			case 0x36:
				ba.append("6");
				break;
			case 0x37:
				ba.append("7");
				break;
			case 0x38:
				ba.append("8");
				break;
			case 0x39:
				ba.append("9");
				break;
			default:
				break;
			}*/
	}
	double dValue = ba.toDouble();
	return dValue;
}

void SerialPortComm::HandleArray(unsigned char arr1[], int i)
{
	std::string str1 = ByteArrayToHex(arr1, 3);
	std::string str2 = ByteArrayToHex(arr1, 2);
	std::string str3 = ByteArrayToHex(arr1, 1);
	std::string str4 = ByteArrayToHex(arr1, 0);
	if (str1 != "")
	{
		m_sX1[i] = str1.substr(0, 1);
		m_sX1[i + 1] = str1.substr(1, 1);
	}
	else
	{
		m_sX1[i] = "0";
		m_sX1[i + 1] = str1.substr(0, 1);
	}
	if (str2 != "")
	{
		m_sX1[i + 2] = str2.substr(0, 1);
		m_sX1[i + 3] = str2.substr(1, 1);
	}
	else
	{
		m_sX1[i + 2] = "0";
		m_sX1[i + 3] = str2.substr(0, 1);
	}
	if (str3 != "")
	{
		m_sX1[i + 4] = str3.substr(0, 1);
		m_sX1[i + 5] = str3.substr(1, 1);
	}
	else
	{
		m_sX1[i + 4] = "0";
		m_sX1[i + 5] = str3.substr(0, 1);
	}
	if (str4 != "")
	{
		m_sX1[i + 6] = str4.substr(0, 1);
		m_sX1[i + 7] = str4.substr(1, 1);
	}
	else
	{
		m_sX1[i + 6] = "0";
		m_sX1[i + 7] = str4.substr(0, 1);
	}
}

std::string SerialPortComm::ByteArrayToHex(unsigned char arr[], int i)
{
	std::string hexstr = "";
	char hex1;
	char hex2;
	int value = arr[i];
	int v1 = value / 16;
	int v2 = value % 16;

	if (v1 >= 0 && v1 <= 9)
		hex1 = (char)(48 + v1);
	else
		hex1 = (char)(55 + v1);

	if (v2 >= 0 && v2 <= 9)
		hex2 = (char)(48 + v2);
	else
		hex2 = (char)(55 + v2);

	if (hex1 >= 'A' && hex1 <= 'F')
		hex1 += 32;

	if (hex2 >= 'A' && hex2 <= 'F')
		hex2 += 32;

	hexstr = hexstr + hex1 + hex2;
	return hexstr;
}

float SerialPortComm::GetNFromData(float val, int n)
{
	float temp1 = val;
	int t1 = (uint32_t)(temp1 * pow(10, n + 1)) % 10;
	if (t1 > 4)
	{
		temp1 = ((int)(temp1 * pow(10.0f, n)) + 1) / pow(10.0f, n);
	}
	else
	{
		temp1 = ((int)(temp1 * pow(10.0f, n))) / pow(10.0f, n);
	}
	return temp1;
}

bool SerialPortComm::handleFrame(const std::vector<uint8_t>& buf)
{
	uint8_t type;
	std::vector<uint8_t> payload;
	if (!m_SerLhgkProtocol.checkAndParseFrame(buf, type, payload) || type == 0xFF)
	{
		return false;
	}
	//uint8_t type = lhgkProto_.getFrameType(buf);
	//if (type == 0xFF) return false;

	// 去除帧头和 CRC，提取负载
	//std::vector<uint8_t> payload(buf.begin() + 5, buf.end() - 2);

	switch (type) {
	case 0x08: processNegotiationFrame(payload); break;
	case 0x02:	if (m_SerialPortInfo.input.iTriggerCtrl)processControlCommand(payload);		break;
	case 0x07: processDataStreamControl(payload); break;
	case 0x05: processPollingRequest(); break;
	case 0x01:  // 模块数据帧（数据流模式或轮询响应）
	{
		if (m_SerialPortInfo.input.iChannelState)
		{
			double* data = new double[m_SerialPortInfo.input.iChannelSize];
			double dforce = 0, dtemp = 0;
			// v1.1: payload长度除以10得到数据段个数
			uint8_t segmentCount = payload.size() / 10;
			m_SerLhgkProtocol.getExternalData(segmentCount, payload, dforce, dtemp);
			int itype = -1;
			for (int i = 0; i < m_SerialPortInfo.input.iChannelSize; i++)
			{
				if (0 == m_SerialPortInfo.input.iDataType[i])
				{
					data[i] = dforce;
					if (itype == 1)
					{
						itype = 2;
					}
					else {
						itype = 0;
					}
				}
				else {
					data[i] = dtemp;
					if (itype == 0)
					{
						itype = 2;
					}
					else {
						itype = 1;
					}
				}
			}
			emit emitNewData(data, m_SerialPortInfo.input.iChannelSize, itype);
			delete[]data;
			data = nullptr;
		}
		break;
	}
	case 0x04:  // v1.1错误帧
		// 处理错误帧（可选）
		break;
	default:
		sendErrorAndClose(0x01, "未知命令");
		return false;
	}
	return true;
}

// 联恒串口：按 AA55…0D0A 拼帧；断帧不报未解析，完整坏帧才报
void SerialPortComm::processLhgkRxStream(const QByteArray& chunk)
{
	constexpr int kMaxLhgkRxBytes = 64 * 1024;
	m_lhgkRxBuffer.append(chunk);
	if (m_lhgkRxBuffer.size() > kMaxLhgkRxBytes) {
		emit emitUnparsedRx(m_lhgkRxBuffer.left(4096));
		m_lhgkRxBuffer.clear();
		return;
	}

	while (!m_lhgkRxBuffer.isEmpty()) {
		const int sync = m_lhgkRxBuffer.indexOf(QByteArray("\xAA\x55", 2));
		if (sync < 0) {
			// 保留可能的半同步字节 0xAA，其余视为无法对齐的噪声
			if (m_lhgkRxBuffer.endsWith(char(0xAA))) {
				if (m_lhgkRxBuffer.size() > 1)
					emit emitUnparsedRx(m_lhgkRxBuffer.left(m_lhgkRxBuffer.size() - 1));
				m_lhgkRxBuffer = QByteArray(1, char(0xAA));
			} else {
				emit emitUnparsedRx(m_lhgkRxBuffer);
				m_lhgkRxBuffer.clear();
			}
			break;
		}
		if (sync > 0) {
			emit emitUnparsedRx(m_lhgkRxBuffer.left(sync));
			m_lhgkRxBuffer.remove(0, sync);
		}

		std::vector<uint8_t> probe(m_lhgkRxBuffer.begin(), m_lhgkRxBuffer.end());
		const int need = m_SerLhgkProtocol.probeCompleteFrameSize(probe);
		if (need == 0)
			break; // 断帧：继续等待后续字节
		if (need < 0) {
			// 同步点非法：丢弃 1 字节后重新搜头
			emit emitUnparsedRx(m_lhgkRxBuffer.left(1));
			m_lhgkRxBuffer.remove(0, 1);
			continue;
		}

		const QByteArray frame = m_lhgkRxBuffer.left(need);
		m_lhgkRxBuffer.remove(0, need);
		std::vector<uint8_t> frameVec(frame.begin(), frame.end());
		if (!handleFrame(frameVec))
			emit emitUnparsedRx(frame);
	}
}

void SerialPortComm::processNegotiationFrame(const std::vector<uint8_t>& payload)
{
	if (payload.size() < 3) {
		sendErrorAndClose(0x01, "协商帧无效");
		return;
	}

	//uint8_t type = payload[0];
	uint8_t major = payload[1];
	uint8_t minor = payload[2];

	// v1.1协议版本检查
	if (major != 1 || minor > 1) {
		sendErrorAndClose(0x02, "协议版本不支持");
		return;
	}

	std::cout << "协商成功：版本 " << int(major) << "." << int(minor) << std::endl;

	// 回复协商响应帧
	auto resp = m_SerLhgkProtocol.buildProtocolNegotiationFrame(false);
	sendData(resp);

	// 设置当前外部数据频率（回复一帧告知客户端）
	auto fps = m_SerLhgkProtocol.buildExternalDataRateFrame(m_dfreq);
	sendData(fps);
}

void SerialPortComm::processControlCommand(const std::vector<uint8_t>& payload)
{
	if (payload.empty()) return;
	uint8_t cmd = payload[0];

	if (cmd == 0x01)
	{
		if (m_SerialPortInfo.input.iTriggerCtrl)
		{
			// 开始计算 实时计算模式下
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
		}
		//isCalculating_ = true;
		//std::cout << "[状态] 开始计算" << std::endl;
	}
	else if (cmd == 0x02)
	{
		// 遗留问题： 先停止发送还是先停止计算？？？？
		// 停止计算 实时计算模式下
		if (m_SerialPortInfo.input.iTriggerCtrl)
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
		}
		//isCalculating_ = false;
		//std::cout << "[状态] 停止计算" << std::endl;
	}
	else {
		sendErrorAndClose(0x01, "非法控制命令");
	}
}

void SerialPortComm::processDataStreamControl(const std::vector<uint8_t>& payload)
{
	if (payload.empty()) return;

	uint8_t ctrl = payload[0];

	if (ctrl == 0x01) {
		// 开启数据流
		m_SerialPortInfo.bInquireSendFlag = true;
		std::cout << "[数据流] 开启数据流" << std::endl;
	}
	else if (ctrl == 0x02) {
		// 关闭数据流
		m_SerialPortInfo.bInquireSendFlag = false;
		std::cout << "[数据流] 关闭数据流" << std::endl;
	}
	else {
		sendErrorAndClose(0x01, "非法数据流控制命令");
		return;
	}

	emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATASTREAMING);
}

void SerialPortComm::processPollingRequest()
{
	emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATAPOLLING);
	std::cout << "[轮询] 收到轮询请求，发送当前数据" << std::endl;

	// v1.1: 轮询请求的响应使用模块数据帧（0x01），不存在独立的轮询响应帧
	// 示例：发送当前已设置模块的最新数据（力值和温度）
	/*std::vector<uint8_t> moduleId = {0x06, 0x06};
	std::vector<uint8_t> dataTypeId = {0x0C, 0x0D};
	std::vector<double> data = {123.4, 36.5};
	auto frame = m_SerLhgkProtocol.buildModuleDataFrame(moduleId, dataTypeId, data);
	sendData(frame);*/
}


void SerialPortComm::sendErrorAndClose(uint8_t errCode, const std::string& msg)
{
	auto err = m_SerLhgkProtocol.buildErrorFrame(errCode, msg);
	sendData(err);
}

void SerialPortComm::handleEventMsg(int ctrlCmd, int viewID, int msg)
{
	// 处理命令发送的事件消息
	emit emitEventMsg(ctrlCmd, viewID, msg);
}

void SerialPortComm::onRecvDataSlots()
{
	QByteArray readData = m_serialPort.readAll();
	QString rawData = readData.toHex().toUpper();


	switch (m_SerialPortInfo.iProtocolType)
	{
		/*case 0:
		{
			handleFrame(std::vector<uint8_t>(readData.begin(), readData.end()));
			return;
		}*/
	case 0:
	{
		if (m_SerialPortInfo.input.iTriggerCtrl)
		{
			if (rawData == "3D0D")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
				//SendOKResponse();
				return;
			}
			else if (rawData == "3E0D")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
				//SendOKResponse();
				return;
			}
			else if (rawData == "6B0D")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_AUTO_SAVE_IMAGE);
				return;
			}
			else if (rawData == "5A0D")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_ZERO_CLEARING);
				//SendOKResponse();
				return;
			}
			else if (rawData.startsWith("47"))
			{
				bool hasHorizontal = false;
				bool hasVertical = false;
				double horizontalGauge = 0.0;
				double verticalGauge = 0.0;
				int pos = 2; // Start after "47"

				// Parse horizontal gauge if present
				if (pos < rawData.length() && rawData.mid(pos, 2) == "48") // 'H'
				{
					pos += 2;
					if (pos < rawData.length() && rawData.mid(pos, 2) == "2B") // '+'
					{
						pos += 2;
						// Extract gauge value until we hit 'V' (56) or CR (0D)
						QString valueStr;
						while (pos < rawData.length())
						{
							QString byte = rawData.mid(pos, 2);
							if (byte == "56" || byte == "0D") // 'V' or CR
								break;
							// Convert hex to ASCII character
							bool ok;
							char ch = byte.toInt(&ok, 16);
							if (ok)
								valueStr.append(ch);
							pos += 2;
						}
						horizontalGauge = valueStr.toDouble();
						hasHorizontal = true;
					}
				}

				// Parse vertical gauge if present
				if (pos < rawData.length() && rawData.mid(pos, 2) == "56") // 'V'
				{
					pos += 2;
					if (pos < rawData.length() && rawData.mid(pos, 2) == "2B") // '+'
					{
						pos += 2;
						// Extract gauge value until we hit CR (0D)
						QString valueStr;
						while (pos < rawData.length())
						{
							QString byte = rawData.mid(pos, 2);
							if (byte == "0D") // CR
								break;
							// Convert hex to ASCII character
							bool ok;
							char ch = byte.toInt(&ok, 16);
							if (ok)
								valueStr.append(ch);
							pos += 2;
						}
						verticalGauge = valueStr.toDouble();
						hasVertical = true;
					}
				}

				// Check if command ends with CR (0D)
				if (rawData.endsWith("0D") && (hasHorizontal || hasVertical))
				{
					QVariantMap extra;
					extra["H"] = horizontalGauge;	// X
					extra["V"] = verticalGauge;		// Y

					emit emitEventMsgAndData(CommCtrlCmd::ControlSoftStatus, CommViewID::SerialPort_ID, W_CUSTOM_COMM_RESET_LVE_LENGTH, extra);

					// Send OK response
					//SendOKResponse();
					return;
				}
			}
		}
		int iLength = rawData.size();
		double* arraydata = new double[6];
		int iSize = 0;
		if (!m_SerialPortInfo.bInDataCombinating)
		{
			for (int i = iLength - 1; i >= 0; i--)//从后遍历，取最新值
			{
				if (rawData.at(i) == 'E' && i + 1 <= iLength - 1)
				{
					if (rawData.at(i + 1) == '2')
					{
						m_sInputData = rawData.mid(i + 2);
						if (m_sInputData.size() >= 8)
						{
							if (m_sInputData.size() > 8)
							{
								m_sInputData = m_sInputData.left(8);
							}
							MsgDataInput(m_sInputData, arraydata, iSize);
						}
						else
						{
							m_SerialPortInfo.bInDataCombinating = true;
						}
						//return;
					}
				}
			}
		}
		else
		{
			bool bHasHeader = false;
			for (int i = iLength - 1; i >= 0; i--)
			{
				if (rawData.at(i) == 'E' && i + 1 <= iLength - 1)
				{
					if (rawData.at(i + 1) == '2')
					{
						m_sInputData = rawData.mid(i + 2);
						bHasHeader = true;
						break;
					}
				}
			}
			if (!bHasHeader)
			{
				m_sInputData += rawData;
			}
			if (m_sInputData.size() >= 8)
			{
				if (m_sInputData.size() > 8)
				{
					m_sInputData = m_sInputData.left(8);
				}
				MsgDataInput(m_sInputData, arraydata, iSize);
				m_SerialPortInfo.bInDataCombinating = false;
			}
		}
		emit emitNewData(static_cast<void*>(arraydata), iSize, 0);

		delete[]arraydata;
		arraydata = nullptr;
		// 拼帧未完成时不报未解析；完成仍无数值且非已知控制指令时上报
		if (iSize <= 0 && !readData.isEmpty() && !m_SerialPortInfo.bInDataCombinating
			&& rawData != "3D0D" && rawData != "3E0D" && rawData != "6B0D" && rawData != "5A0D"
			&& !rawData.startsWith("47"))
		{
			emit emitUnparsedRx(readData);
		}
		break;
	}
	case 1:
	{
		int iLength = rawData.size();
		bool bMatch = false;
		// 科新控制字匹配成功后上报开始/停止（原先仅置 bHexSetZero，助手侧无事件）
		auto emitKexinCtrlIfMatched = [this](const QString& ctrlHex) {
			if (ctrlHex.size() < 12)
				return;
			const QChar sel = ctrlHex.at(11);
			if (sel == QLatin1Char('1'))
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
			else if (sel == QLatin1Char('2'))
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
		};
		if (!m_SerialPortInfo.bInDataCombinating)
		{
			for (int i = 0; i < iLength; i++)
			{
				if (rawData.at(i) == '7' && i + 1 <= iLength - 1)//从前遍历，保证指令完整
				{
					if (rawData.at(i + 1) == 'B')
					{
						m_sCtrlData = rawData.mid(i);
						if (m_sCtrlData.size() >= 16)
						{
							if (m_sCtrlData.size() > 16)
							{
								m_sCtrlData = m_sCtrlData.left(16);
							}
							bMatch = MatchCtrlData(m_sCtrlData);
							if (bMatch)
							{
								m_SerialPortInfo.bHexSetZero = true;
								emitKexinCtrlIfMatched(m_sCtrlData);
							}
						}
						else
						{
							m_SerialPortInfo.bInDataCombinating = true;
						}
					}
				}
			}
		}
		else
		{
			bool bHasHeader = false;
			for (int i = 0; i < iLength; i++)
			{
				if (rawData.at(i) == '7' && i + 1 <= iLength - 1)
				{
					if (rawData.at(i + 1) == 'B')
					{
						QString str = rawData.mid(0, i + 1);
						m_sCtrlData = m_sCtrlData + str;
						bMatch = MatchCtrlData(m_sCtrlData);
						if (bMatch)
						{
							m_SerialPortInfo.bHexSetZero = true;
							emitKexinCtrlIfMatched(m_sCtrlData);
						}
						else
						{
							m_sCtrlData = rawData.mid(i);
						}
						bHasHeader = true;
						break;
					}
				}
			}
			if (!bMatch)
			{
				if (!bHasHeader)
				{
					m_sCtrlData += rawData;
				}
				if (m_sCtrlData.size() >= 16)
				{
					if (m_sCtrlData.size() > 16)
					{
						m_sCtrlData = m_sCtrlData.left(16);
					}
					bMatch = MatchCtrlData(m_sCtrlData);
					if (bMatch)
					{
						m_SerialPortInfo.bHexSetZero = true;
						emitKexinCtrlIfMatched(m_sCtrlData);
					}
					m_SerialPortInfo.bInDataCombinating = false;
				}
			}
		}

		double* arraydata = new double[6];
		int iSize = 0;
		if (!m_SerialPortInfo.bInDataCombinating)
		{
			for (int i = iLength - 1; i >= 0; i--)
			{
				if (rawData.at(i) == '0' && i + 3 <= iLength - 1)
				{
					if (rawData.at(i + 1) == '2' && rawData.at(i + 2) == '4' && rawData.at(i + 3) == 'C')
					{
						m_sInputData = rawData.mid(i + 4);
						if (m_sInputData.size() >= 18)
						{
							if (m_sInputData.size() > 18)
							{
								m_sInputData = m_sInputData.left(18);
							}
							MsgDataInput(m_sInputData, arraydata, iSize);
						}
						else
						{
							m_SerialPortInfo.bInDataCombinating = true;
						}

					}
				}
			}
		}
		else
		{
			bool bHasHeader = false;
			for (int i = iLength - 1; i >= 0; i--)
			{
				if (rawData.at(i) == 'E' && i + 1 <= iLength - 1)
				{
					if (rawData.at(i + 1) == '2')
					{
						m_sInputData = rawData.mid(i + 2);
						bHasHeader = true;
						break;
					}
				}
			}
			if (!bHasHeader)
			{
				m_sInputData += rawData;
			}
			if (m_sInputData.size() >= 18)
			{
				if (m_sInputData.size() > 18)
				{
					m_sInputData = m_sInputData.left(18);
				}
				MsgDataInput(m_sInputData, arraydata, iSize);
				m_SerialPortInfo.bInDataCombinating = false;
			}
		}
		if (iSize > 0)
			emit emitNewData(static_cast<void*>(arraydata), iSize, 0);
		else if (!bMatch && !readData.isEmpty() && !m_SerialPortInfo.bInDataCombinating)
			emit emitUnparsedRx(readData);

		delete[]arraydata;
		arraydata = nullptr;

		break;
	}
	case 2:
	{
		// 等待接收数据完成
		g_serialBuffer += readData;

		while (true)
		{
			int pos = g_serialBuffer.indexOf("\r\n");
			if (pos == -1) break;

			// 提取 \r\n 之前的数据
			QByteArray segment = g_serialBuffer.left(pos);
			g_serialBuffer.remove(0, pos + 2);  // 删除这一帧 + \r\n
			// 转 ASCII
			QString dataStr = QString::fromLatin1(segment);
			QStringList parts = dataStr.split(',');

			if (parts.size() == 4) {
				double value = parts[0].toDouble();  // 37.0
				//int num = parts[1].toInt();       // 97
				QString type = parts[2];          // "CONSTANT"
				//int flag = parts[3].toInt();      // 0
				if (type == QString("CONSTANT") || type == QString("RUN"))
				{
					double* arraydata = new double;
					*arraydata = value;
					emit emitNewData(static_cast<void*>(arraydata), 1, 1);

					delete arraydata;
					arraydata = nullptr;
				}
				else if (!segment.isEmpty())
				{
					emit emitUnparsedRx(segment);
				}
			}
			else if (!segment.isEmpty())
			{
				emit emitUnparsedRx(segment);
			}
		}
		return;
	}
	case 3:
	{
		if (m_SerialPortInfo.input.iTriggerCtrl)
		{
			//int rescode = -1;	// rescode  = 0 开始  1 - 结束  2 - 退出软件 
			if (rawData == "24000D0A")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
			}
			else if (rawData == "24FF0D0A")
			{
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
			}
			else if (!readData.isEmpty())
			{
				emit emitUnparsedRx(readData);
			}
		}
		else if (!readData.isEmpty())
		{
			emit emitUnparsedRx(readData);
		}
		return;
	}
	case 5:
	{
		// 联恒光科串口：流式拼帧（断帧等待；完整但校验失败才未解析上报）
		processLhgkRxStream(readData);
		return;
	}
	default:
		if (!readData.isEmpty())
			emit emitUnparsedRx(readData);
		break;
	}



	if (0 == m_SerialPortInfo.input.iTransferType)
	{
		if (rawData == "3F0D")
		{
			m_SerialPortInfo.bInquireSendFlag = true;
			return;
		}
	}
}
