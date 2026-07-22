#include "SocketComm.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SocketComm::SocketComm()
{
	ConnectSlots();
	initParameters();
	initInfo();
}

SocketComm::~SocketComm()
{}

void SocketComm::SetToDefault()
{
	initInfo();
}

void SocketComm::ConnectSlots()
{
	QObject::connect(&m_controlBox, &NetworkBox::emitNewConn, this, &SocketComm::onNewConnIpAndPortSlots);				// 新客户端连接到本地服务器
	QObject::connect(&m_controlBox, &NetworkBox::emitNewMessage, this, &SocketComm::onRecvDataSlots);				    // 接收到新消息触发
	QObject::connect(&m_controlBox, &NetworkBox::emitDisConnected, this, &SocketComm::onClientDisConnSlots);			// TCP断开连接			
}


bool SocketComm::SetSysParInfo(void* ptr)
{
	if (ptr)
	{
		m_socketInfo = *(static_cast<SocketInfo*>(ptr));
		return true;
	}
	return false;
}

bool SocketComm::GetSysParInfo(void* ptr) const
{
	if (ptr)
	{
		// 将 void* 转换为 SocketInfo* 并赋值
		SocketInfo* socketInfoPtr = static_cast<SocketInfo*>(ptr);
		*socketInfoPtr = m_socketInfo;
		return true;
	}
	return false;
}

void SocketComm::setParameter(const QString& key, const Any& value)
{
	auto it = setters.find(key);
	if (it != setters.end()) {
		it->second(value);
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

Any SocketComm::getParameter(const QString& key) const
{
	auto it = getters.find(key);
	if (it != getters.end()) {
		return it->second();
	}
	else {
		throw std::invalid_argument("Invalid key");
	}
}

bool SocketComm::setExternalFreq(const int& freq)
{
	// 设置当前外部数据频率（回复一帧告知客户端）
	m_dfreq = freq;
	sendData(lhgkProto_.buildExternalDataRateFrame(m_dfreq));
	return true;
}


bool SocketComm::Connect(int type)
{
	Q_UNUSED(type);
	bool bFlag = false;
	m_socketInfo.bConntction = false;

	if (m_socketInfo.iTransferType == 0) // tcp
	{
		if (m_socketInfo.iModel == 0) // 服务器端：listen 成功即本地就绪（尚无对端会话）
		{
			m_controlBox.SetNetType(NetworkBox::iTcpServer);
			bFlag = m_controlBox.initConnection(m_socketInfo.sLocalIP, m_socketInfo.iLocalPort);
		}
		else // 客户端：必须 TCP 握手成功，对端拒绝/超时一律失败
		{
			m_controlBox.SetNetType(NetworkBox::iTcpClient);
			if (m_socketInfo.sPurIP.trimmed().isEmpty() || m_socketInfo.iPurPort <= 0
				|| m_socketInfo.iPurPort > 65535) {
				return false;
			}
			bFlag = m_controlBox.initConnection(m_socketInfo.sPurIP, m_socketInfo.iPurPort);
		}
	}
	else // udp：以本地 bind 成功为准（无连接态）
	{
		m_controlBox.SetNetType(NetworkBox::iUdp);
		m_controlBox.SetDestIpAndPort(QHostAddress(m_socketInfo.sPurIP),
									  static_cast<quint16>(m_socketInfo.iPurPort));
		bFlag = m_controlBox.initConnection(m_socketInfo.sLocalIP, m_socketInfo.iLocalPort);
	}

	m_socketInfo.bConntction = bFlag;
	return bFlag;
}

bool SocketComm::Disconnect(int type)
{
	m_socketInfo.bOnlineCollect = false;
	return m_controlBox.closeConnection();
}

//void SocketComm::SendData(void* ptr, size_t size)
//{
//	//QByteArray byteArray(); // 使用 QByteArray 处理二进制数据
//	//QString qstr = QString::fromUtf8(byteArray); // 从 QByteArray 转换为 QString
//	///qint64 length = static_cast<qint64>(size);
//	const char* data = static_cast<const char*>(ptr);
//	m_controlBox.sendData(data, strlen(data));
//}
//
//void SocketComm::SendData(const QString& data)
//{
//	m_controlBox.sendData(data);
//}

void SocketComm::SendData(const std::vector<double>& vData)
{
	if (!m_socketInfo.bInquireSendFlag)
	{
		return;
	}

	QString sdata = "";
	switch (m_socketInfo.iProtoType)
	{
		//case 0:
		//{
		//	std::vector < uint8_t> moduleId, dataTypeId;
		//	//CommFunc::GetFromStageData(m_socketInfo.output, moduleId, dataTypeId);
		//	sendData(lhgkProto_.buildModuleDataFrame(moduleId, dataTypeId, vData));
		//	break;
		//}
	case 0:	// json
	{
		sdata = CreateJsonDataPack(vData);
		break;
	}
	case 1:
	{
		sdata = CreateWanceSimbolDataPack(vData);
		break;
	}
	case 2:
	{
		// 中机：二进制 ServerData 直发，禁止经 QString/toLatin1 以免损坏线帧
		if (vData.size() != 2)
			return;
		SocketProtol::ServerData serdata{};
		serdata.msgType = SocketProtol::SERVER_DATA_MESSAGE;
		serdata.valueX = vData.at(0);
		serdata.valueY = vData.at(1);
		m_controlBox.sendData(reinterpret_cast<const char*>(&serdata),
							  static_cast<qint64>(sizeof(serdata)));
		if (m_socketInfo.output.iTransferType == 1)
			m_socketInfo.bInquireSendFlag = false;
		return;
	}
	case 6:
	{
		sdata = CreateHNBCDataPack(vData);
		break;
	}
	case 7:
	{
		if (vData.size() == 4)
		{
			sdata = GenJsonDocument(vData.at(0), vData.at(1), vData.at(2), vData.at(3));
		}
		break;
	}
	case 8:
	{
		// 联恒光科网口（BASELINE V1.2.5 / iProtoType=8）：至少力、温两路，二进制帧直发
		if (vData.size() < 2)
		{
			return;
		}
		sendData(lhgkProto_.buildExternalDataFrame(vData.at(0), vData.at(1)));
		break;
	}
	default:
		break;
	}

	if (!sdata.isEmpty())
	{
		m_controlBox.sendData(sdata);
	}

	if (m_socketInfo.output.iTransferType == 1)
	{
		m_socketInfo.bInquireSendFlag = false;
	}

}

bool SocketComm::SaveParInfo(const QString& filename)
{
	std::ofstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) return false;

	// 简单类型
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iTransferType), sizeof(m_socketInfo.iTransferType));
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iModel), sizeof(m_socketInfo.iModel));

	// QStringList AllIp
	/*int allIpCount = m_socketInfo.AllIp.size();
	file.write(reinterpret_cast<const char*>(&allIpCount), sizeof(allIpCount));
	for (const QString& ip : m_socketInfo.AllIp) {
		QByteArray ipBytes = ip.toUtf8();
		int len = ipBytes.size();
		file.write(reinterpret_cast<const char*>(&len), sizeof(len));
		file.write(ipBytes.data(), len);
	}*/

	// QString sLocalIP
	QByteArray localIPBytes = m_socketInfo.sLocalIP.toUtf8();
	int localIPLength = localIPBytes.size();
	file.write(reinterpret_cast<const char*>(&localIPLength), sizeof(localIPLength));
	file.write(localIPBytes.data(), localIPLength);

	// int iLocalPort
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iLocalPort), sizeof(m_socketInfo.iLocalPort));

	// QString sPurIP
	QByteArray destIPBytes = m_socketInfo.sPurIP.toUtf8();
	int destIPLength = destIPBytes.size();
	file.write(reinterpret_cast<const char*>(&destIPLength), sizeof(destIPLength));
	file.write(destIPBytes.data(), destIPLength);

	// int iPurPort
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iPurPort), sizeof(m_socketInfo.iPurPort));

	// int iProtoType
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iProtoType), sizeof(m_socketInfo.iProtoType));

	// bool bOpenCMD[3]
	for (int i = 0; i < 3; ++i) {
		char b = m_socketInfo.bOpenCMD[i] ? 1 : 0;
		file.write(&b, 1);
	}

	// char cCMD[3][16]
	file.write(reinterpret_cast<const char*>(&m_socketInfo.cCMD), sizeof(m_socketInfo.cCMD));

	// int iSymbolType
	file.write(reinterpret_cast<const char*>(&m_socketInfo.iSymbolType), sizeof(m_socketInfo.iSymbolType));

	// bool bOnlineCollect, bAllOpenCamera, bConntction, bInquireSendFlag
	char bOnlineCollect = m_socketInfo.bOnlineCollect ? 1 : 0;
	char bAllOpenCamera = m_socketInfo.bAllOpenCamera ? 1 : 0;
	char bConntction = m_socketInfo.bConntction ? 1 : 0;
	char bInquireSendFlag = m_socketInfo.bInquireSendFlag ? 1 : 0;
	file.write(&bOnlineCollect, 1);
	file.write(&bAllOpenCamera, 1);
	file.write(&bConntction, 1);
	file.write(&bInquireSendFlag, 1);

	// DataInput
	file.write(reinterpret_cast<const char*>(&m_socketInfo.input), sizeof(m_socketInfo.input));
	// DataOutput
	file.write(reinterpret_cast<const char*>(&m_socketInfo.output), sizeof(m_socketInfo.output));

	file.close();
	return true;
}

