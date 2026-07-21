#include "CommHandler.h"
#include "SocketComm.h"
#include "SerialPortComm.h"
#include "VoltageComm.h"
#include "PulseComm.h"

CommHandler::CommHandler(QObject* parent)
	: QObject(parent)
	, m_curCommModule(nullptr)
{
	m_commModules[NETWORK] = new SocketComm();
	m_commModules[SERIAL] = new SerialPortComm();
	m_commModules[VOLTAGE] = new VoltageComm();
	m_commModules[PULSE] = new PulseComm();
	m_curCommModule = m_commModules.value(NETWORK, nullptr);
	ConnectSlots();
}
CommHandler::~CommHandler()
{
	for (auto module : m_commModules)
	{

		delete module;
	}
}

void CommHandler::SetToDefault()
{
	m_curCommModule->SetToDefault();
}

void CommHandler::ConnectSlots()
{
	SocketComm* sockComm = dynamic_cast<SocketComm*>(m_commModules[NETWORK]);
	connect(sockComm, &SocketComm::emitNewData, this, &CommHandler::onNewDataReceived);
	connect(sockComm, &SocketComm::emitNewConn, this, &CommHandler::onNewConnection);
	connect(sockComm, &SocketComm::emitClientDisConn, this, &CommHandler::onClientDisconnected);
	connect(sockComm, &SocketComm::emitEventMsg, this, &CommHandler::onEventMessage);
	connect(sockComm, &SocketComm::emitEventMsgAndData, this, &CommHandler::onEventMsgAndData);
	connect(sockComm, &SocketComm::emitUnparsedRx, this, &CommHandler::onUnparsedRx);


	SerialPortComm* serialComm = dynamic_cast<SerialPortComm*>(m_commModules[SERIAL]);
	connect(serialComm, &SerialPortComm::emitNewData, this, &CommHandler::onNewDataReceived);
	//connect(serialComm, &SerialPortComm::emitNewConn, this, &CommHandler::onNewConnection);
	//connect(serialComm, &SerialPortComm::emitClientDisConn, this, &CommHandler::onClientDisconnected);
	connect(serialComm, &SerialPortComm::emitEventMsg, this, &CommHandler::onEventMessage);
	connect(serialComm, &SerialPortComm::emitEventMsgAndData, this, &CommHandler::onEventMsgAndData);
	connect(serialComm, &SerialPortComm::emitUnparsedRx, this, &CommHandler::onUnparsedRx);

	VoltageComm* voltaComm = dynamic_cast<VoltageComm*>(m_commModules[VOLTAGE]);
	connect(voltaComm, &VoltageComm::emitNewData, this, &CommHandler::onNewDataReceived);
	//connect(voltaComm, &VoltageComm::emitNewConn, this, &CommHandler::onNewConnection);
	//connect(sockComm, &VoltageComm::emitClientDisConn, this, &CommHandler::onClientDisconnected);
	//connect(sockComm, &VoltageComm::emitEventMsg, this, &CommHandler::onEventMessage);

	PulseComm* pulseComm = dynamic_cast<PulseComm*>(m_commModules[PULSE]);
	//connect(pulseComm, &PulseComm::emitNewData, this, &CommHandler::onNewDataReceived);
	//connect(pulseComm, &PulseComm::emitNewConn, this, &CommHandler::onNewConnection);
	//connect(pulseComm, &PulseComm::emitClientDisConn, this, &CommHandler::onClientDisconnected);
	connect(pulseComm, &PulseComm::emitEventMsg, this, &CommHandler::onEventMessage);
}

void CommHandler::SetCommType(CommType type)
{
	m_curCommModule = m_commModules.value(type, nullptr);
}

void CommHandler::setForceUnit(int unit)
{
	//m_commModules[NETWORK]->setParameter("iUnit", unit);
	//m_commModules[SERIAL]->setParameter("iUnit", unit);
	//m_commModules[VOLTAGE]->setParameter("iForceUnit", unit);
	//m_commModules[PULSE]->setParameter("iUnit", unit);
}

bool CommHandler::SetSysParInfo(void* ptr)
{
	return m_curCommModule->SetSysParInfo(ptr);
}

bool CommHandler::GetSysParInfo(void* ptr) const
{
	return m_curCommModule->GetSysParInfo(ptr);
}

void CommHandler::setParameter(const QString& key, const Any& value)
{
	m_curCommModule->setParameter(key, value);
}

Any CommHandler::getParameter(const QString& key) const
{
	return m_curCommModule->getParameter(key);
}

bool CommHandler::Connect(int type)
{
	return m_curCommModule->Connect(type);
}

