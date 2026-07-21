#include "tcpserver.h"
#include <QNetworkInterface>

TcpServer::TcpServer(QObject* parent)
{
	/****************
	 * QHostAddress
	 *  Null            空地址
	 *  LocalHost       IPv4 127.0.0.1 本地回环地址
	 *  LocalHostIPv6   IPv6           本地回环地址
	 *  Broadcast       广播地址 255.255.255.255
	 *  Any             IPv4 0.0.0.0   任意地址
	 *  AnyIPv6         IPv6           任意地址
	******************/
	m_pServer = new QTcpServer;
	m_bConnected = false;
	connect(m_pServer, &QTcpServer::newConnection,
		[=]()
		{
			//取出建立好连接的套接字
			QTcpSocket* newSocket = m_pServer->nextPendingConnection();
			if (!newSocket) {
				return;
			}

			// 添加到客户端列表
			m_clientSockets.append(newSocket);
			m_bConnected = true;

			QString IP = newSocket->peerAddress().toString();
			int port = newSocket->peerPort();
			emit emitNewConn(IP, port);

			// 本 socket 接受到的信息
			connect(newSocket, &QTcpSocket::readyRead, [this, newSocket]() {
				if (m_clientSockets.contains(newSocket))
				{
					QByteArray array = newSocket->readAll();
					emit emitNewMessage(array);
				}
			});

			connect(newSocket, &QTcpSocket::disconnected, [this, newSocket]() {
				if (m_clientSockets.contains(newSocket))
				{
					qintptr q = newSocket->socketDescriptor();
					emit emitDisConnected(q);

					// 从列表中移除
					m_clientSockets.removeOne(newSocket);
					if (m_clientSockets.isEmpty()) {
						m_bConnected = false;
					}

					// 使用 deleteLater 延迟删除，避免在信号处理中直接删除
					newSocket->deleteLater();
				}
			});
		});
}

bool TcpServer::initConnection(const QString& address, int port)
{
	if (port <= 0 || port > 65535)
		return false;
	if (m_pServer->isListening())
		m_pServer->close();

	// address 为空或 0.0.0.0 时监听任意 IPv4；否则绑定指定地址
	QHostAddress bindAddr = QHostAddress::AnyIPv4;
	const QString trimmed = address.trimmed();
	if (!trimmed.isEmpty() && trimmed != QStringLiteral("0.0.0.0")) {
		const QHostAddress parsed(trimmed);
		if (parsed.isNull())
			return false;
		bindAddr = parsed;
	}

	if (!m_pServer->listen(bindAddr, static_cast<quint16>(port)))
		return false;
	return m_pServer->isListening();
}

bool TcpServer::closeConnection()
{
	m_bConnected = false;

	// 先断开所有客户端连接
	for (QTcpSocket* socket : m_clientSockets) {
		if (socket) {
			socket->disconnectFromHost();
			socket->waitForDisconnected();
			socket->deleteLater();
		}
	}
	m_clientSockets.clear();

	// 关闭服务器
	if (m_pServer->isListening()) {
		m_pServer->close();
	}

	return true;
}

bool TcpServer::sendData(const QString& data)
{
	if (!m_bConnected || m_clientSockets.isEmpty()) {
		return false;
	}

	// 向所有连接的客户端发送数据
	bool allSuccess = true;
	for (QTcpSocket* socket : m_clientSockets) {
		if (socket && socket->state() == QAbstractSocket::ConnectedState) {
			qint64 written = socket->write(data.toLatin1());
			socket->flush();
			if (written == -1) {
				allSuccess = false;
			}
		}
	}
	return allSuccess;
}

bool TcpServer::sendData(const char* data, const qint64 length)
{
	if (!m_bConnected || m_clientSockets.isEmpty()) {
		return false;
	}

	// 向所有连接的客户端发送数据
	bool allSuccess = true;
	for (QTcpSocket* socket : m_clientSockets) {
		if (socket && socket->state() == QAbstractSocket::ConnectedState) {
			qint64 written = socket->write(data, length);
			socket->flush();
			if (written == -1) {
				allSuccess = false;
			}
		}
	}
	return allSuccess;
}

void TcpServer::sendData(const char* data)
{
	if (!m_bConnected || m_clientSockets.isEmpty()) {
		return;
	}

	// 向所有连接的客户端发送数据
	for (QTcpSocket* socket : m_clientSockets) {
		if (socket && socket->state() == QAbstractSocket::ConnectedState) {
			socket->write(data);
			socket->flush();
		}
	}
}

QHostAddress TcpServer::get_Host_IP_Address()
{
	// 获取所有网络接口列表
	QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
	// 遍历每个网络接口
	foreach(QNetworkInterface interface, list)
	{
		// 遍历每个网络接口的所有IP地址
		foreach(QNetworkAddressEntry entry, interface.addressEntries())
		{
			// 如果是IPv4地址，且不是本地回环地址
			if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && entry.ip() != QHostAddress(QHostAddress::LocalHost))
			{
				return  entry.ip();
			}
		}
	}
	return QHostAddress::LocalHost;
}

QList<QString> TcpServer::get_All_Host_IP_Address() const
{
	// 获取所有网络接口列表
	QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
	QList<QString> ipList;
	// 遍历每个网络接口
	foreach(QNetworkInterface interface, list)
	{
		// 遍历每个网络接口的所有IP地址
		foreach(QNetworkAddressEntry entry, interface.addressEntries())
		{
			// 如果是IPv4地址
			if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
			{
				ipList.append(entry.ip().toString());
			}
		}
	}
	return ipList;
}