bool SocketComm::LoadParInfo(const QString& filename)
{
	std::ifstream file(filename.toLocal8Bit().constData(), std::ios::binary);
	if (!file.is_open()) return false;

	// 简单类型
	file.read(reinterpret_cast<char*>(&m_socketInfo.iTransferType), sizeof(m_socketInfo.iTransferType));
	file.read(reinterpret_cast<char*>(&m_socketInfo.iModel), sizeof(m_socketInfo.iModel));

	// QStringList AllIp
	/*int allIpCount = 0;
	file.read(reinterpret_cast<char*>(&allIpCount), sizeof(allIpCount));
	m_socketInfo.AllIp.clear();
	for (int i = 0; i < allIpCount; ++i) {
		int len = 0;
		file.read(reinterpret_cast<char*>(&len), sizeof(len));
		QByteArray ipBytes(len, 0);
		file.read(ipBytes.data(), len);
		m_socketInfo.AllIp << QString::fromUtf8(ipBytes);
	}*/
	m_socketInfo.AllIp = m_controlBox.GetHostAllIPAddress();

	// QString sLocalIP
	int localIPLength = 0;
	file.read(reinterpret_cast<char*>(&localIPLength), sizeof(localIPLength));
	QByteArray localIPBytes(localIPLength, 0);
	file.read(localIPBytes.data(), localIPLength);
	m_socketInfo.sLocalIP = QString::fromUtf8(localIPBytes);
	//m_socketInfo.sLocalIP = m_socketInfo.AllIp.at(0);

	// int iLocalPort
	file.read(reinterpret_cast<char*>(&m_socketInfo.iLocalPort), sizeof(m_socketInfo.iLocalPort));

	// QString sPurIP
	int destIPLength = 0;
	file.read(reinterpret_cast<char*>(&destIPLength), sizeof(destIPLength));
	QByteArray destIPBytes(destIPLength, 0);
	file.read(destIPBytes.data(), destIPLength);
	m_socketInfo.sPurIP = QString::fromUtf8(destIPBytes);

	// int iPurPort
	file.read(reinterpret_cast<char*>(&m_socketInfo.iPurPort), sizeof(m_socketInfo.iPurPort));

	// int iProtoType
	file.read(reinterpret_cast<char*>(&m_socketInfo.iProtoType), sizeof(m_socketInfo.iProtoType));

	// bool bOpenCMD[3]
	for (int i = 0; i < 3; ++i) {
		char b = 0;
		file.read(&b, 1);
		m_socketInfo.bOpenCMD[i] = (b != 0);
	}

	// char cCMD[3][16]
	file.read(reinterpret_cast<char*>(&m_socketInfo.cCMD), sizeof(m_socketInfo.cCMD));

	// int iSymbolType
	file.read(reinterpret_cast<char*>(&m_socketInfo.iSymbolType), sizeof(m_socketInfo.iSymbolType));

	// bool bOnlineCollect, bAllOpenCamera, bConntction, bInquireSendFlag
	char bOnlineCollect = 0, bAllOpenCamera = 0, bConntction = 0, bInquireSendFlag = 0;
	file.read(&bOnlineCollect, 1);
	file.read(&bAllOpenCamera, 1);
	file.read(&bConntction, 1);
	file.read(&bInquireSendFlag, 1);
	/*m_socketInfo.bOnlineCollect = (bOnlineCollect != 0);
	m_socketInfo.bAllOpenCamera = (bAllOpenCamera != 0);
	m_socketInfo.bConntction = (bConntction != 0);
	m_socketInfo.bInquireSendFlag = (bInquireSendFlag != 0);*/

	m_socketInfo.bOnlineCollect = false;
	m_socketInfo.bAllOpenCamera = false;
	m_socketInfo.bConntction = false;
	m_socketInfo.bInquireSendFlag = false;

	// DataInput
	file.read(reinterpret_cast<char*>(&m_socketInfo.input), sizeof(m_socketInfo.input));
	// DataOutput
	file.read(reinterpret_cast<char*>(&m_socketInfo.output), sizeof(m_socketInfo.output));
	file.close();

	//m_socketInfo.input.iChannelState = 0;
	//m_socketInfo.input.iChannelSize = 1;
	//m_socketInfo.input.iTriggerCtrl = 0;
	//m_socketInfo.input.iTransferType = 0;
	//for (int i = 0; i < 4; i++)
	//{
	//	m_socketInfo.input.iDataType[i] = 0;
	//	m_socketInfo.input.iUnit[i] = 0;
	//}

	//m_socketInfo.output.iChannelState = 0;
	//m_socketInfo.output.iChannelSize = 1;
	//m_socketInfo.output.iTriggerCtrl = 0;
	//m_socketInfo.output.iTransferType = 0;
	//for (int i = 0; i < 8; i++)
	//{
	//	m_socketInfo.output.iDataModel[i] = 0;
	//	m_socketInfo.output.iDataType[i] = 0;
	//	m_socketInfo.output.iData[i] = 0;
	//	m_socketInfo.output.bReverse[i] = 0;
	//	//m_socketInfo.output.iUnit[i] = 0;
	//}

	return true;
}