bool CommHandler::Disconnect(int type)
{
	if (-1 == type)
	{
		// 关闭所有通讯
		CommType commTypes[] = { NETWORK, SERIAL, VOLTAGE, PULSE };
		for (CommType type : commTypes)
		{
			CommBase* commModule = m_commModules.value(type, nullptr);
			try
			{
				return commModule->Disconnect();
			}
			catch (const std::invalid_argument&) {
				// 捕获异常并尝试下一种通信方式
				continue;
			}
		}
	}
	else {
		return m_curCommModule->Disconnect(type);
	}
	return false;
}

//void CommHandler::SendData(void* ptr, size_t size)
//{
//	m_curCommModule->SendData(ptr, size);
//}
//
//void CommHandler::SendData(const QString& data)
//{
//	m_curCommModule->SendData(data);
//}

void CommHandler::SendData(const std::vector<double>& vData, const CommType& type)
{
	m_commModules[type]->SendData(vData);
}

void CommHandler::SendData(const QString& sMsg, const CommType& type)
{
	m_commModules[type]->SendData(sMsg);
}

bool CommHandler::SaveParInfo(const QString& filename)
{
	return m_curCommModule->SaveParInfo(filename);
}

bool CommHandler::LoadParInfo(const QString& filename)
{
	return m_curCommModule->LoadParInfo(filename);
}

bool CommHandler::saveCmdToFile(const QString& filename)
{
	return m_curCommModule->saveCmdToFile(filename.toLocal8Bit().constData());
}

bool CommHandler::loadCmdFromFile(const QString& filename)
{
	return m_curCommModule->loadCmdFromFile(filename.toLocal8Bit().constData());
}

bool CommHandler::SendRequireMsg()
{
	/*SerialPortComm* serialport = dynamic_cast<SerialPortComm*>(m_commModules[SERIAL]);
	if (serialport) {
		return serialport->SendRequireMsg();
	}*/
	return false;
}

void CommHandler::SendData2ComPort(float fStrain1, float fStrain2, float fMovement1, float fMovement2, float fReserve, float fTime)
{
	SerialPortComm* serialport = dynamic_cast<SerialPortComm*>(m_commModules[SERIAL]);
	if (serialport)
	{
		serialport->SendData2ComPort(fStrain1, fStrain2, fMovement1, fMovement2, fReserve, fTime);
	}
}

void CommHandler::DataPackAndWrite(const int& value)
{
	SerialPortComm* serialport = dynamic_cast<SerialPortComm*>(m_commModules[SERIAL]);
	if (serialport)
	{
		serialport->DataPackAndWrite(value);
	}
}

void CommHandler::WaitForBytesWritten(const int& ms)
{
	SerialPortComm* serialport = dynamic_cast<SerialPortComm*>(m_commModules[SERIAL]);
	if (serialport)
	{
		serialport->WaitForBytesWritten(ms);
	}
}

bool CommHandler::HardTrigger(double value, double dutyCycle)
{
	VoltageComm* voltage = dynamic_cast<VoltageComm*>(m_commModules[VOLTAGE]);
	if (voltage) {
		return voltage->HardTrigger(value, dutyCycle);
	}
	return false;
}

bool CommHandler::ReleaseHardTrigger()
{
	VoltageComm* voltage = dynamic_cast<VoltageComm*>(m_commModules[VOLTAGE]);
	if (voltage) {
		return voltage->ReleaseHardTrigger();
	}
	return false;
}

bool CommHandler::ReadCIFreq()
{
	VoltageComm* voltage = dynamic_cast<VoltageComm*>(m_commModules[VOLTAGE]);
	if (voltage) {
		return voltage->ReadCIFreq();
	}
	return false;
}

bool CommHandler::ReadADData(double* pdData, double* pGainData, double* truthData, bool bGetVolt)
{
	VoltageComm* voltage = dynamic_cast<VoltageComm*>(m_commModules[VOLTAGE]);
	if (voltage) {
		return voltage->ReadADData(pdData, pGainData, truthData, bGetVolt);
	}
	return false;
}

void CommHandler::SendCaliData()
{
	// 确保当前通讯类型是脉冲通讯
	PulseComm* pulseComm = dynamic_cast<PulseComm*>(m_commModules[PULSE]);
	if (pulseComm) {
		pulseComm->SendCaliData();
	}
}

void CommHandler::onNewDataReceived(void* data, int size, int type)
{
	emit emitNewData(data, size, type);
}
void CommHandler::onNewConnection(int iType, QString ip, int port)
{
	emit emitNewConn(iType, ip, port);
}
void CommHandler::onClientDisconnected()
{
	emit emitClientDisConn();
}
void CommHandler::onEventMessage(int ctrlCmd, int ViewID, int msg)
{
	emit emitEventMsg(ctrlCmd, ViewID, msg);
}

void CommHandler::onEventMsgAndData(const int& ctrlCmd, const int& ViewID, const int& msg, const QVariantMap& extra)
{
	emit emitEventMsgAndData(ctrlCmd, ViewID, msg, extra);
}

void CommHandler::onUnparsedRx(const QByteArray& raw)
{
	emit emitUnparsedRx(raw);
}
