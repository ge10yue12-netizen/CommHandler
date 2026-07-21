#include "NetworkBox.h"
#include "tcpserver.h"
#include "tcpsocket.h"
#include "udpsocket.h"

NetworkBox::NetworkBox(QObject* parent) :
	QObject(parent),
	m_pCurNetModule(nullptr)
{
	m_pNetModules[iTcpServer] = new TcpServer(this);
	m_pNetModules[iTcpClient] = new TcpSocket(this);
	m_pNetModules[iUdp] = new udpsocket(this);

	// 连接各个网络模块的信号
	for (auto type : { iTcpServer, iTcpClient, iUdp }) {
		connect(m_pNetModules[type], &NetworkBase::emitNewConn,
			this, &NetworkBox::handleNewConn);
		connect(m_pNetModules[type], &NetworkBase::emitNewMessage,
			this, &NetworkBox::handleNewMessage);
		connect(m_pNetModules[type], &NetworkBase::emitDisConnected,
			this, &NetworkBox::handleDisConn);
	}
}
NetworkBox::~NetworkBox()
{
	//for (auto module : m_pNetModules) {
	//	delete module;
	//}
}

void NetworkBox::SetNetType(NetType type)
{
	m_pCurNetModule = m_pNetModules.value(type, nullptr);
}

void NetworkBox::SetDestIpAndPort(QHostAddress ip, quint16 port)
{
	// 确保当前通讯类型是udp
	udpsocket* pudpsocket = dynamic_cast<udpsocket*>(m_pCurNetModule);
	if (pudpsocket) {
		pudpsocket->SetDestIpAndPort(ip, port);
	}
	else {

	}
}

QStringList NetworkBox::GetHostAllIPAddress()
{
	TcpServer* tcpServer = qobject_cast<TcpServer*>(m_pNetModules[iTcpServer]);
	if (tcpServer) {
		return tcpServer->get_All_Host_IP_Address();
	}
	return QStringList();
}

bool NetworkBox::initConnection(const QString& address, int port)
{
	if (!m_pCurNetModule) {
		return false;
	}
	return m_pCurNetModule->initConnection(address, port);
}

bool NetworkBox::closeConnection()
{
	if (!m_pCurNetModule)
	{
		return true;
	}
	return m_pCurNetModule->closeConnection();

}

bool NetworkBox::sendData(const QString& data)
{
	if (!m_pCurNetModule) {
		return false;
	}
	return  m_pCurNetModule->sendData(data);
}

void NetworkBox::sendData(const char* data, const qint64 length)
{
	if (!m_pCurNetModule) {
		return;
	}
	m_pCurNetModule->sendData(data, length);
}

void NetworkBox::sendData(const char* data)
{
	if (!m_pCurNetModule) {
		return;
	}
	m_pCurNetModule->sendData(data);
}

void NetworkBox::handleNewConn(QString ip, int port)
{
	emit emitNewConn(ip, port);
}

void NetworkBox::handleNewMessage(QByteArray msg)
{
	emit emitNewMessage(msg);
}

void NetworkBox::handleDisConn(qintptr port)
{
	emit emitDisConnected(port);
}