bool SocketComm::saveCmdToFile(const std::string& filename)
{
	return true;
}

bool SocketComm::loadCmdFromFile(const std::string& filename)
{
	return true;
}

void SocketComm::initParameters()
{
	setters["iTransferType"] = [this](const Any& value) { m_socketInfo.iTransferType = value.any_cast<int>(); };
	setters["iModel"] = [this](const Any& value) { m_socketInfo.iModel = value.any_cast<int>(); };
	setters["AllIp"] = [this](const Any& value) { m_socketInfo.AllIp = value.any_cast<QStringList>(); };
	setters["sLocalIP"] = [this](const Any& value) { m_socketInfo.sLocalIP = value.any_cast<QString>(); };
	setters["iLocalPort"] = [this](const Any& value) { m_socketInfo.iLocalPort = value.any_cast<int>(); };
	setters["sDestIP"] = [this](const Any& value) { m_socketInfo.sPurIP = value.any_cast<QString>(); };
	setters["iDestPort"] = [this](const Any& value) { m_socketInfo.iPurPort = value.any_cast<int>(); };
	setters["iProtoType"] = [this](const Any& value) {
		m_socketInfo.iProtoType = value.any_cast<int>();
		// 协议切换清空拼帧缓冲，避免旧半包污染
		m_lhgkRxBuffer.clear();
	};
	setters["bOpenCMD"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<bool, 3>>();
		std::copy(arr.begin(), arr.end(), std::begin(m_socketInfo.bOpenCMD));
		};
	setters["cCMD"] = [this](const Any& value) {
		auto arr = value.any_cast<std::array<std::array<char, 16>, 3>>();
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 16; j++)
				m_socketInfo.cCMD[i][j] = arr[i][j];
		}
		};
	setters["iSymbolType"] = [this](const Any& value) { m_socketInfo.iSymbolType = value.any_cast<int>(); };
	setters["bOnlineCollect"] = [this](const Any& value) { m_socketInfo.bOnlineCollect = value.any_cast<bool>(); };
	setters["bAllOpenCamera"] = [this](const Any& value) { m_socketInfo.bAllOpenCamera = value.any_cast<bool>(); };
	setters["bConntction"] = [this](const Any& value) { m_socketInfo.bConntction = value.any_cast<bool>(); };
	setters["bInquireSendFlag"] = [this](const Any& value) { m_socketInfo.bInquireSendFlag = value.any_cast<bool>(); };
	// 助手观测开关：默认 false，正式软件不设则零语义变化
	setters["bAssistObserve"] = [this](const Any& value) { m_bAssistObserve = value.any_cast<bool>(); };
	setters["input"] = [this](const Any& value) { m_socketInfo.input = value.any_cast<DataInput>(); };
	setters["output"] = [this](const Any& value) { m_socketInfo.output = value.any_cast<DataOutput>(); };

	getters["iTransferType"] = [this]() -> Any { return m_socketInfo.iTransferType; };
	getters["iModel"] = [this]() -> Any { return m_socketInfo.iModel; };
	getters["AllIp"] = [this]() -> Any { return m_socketInfo.AllIp; };
	getters["sLocalIP"] = [this]() -> Any { return m_socketInfo.sLocalIP; };
	getters["iLocalPort"] = [this]() -> Any { return m_socketInfo.iLocalPort; };
	getters["sDestIP"] = [this]() -> Any { return m_socketInfo.sPurIP; };
	getters["iDestPort"] = [this]() -> Any { return m_socketInfo.iPurPort; };
	getters["iProtoType"] = [this]() -> Any { return m_socketInfo.iProtoType; };
	getters["bOpenCMD"] = [this]() -> Any {
		std::array<bool, 3> arr;
		std::copy(std::begin(m_socketInfo.bOpenCMD), std::end(m_socketInfo.bOpenCMD), arr.begin());
		return arr;
		};
	getters["cCMD"] = [this]() -> Any {
		std::array<std::array<char, 16>, 3> arr;
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 16; ++j) {
				arr[i][j] = m_socketInfo.cCMD[i][j];
			}
		}
		return arr;
		};
	getters["iSymbolType"] = [this]() -> Any { return m_socketInfo.iSymbolType; };
	getters["bOnlineCollect"] = [this]() -> Any { return m_socketInfo.bOnlineCollect; };
	getters["bAllOpenCamera"] = [this]() -> Any { return m_socketInfo.bAllOpenCamera; };
	getters["bConntction"] = [this]() -> Any { return m_socketInfo.bConntction; };
	getters["bInquireSendFlag"] = [this]() -> Any { return m_socketInfo.bInquireSendFlag; };
	getters["bAssistObserve"] = [this]() -> Any { return m_bAssistObserve; };
	getters["input"] = [this]() -> Any { return m_socketInfo.input; };
	getters["output"] = [this]() -> Any { return m_socketInfo.output; };
}

void SocketComm::initInfo()
{
	m_dfreq = 20;
	m_socketInfo.iTransferType = 0;
	m_socketInfo.iModel = 0;		// 服务器端
	m_socketInfo.AllIp = m_controlBox.GetHostAllIPAddress();		// 服务器端
	m_socketInfo.sLocalIP = "127.0.0.1";
	m_socketInfo.iLocalPort = 8005;
	m_socketInfo.sPurIP = "127.0.0.1";
	m_socketInfo.iPurPort = 8806;
	m_socketInfo.iProtoType = 0;
	m_socketInfo.bOpenCMD[0] = 1;
	m_socketInfo.bOpenCMD[1] = 0;
	m_socketInfo.bOpenCMD[2] = 0;
	for (auto& range : m_socketInfo.cCMD) {
		for (auto& r : range)
		{
			r = '\0';
		}
	}
	m_socketInfo.iSymbolType = 1;
	m_socketInfo.bOnlineCollect = false;
	m_socketInfo.bAllOpenCamera = false;
	m_socketInfo.bConntction = false;
	m_socketInfo.bInquireSendFlag = false;
	m_bAssistObserve = false;

	m_socketInfo.input.iChannelState = 0;
	m_socketInfo.input.iChannelSize = 1;
	m_socketInfo.input.iTriggerCtrl = 0;
	m_socketInfo.input.iTransferType = 0;
	for (int i = 0; i < 4; i++)
	{
		m_socketInfo.input.iDataType[i] = 0;
		m_socketInfo.input.iUnit[i] = 0;
	}

	m_socketInfo.output.iChannelState = 0;
	m_socketInfo.output.iChannelSize = 1;
	m_socketInfo.output.iTriggerCtrl = 0;
	m_socketInfo.output.iTransferType = 0;
	for (int i = 0; i < 8; i++)
	{
		m_socketInfo.output.iDataModel[i] = 0;
		m_socketInfo.output.iDataType[i] = 0;
		m_socketInfo.output.iData[i] = 1;
		m_socketInfo.output.bReverse[i] = 0;
		//m_socketInfo.output.iUnit[i] = 0;
	}
}

QString SocketComm::CreateJsonDataPack(const std::vector<double>& data)
{
	QJsonObject jsonObj;
	QJsonDocument jsonDoc;
	QJsonArray array;
	// 有数据到来 
	for (int i = 0; i < data.size(); i++)
	{
		array.append(QString::number(data.at(i), 'f', 12).toDouble());		// 应变
	}
	// 插入数据
	jsonObj.insert("code", 0);
	jsonObj.insert("values", array);
	jsonObj.insert("tn", 2);
	jsonDoc.setObject(jsonObj);

	return  jsonDoc.toJson(QJsonDocument::Compact);
}

QString SocketComm::CreateZhongJiShuangZhouFour(const std::vector<double>& data)
{
	if (data.size() != 2)
	{
		return "";
	}
	SocketProtol::ServerData serdata;
	serdata.valueY = serdata.valueX = -1;
	serdata.msgType = SocketProtol::SERVER_DATA_MESSAGE;
	serdata.valueX = data.at(0);
	serdata.valueY = data.at(1);
	QByteArray byteArray(reinterpret_cast<const char*>(&serdata), sizeof(SocketProtol::ServerData));
	return byteArray;
}

QString SocketComm::CreateWanceSimbolDataPack(const std::vector<double>& data)
{
	QString symbol = ",";
	QString msg = "";
	switch (m_socketInfo.iSymbolType)
	{
	case 1:
		symbol = ",";
		break;
	case 2:
		symbol = " ";
		break;
	case 3:
		symbol = ";";
		break;
	default:break;
	}

	for (int i = 0; i < data.size(); i++)
	{
		msg += QString::number(data.at(i));
		if (i != data.size() - 1)
		{
			msg += symbol;
		}
	}
	return msg;
}

QString SocketComm::CreateHNBCDataPack(const std::vector<double>& data)
{
	QJsonObject jsonObj;
	QJsonArray jsonArray;
	for (double value : data)
	{
		jsonArray.append(value);
	}
	jsonObj.insert("values", jsonArray);
	jsonObj.insert("name", "LVEComputedValues");
	QJsonDocument doc(jsonObj);
	// Compact 格式：去掉换行和空格
	return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QString SocketComm::GenJsonDocument(double L1, double b1, double Le1, double bo1)
{
	// 创建 JSON 对象
	QJsonObject hsmObject;

	// 设置固定为0的字段
	hsmObject["L2"] = "0";
	hsmObject["b2"] = "0";
	hsmObject["Le2"] = "0";
	hsmObject["bo2"] = "0";
	hsmObject["bu"] = "0";
	hsmObject["val"] = "0";
	hsmObject["YMaxval"] = "0";
	hsmObject["XMaxval"] = "0";

	// 填充 JSON 对象的键值对，仅处理有变量数据的字段
	hsmObject["L1"] = QString::number(L1);
	hsmObject["b1"] = QString::number(b1);
	hsmObject["Le1"] = QString::number(Le1);
	hsmObject["bo1"] = QString::number(bo1);

	// 创建包含 hsmObject 的外层 JSON 对象
	QJsonObject rootObject;
	rootObject["hsm"] = hsmObject;
	// 创建 JSON 文档
	QJsonDocument doc(rootObject);

	// 返回 JSON 字符串
	return QString(doc.toJson(QJsonDocument::Compact)); // Compact 返回紧凑形式，没有多余换行

}

void SocketComm::onNewConnIpAndPortSlots(QString IP, int port)
{
	int iTransferType = 0;
	if (m_socketInfo.iTransferType == 0)
	{
		if (m_socketInfo.iModel == 0)
		{
			iTransferType = 0;
		}
		else
		{
			iTransferType = 1;
		}
	}
	else
	{
		if (m_socketInfo.iModel == 0)
		{
			iTransferType = 2;
		}
		else
		{
			iTransferType = 3;
		}
	}
	emit emitNewConn(iTransferType, IP, port);
}

void SocketComm::onClientDisConnSlots(qintptr)
{
	emit emitClientDisConn();
}

void SocketComm::onRecvDataSlots(QByteArray data)
{
	switch (m_socketInfo.iProtoType)
	{
		//case 0:	// 联恒
		//{
		//	handleFrame(std::vector<uint8_t>(data.begin(), data.end()));
		//	break;
		//}
	case 0: //json
	{
		ParsingProtocolI(data);
		break;
	}
	case 1: // 万测
	{
		ParsingProtocolII(data);
		break;
	}
	case 2: // 中机四通道
	{
		ParsingProtocolZhongji(data);
		break;
	}
	case 3: // 三思试验
	{
		ParsingProtocolSansi(data);
		break;
	}
	case 4:	// 触发存图
	{
		std::string sCommandData = data.toStdString();
		if (sCommandData.find("\r\n") != std::string::npos)
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_SINGALTRIIMAGESAVE);
			//m_controlBox.closeConnection();
		}
		else if (!data.isEmpty())
		{
			emitAssistUnparsedRx(data);
		}
		break;
	}
	case 5: // 福建威盛
	{
		TestData testdata;
		if (parseFuJianWeishengData(data, testdata))
		{
			double* arraydata = new double;
			*arraydata = testdata.load;
			emit emitNewData(static_cast<void*>(arraydata), 1, 0);

			delete arraydata;
			arraydata = nullptr;
		}
		else if (!data.isEmpty())
		{
			emitAssistUnparsedRx(data);
		}
		break;
	}
	case 6: // 纳百川全自动化
	{
		paeseNaBaiChuanData(data);
		break;
	}
	case 7: // 纳百川识别线条
	{
		paeseNaBaiChuanDataCalcLine(data);
		break;
	}
	case 8: // 联恒光科（不占用历史 case 0 / JSON）
	{
		// TCP 半包/粘包：流式拼帧后再判定未解析
		processLhgkRxStream(data);
		break;
	}
	default:
		if (!data.isEmpty())
			emitAssistUnparsedRx(data);
		break;
	}
}

void SocketComm::handleEventMsg(int ctrlCmd, int viewID, int msg)
{
	// 处理命令发送的事件消息
	emit emitEventMsg(ctrlCmd, viewID, msg);
}

// 联恒网口：按 AA55…0D0A 拼帧；断帧不报未解析，完整坏帧才报
void SocketComm::processLhgkRxStream(const QByteArray& chunk)
{
	constexpr int kMaxLhgkRxBytes = 64 * 1024;
	m_lhgkRxBuffer.append(chunk);
	if (m_lhgkRxBuffer.size() > kMaxLhgkRxBytes) {
		emitAssistUnparsedRx(m_lhgkRxBuffer.left(4096));
		m_lhgkRxBuffer.clear();
		return;
	}

	while (!m_lhgkRxBuffer.isEmpty()) {
		const int sync = m_lhgkRxBuffer.indexOf(QByteArray("\xAA\x55", 2));
		if (sync < 0) {
			if (m_lhgkRxBuffer.endsWith(char(0xAA))) {
				if (m_lhgkRxBuffer.size() > 1)
					emitAssistUnparsedRx(m_lhgkRxBuffer.left(m_lhgkRxBuffer.size() - 1));
				m_lhgkRxBuffer = QByteArray(1, char(0xAA));
			} else {
				emitAssistUnparsedRx(m_lhgkRxBuffer);
				m_lhgkRxBuffer.clear();
			}
			break;
		}
		if (sync > 0) {
			emitAssistUnparsedRx(m_lhgkRxBuffer.left(sync));
			m_lhgkRxBuffer.remove(0, sync);
		}

		std::vector<uint8_t> probe(m_lhgkRxBuffer.begin(), m_lhgkRxBuffer.end());
		const int need = lhgkProto_.probeCompleteFrameSize(probe);
		if (need == 0)
			break;
		if (need < 0) {
			emitAssistUnparsedRx(m_lhgkRxBuffer.left(1));
			m_lhgkRxBuffer.remove(0, 1);
			continue;
		}

		const QByteArray frame = m_lhgkRxBuffer.left(need);
		m_lhgkRxBuffer.remove(0, need);
		std::vector<uint8_t> frameVec(frame.begin(), frame.end());
		if (!handleFrame(frameVec))
			emitAssistUnparsedRx(frame);
	}
}

bool SocketComm::handleFrame(const std::vector<uint8_t>& buf)
{
	uint8_t type;
	std::vector<uint8_t> payload;
	if (!lhgkProto_.checkAndParseFrame(buf, type, payload) || type == 0xFF)
	{
		return false;
	}
	//uint8_t type = lhgkProto_.getFrameType(buf);
	//if (type == 0xFF) return false;

	// 去除帧头和 CRC，提取负载
	//std::vector<uint8_t> payload(buf.begin() + 5, buf.end() - 2);

	switch (type) {
	case 0x08: processNegotiationFrame(payload); break;
	case 0x02: if (m_socketInfo.input.iTriggerCtrl)processControlCommand(payload); break;
	case 0x07: processDataStreamControl(payload); break;
	case 0x05: processPollingRequest(); break;
	case 0x01:
	{
		if (m_socketInfo.input.iChannelState)
		{
			double* data = new double[m_socketInfo.input.iChannelSize];
			double dforce = 0, dtemp = 0;
			int itype = -1;
			lhgkProto_.getExternalData(buf[4], payload, dforce, dtemp);
			for (int i = 0; i < m_socketInfo.input.iChannelSize; i++)
			{
				if (0 == m_socketInfo.input.iDataType[i])
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
			emit emitNewData(data, m_socketInfo.input.iChannelSize, itype);
			delete[]data;
			data = nullptr;
		}
		break;
	}
	default:
		sendErrorAndClose(0x01, "未知命令");
		return false;
	}
	return true;
}


void SocketComm::processNegotiationFrame(const std::vector<uint8_t>& payload)
{
	if (payload.size() < 3) {
		sendErrorAndClose(0x01, "协商帧无效");
		return;
	}

	uint8_t type = payload[0];
	uint8_t major = payload[1];
	uint8_t minor = payload[2];

	if (major != 1 || minor != 3) {
		sendErrorAndClose(0x02, "协议版本不支持");
		return;
	}

	std::cout << "协商成功：版本 " << int(major) << "." << int(minor) << std::endl;

	// 回复协商响应帧
	auto resp = lhgkProto_.buildProtocolNegotiationFrame(false);
	sendData(resp);

	// 设置当前外部数据频率（回复一帧告知客户端）
	auto fps = lhgkProto_.buildExternalDataRateFrame(m_dfreq);
	sendData(fps);
}

void SocketComm::processControlCommand(const std::vector<uint8_t>& payload)
{
	if (payload.empty()) return;
	uint8_t cmd = payload[0];

	if (cmd == 0x01)
	{
		// 开始计算 实时计算模式下
		emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
		//isCalculating_ = true;
		//std::cout << "[状态] 开始计算" << std::endl;
	}
	else if (cmd == 0x02)
	{
		// 遗留问题： 先停止发送还是先停止计算？？？？
		// 停止计算 实时计算模式下
		emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
		//isCalculating_ = false;
		//std::cout << "[状态] 停止计算" << std::endl;
	}
	else {
		sendErrorAndClose(0x01, "非法控制命令");
	}
}

void SocketComm::processDataStreamControl(const std::vector<uint8_t>& payload)
{
	if (payload.empty()) return;
	uint8_t ctrl = payload[0];
	if (ctrl == 0x01)
	{
		m_socketInfo.bInquireSendFlag = true;
	}
	m_socketInfo.output.iTransferType = 0;
	emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATASTREAMING);
}

void SocketComm::processPollingRequest()
{
	m_socketInfo.output.iTransferType = 1;
	m_socketInfo.bInquireSendFlag = true;
	emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATAPOLLING);
	//std::cout << "[轮询] 客户端请求一次数据响应" << std::endl;

	// 伪造一个数据帧：外部力值+温度
	/*float f = 123.4f, t = 36.5f;
	auto frame = lhgkProto_.buildExternalDataFrame(f, t);
	sendData(frame);*/
}


void SocketComm::sendErrorAndClose(uint8_t errCode, const std::string& msg)
{
	auto err = lhgkProto_.buildErrorFrame(errCode, msg);
	sendData(err);
}

void SocketComm::ParsingProtocolI(const QByteArray& message)
{
	// 接收到json数据
	QJsonDocument msg = QJsonDocument::fromJson(message);

	if (!msg.isObject())
	{
		if (!message.isEmpty())
			emitAssistUnparsedRx(message);
		return;
	}

	if (msg.isObject())
	{
		QJsonObject obj = msg.object();
		QStringList keys = obj.keys();
		for (int i = 0; i < keys.size(); i++) {
			QString key = keys.at(i);
			QJsonValue value = obj.value(key);
			if (value.isDouble())
			{
				switch (value.toInt())
				{
					// 同步开始
				case 1:
				{
					// 相机未打开
					if (!m_socketInfo.bAllOpenCamera)
					{
						QJsonObject json;
						json.insert("code", 70);
						json.insert("message", "The camera is not turned on.");

						QJsonArray xarray, yarray;
						xarray.append(0);
						yarray.append(0);

						json.insert("ylength", yarray);
						json.insert("xlength", xarray);
						json.insert("tn", 1);
						QJsonDocument doc(json);
						QString jsonString = doc.toJson(QJsonDocument::Compact);
						m_controlBox.sendData(jsonString);
						notifyWireTxAck(70, QStringLiteral("The camera is not turned on."), 1, jsonString);
						return;
					}

					// 还没有开始计算
					if (!m_socketInfo.bOnlineCollect)
					{
						if (m_socketInfo.output.iChannelSize <= 0)
						{
							QJsonObject json;
							json.insert("code", 100);
							json.insert("message", "Gauge length does not exists.");

							QJsonArray xarray, yarray;
							xarray.append(0);
							yarray.append(0);

							json.insert("ylength", yarray);
							json.insert("xlength", xarray);
							json.insert("tn", 1);
							QJsonDocument doc(json);
							QString jsonString = doc.toJson(QJsonDocument::Compact);
							m_controlBox.sendData(jsonString);
							notifyWireTxAck(100, QStringLiteral("Gauge length does not exists."), 1, jsonString);
						}

						if (m_socketInfo.input.iTriggerCtrl)
						{
							// 开始计算 实时计算模式下
							emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
						}
						// 通知外部发发送一次标距长度数据
						//emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATAPOLLING);
						QJsonObject json;
						json.insert("code", 0);
						json.insert("message", QJsonValue::Null);
						QJsonArray xarray, yarray;
						int ix = 0, iy = 0;
						for (int i = 0; i < m_socketInfo.output.iChannelSize; i++)
						{

						}
						if (ix == 0)
						{
							xarray.append(0);
						}
						if (iy == 0)
						{
							yarray.append(0);
						}
						json.insert("ylength", yarray);
						json.insert("xlength", xarray);
						json.insert("tn", 1);
						QJsonDocument doc(json);
						QString jsonString = doc.toJson(QJsonDocument::Compact);
						m_controlBox.sendData(jsonString);
						notifyWireTxAck(0, QString(), 1, jsonString);
					}
					// 已经在计算了
					else if (m_socketInfo.bOnlineCollect)
					{
						// 通知外部发发送一次已经在计算数据
						//emit emitEventMsg(CommCtrlCmd::ControlSoftStat	`us, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
						QJsonObject json;
						json.insert("code", 170);
						json.insert("message", "Computation is already running.");
						QJsonArray xarray, yarray;
						int ix = 0, iy = 0;
						for (int i = 0; i < m_socketInfo.output.iChannelSize; i++)
						{

						}
						if (ix == 0)
						{
							xarray.append(0);
						}
						if (iy == 0)
						{
							yarray.append(0);
						}
						json.insert("ylength", yarray);
						json.insert("xlength", xarray);
						json.insert("tn", 1);
						QJsonDocument doc(json);
						QString jsonString = doc.toJson(QJsonDocument::Compact);
						m_controlBox.sendData(jsonString);
						notifyWireTxAck(170, QStringLiteral("Computation is already running."), 1, jsonString);
					}
				}
				break;
				// 结束分析
				case 3:
				{
					// 还没有结束发送数据
					if (m_socketInfo.bOnlineCollect)
					{
						// 遗留问题： 先停止发送还是先停止计算？？？？
						// 开始计算 实时计算模式下
						emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
						SendReturnMsg(0, "null", 3);
					}
					else if (m_socketInfo.bOnlineCollect == false)
					{
						SendReturnMsg(170, "Computation is not running.", 3);
					}
				}
				break;
				default: break;
				}
			}
			//QString str = value.toString();
			// 数据通讯
			if (value.isString())
			{
				// 数据轮询
				if (value.toString() == "datapolling")
				{
					if (!m_socketInfo.bOnlineCollect)
					{
						emitAssistUnparsedRx(message);
						return;
					}
					m_socketInfo.output.iTransferType = 1;
					m_socketInfo.bInquireSendFlag = true;
					// 通知外部发送数据
					//emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATAPOLLING);
				}
				// 数据流
				else if (value.toString() == "dataflow")
				{
					if (!m_socketInfo.bOnlineCollect)
					{
						emitAssistUnparsedRx(message);
						return;
					}
					// 发送数据
					m_socketInfo.output.iTransferType = 0;
					m_socketInfo.bInquireSendFlag = true;
					// 通知外部发送数据
					//emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_DATASTREAMING);
				}
			}
		}
	}
}

void SocketComm::SendReturnMsg(const int& code, const QString& msg, const int& tn)
{
	QJsonObject json;
	json.insert("code", code);
	if (msg == "null")
	{
		json.insert("message", QJsonValue::Null);
	}
	else {
		json.insert("message", msg);
	}

	json.insert("tn", tn);
	QJsonDocument doc(json);
	QString jsonString = doc.toJson(QJsonDocument::Compact);
	m_controlBox.sendData(jsonString);
	notifyWireTxAck(code, (msg == QStringLiteral("null")) ? QString() : msg, tn, jsonString);
}

// 助手观测：上报协议线发 ACK
void SocketComm::notifyWireTxAck(int code, const QString& message, int tn, const QString& wireJson)
{
	if (!m_bAssistObserve)
		return;
	QVariantMap extra;
	extra.insert(QStringLiteral("ackCode"), code);
	extra.insert(QStringLiteral("ackMessage"), message);
	extra.insert(QStringLiteral("tn"), tn);
	extra.insert(QStringLiteral("wireTx"), wireJson);
	extra.insert(QStringLiteral("wireDirection"), QStringLiteral("tx"));
	emit emitEventMsgAndData(CommCtrlCmd::ControlSoftStatus, CommViewID::Socket_ID, W_CUSTOM_COMM_PROTO_ACK, extra);
}

// 助手观测：未识别载荷上报
void SocketComm::emitAssistUnparsedRx(const QByteArray& raw)
{
	if (!m_bAssistObserve || raw.isEmpty())
		return;
	emit emitUnparsedRx(raw);
}

void SocketComm::ParsingProtocolII(const QByteArray& data)
{
	bool matched = false;
	if (m_socketInfo.input.iTriggerCtrl)
	{
		QString msd = data.constData();
		if (m_socketInfo.bOpenCMD[0] && msd == QString::fromLocal8Bit(m_socketInfo.cCMD[0]))	// 开始
		{
			// 开始计算 实时计算模式下
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
			matched = true;
		}
		else if (m_socketInfo.bOpenCMD[1] && msd == QString::fromLocal8Bit(m_socketInfo.cCMD[1]))	// 结束
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
			matched = true;
		}
		else if (m_socketInfo.bOpenCMD[2] && msd == QString::fromLocal8Bit(m_socketInfo.cCMD[2]))	// 退出程序
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_EXITPROG);
			matched = true;
		}
	}
	if (!matched && !data.isEmpty())
		emitAssistUnparsedRx(data);
}

void SocketComm::ParsingProtocolZhongji(const QByteArray& message)
{
	if (message.size() == sizeof(SocketProtol::ControlMessage))
	{
		SocketProtol::ControlMessage sockdata;
		memcpy(&sockdata, message.constData(), sizeof(SocketProtol::ControlMessage));
		// 控制信息
		if (sockdata.msgType == SocketProtol::MessageType::CONTROL_MESSAGE)
		{
			AnalysisZhongjiControl(sockdata);
		}
		if (sockdata.msgType == SocketProtol::MessageType::RETURN_MESSAGE)
		{
			switch (sockdata.command)
			{
			case SocketProtol::ReturnCommand::START_RUNNING:	// 通讯处于连接状态
			{
				SocketProtol::ControlMessage sockdata;
				sockdata.msgType = SocketProtol::MessageType::RETURN_MESSAGE;
				sockdata.command = SocketProtol::ReturnCommand::START_RUNNING;

				QByteArray dataArray;
				dataArray.resize(sizeof(SocketProtol::ControlMessage));
				memcpy(dataArray.data(), &sockdata, sizeof(SocketProtol::ControlMessage));
				SendData(dataArray);
				break;
			}
			default:		break;
			}
		}
		return;
	}
	if (message.size() == sizeof(SocketProtol::ForceData))
	{
		SocketProtol::ForceData sockdata;
		memcpy(&sockdata, message.constData(), sizeof(SocketProtol::ForceData));
		// 力值信息
		if (sockdata.msgType == SocketProtol::MessageType::DATA_MESSAGE)
		{
			double* data = new double[4];
			memcpy(data, sockdata.forceValues, (sizeof(double) * 4));
			emitNewData(data, 4, 2);

			delete[] data;
			data = nullptr;
		}
		return;
	}
	// 长度与中机结构均不匹配：上报未解析线帧，避免助手侧以为「对端未发」
	if (!message.isEmpty())
		emitAssistUnparsedRx(message);
}

void SocketComm::AnalysisZhongjiControl(const SocketProtol::ControlMessage& message)
{
	switch (message.command)
	{
	case SocketProtol::ControlCommand::START_CALC:	// 同步开始采集命令
	{

		SocketProtol::ControlMessage sockdata;
		sockdata.msgType = SocketProtol::MessageType::RETURN_MESSAGE;
		sockdata.command = SocketProtol::ReturnCommand::NORMAL_RUNNING;
		if (m_socketInfo.bOnlineCollect)
		{
			sockdata.command = SocketProtol::ReturnCommand::ALREADY_RUNNING;
		}
		else if (m_socketInfo.bAllOpenCamera)
		{
			sockdata.command = SocketProtol::ReturnCommand::CAMERA_NOT_OPEN;
		}
		QByteArray dataArray;
		dataArray.resize(sizeof(SocketProtol::ControlMessage));
		memcpy(dataArray.data(), &sockdata, sizeof(SocketProtol::ControlMessage));
		SendData(dataArray);
		if (!m_socketInfo.bOnlineCollect)
		{
			if (m_socketInfo.input.iTriggerCtrl)
			{
				// 开始计算 实时计算模式下
				emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
			}
		}
		m_socketInfo.bOnlineCollect = true;
		break;
	}
	case SocketProtol::ControlCommand::STOP_CALC:	// 同步结束
	{
		SocketProtol::ControlMessage sockdata;
		sockdata.msgType = SocketProtol::MessageType::RETURN_MESSAGE;
		sockdata.command = SocketProtol::ReturnCommand::NORMAL_STOP;
		if (!m_socketInfo.bOnlineCollect)
		{
			sockdata.command = SocketProtol::ReturnCommand::ALREADY_STOPPED;
		}
		QByteArray dataArray;
		dataArray.resize(sizeof(SocketProtol::ControlMessage));
		memcpy(dataArray.data(), &sockdata, sizeof(SocketProtol::ControlMessage));

		if (m_socketInfo.bOnlineCollect)
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
		}
		m_socketInfo.bOnlineCollect = false;
		SendData(dataArray);
		break;
	}
	case SocketProtol::ControlCommand::DATA_COLL_POLLING:	// 数据流
	{
		m_socketInfo.output.iTransferType = 0;
		m_socketInfo.bInquireSendFlag = true;
		break;
	}
	case SocketProtol::ControlCommand::DATA_COLL_FLOW:	//  数据轮询
	{
		m_socketInfo.output.iTransferType = 1;
		m_socketInfo.bInquireSendFlag = true;
		break;
	}
	case SocketProtol::ControlCommand::ACQ_STATE:	// 获取通讯状态
	{
		SocketProtol::ControlMessage sockdata;
		sockdata.msgType = SocketProtol::MessageType::RETURN_MESSAGE;
		sockdata.command = SocketProtol::ReturnCommand::START_RUNNING;

		QByteArray dataArray;
		dataArray.resize(sizeof(SocketProtol::ControlMessage));
		memcpy(dataArray.data(), &sockdata, sizeof(SocketProtol::ControlMessage));
		SendData(dataArray);
	}
	default:		break;
	}
}

void SocketComm::ParsingProtocolSansi(const QByteArray& data)
{
	if (data.size() == static_cast<int>(sizeof(Data)))
	{
		Data recv;
		memcpy(&recv, data.constData(), sizeof(Data));

		double* resdata(new double);
		*resdata = recv.dForce;

		emit emitNewData(resdata, 1, 0);
		delete resdata;
		resdata = nullptr;
		return;
	}
	if (!data.isEmpty())
		emitAssistUnparsedRx(data);
}

bool SocketComm::parseFuJianWeishengData(const QString& message, TestData& data)
{
	// Check message format
	if (message.isEmpty()) {
		return false;
	}

	// Check first character
	QChar firstChar = message[0];
	if (firstChar != 'T' && firstChar != 'S') {
		return false;
	}

	data.status = firstChar.toLatin1();

	// Parse only load (L)
	// Format: TL123.45E67.89D12.34M56.78
	int pos = 1;
	int len = message.length();

	// Parse load (L)
	if (pos < len && message[pos] == 'L') {
		pos++;
		int endPos = message.indexOf('E', pos);
		if (endPos > pos) {
			QString value = message.mid(pos, endPos - pos);
			data.load = value.toFloat();
		}
	}

	return true;
}

void SocketComm::paeseNaBaiChuanData(const QByteArray& message)
{
	// 去掉首尾空白/CRLF，避免文本与「等价 HEX 字节」因尾部分隔符不一致而静默失败
	const QByteArray body = message.trimmed();
	if (body.isEmpty())
		return;

	QJsonDocument jsonDoc = QJsonDocument::fromJson(body);
	bool handled = false;

	if (jsonDoc.isObject())
	{
		QJsonObject jsonObj = jsonDoc.object();

		if (jsonObj.contains("name"))
		{
			QString name = jsonObj["name"].toString();

			if (name == "alphaStartTest")
			{
				handled = true;
				// 已在采集：正式语义直接返回；助手观测时仅上报未解析
				if (m_socketInfo.bOnlineCollect)
				{
					emitAssistUnparsedRx(body);
					return;
				}
				m_socketInfo.bOnlineCollect = true;
				m_socketInfo.output.iTransferType = 0;
				m_socketInfo.bInquireSendFlag = true;
				if (m_socketInfo.input.iTriggerCtrl)
				{
					emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
				}
			}
			else if (name == "alphaStopTest")
			{
				handled = true;
				// 仅采集中清标志并上报 STOP
				if (m_socketInfo.bOnlineCollect)
				{
					m_socketInfo.bOnlineCollect = false;
					emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
				}
				else
				{
					emitAssistUnparsedRx(body);
				}
			}
			else if (name == "alphaSetLD")
			{
				if (!m_socketInfo.bOnlineCollect)
				{
					bool hasL = jsonObj.contains("L") && !jsonObj["L"].isNull();
					bool hasD = jsonObj.contains("D") && !jsonObj["D"].isNull();

					QVariantMap extra;
					extra["M"] = 1;
					if (hasL)
					{
						extra["L"] = jsonObj["L"].toDouble();
					}

					if (hasD)
					{
						extra["D"] = jsonObj["D"].toDouble();
					}

					if (hasL || hasD)
					{
						handled = true;
						emit emitEventMsgAndData(CommCtrlCmd::ControlSoftStatus, CommViewID::Socket_ID, W_CUSTOM_COMM_RESET_LVE_LENGTH, extra);
					}
				}
			}
			else if (name == "alphaSetGauge")
			{
				if (!m_socketInfo.bOnlineCollect)
				{
					if (jsonObj.contains("length") && jsonObj.contains("Gnum") && jsonObj.contains("Gexnum"))
					{
						handled = true;
						QVariantMap extra;
						extra["M"] = 2;
						extra["length"] = jsonObj["length"].toDouble();
						extra["Gnum"] = jsonObj["Gnum"].toDouble();
						extra["Gexnum"] = jsonObj["Gexnum"].toDouble();

						emit emitEventMsgAndData(CommCtrlCmd::ControlSoftStatus, CommViewID::Socket_ID, W_CUSTOM_COMM_RESET_LVE_LENGTH, extra);
					}
				}
			}
		}
	}

	// 未识别载荷必须可观测，禁止静默丢弃（含非 JSON / 缺字段）
	if (!handled && !body.isEmpty())
		emitAssistUnparsedRx(body);
}

void SocketComm::paeseNaBaiChuanDataCalcLine(const QByteArray& message)
{
	// 文本 calcStart 与 HEX 63 61…74 线帧等价；trimmed 消除工具附加的 \r\n
	const QByteArray body = message.trimmed();
	if (body.isEmpty())
		return;

	if (body == "calcStart")
	{
		// 已在采集：正式语义直接返回；助手观测时仅上报未解析
		if (m_socketInfo.bOnlineCollect)
		{
			emitAssistUnparsedRx(body);
			return;
		}
		m_socketInfo.bOnlineCollect = true;
		m_socketInfo.output.iTransferType = 0;
		m_socketInfo.bInquireSendFlag = true;
		if (m_socketInfo.input.iTriggerCtrl)
		{
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STARTCALC);
		}
	}
	else if (body == "calcEnd")
	{
		// 仅采集中清标志并上报 STOP
		if (m_socketInfo.bOnlineCollect)
		{
			m_socketInfo.bOnlineCollect = false;
			emit emitEventMsg(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_STOPCALC);
		}
		else
		{
			emitAssistUnparsedRx(body);
		}
	}
	else
	{
		bool handled = false;
		if (!m_socketInfo.bOnlineCollect)
		{
			const int index = body.indexOf(':');
			if (-1 != index)
			{
				QVariantMap extra;
				extra["X"] = 0;
				extra["Y"] = 0;
				extra["TYPE"] = 0;

				const QByteArray cmd = body.left(index);
				const QByteArray values = body.mid(index + 1);
				if (cmd == "InitYLen")
				{
					extra["TYPE"] = 1;
					extra["Y"] = QString::fromLatin1(values).toDouble();
					handled = true;
				}
				else if (cmd == "InitXLen")
				{
					extra["TYPE"] = 2;
					extra["X"] = QString::fromLatin1(values).toDouble();
					handled = true;
				}
				else if (cmd == "InitXYLen")
				{
					const QStringList xyValues = QString::fromLatin1(values).split(',');
					if (xyValues.size() == 2)
					{
						extra["TYPE"] = 3;
						extra["X"] = xyValues[0].toDouble();
						extra["Y"] = xyValues[1].toDouble();
						handled = true;
					}
				}

				if (handled)
					emit emitEventMsgAndData(CommCtrlCmd::ControlSoftStatus, CommViewID::UNKNOWN_VIEW_ID, W_CUSTOM_COMM_CALC_LVE_LENGTH, extra);
			}
		}

		if (!handled)
			emitAssistUnparsedRx(body);
	}
}
