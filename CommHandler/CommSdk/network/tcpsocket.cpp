#include "tcpsocket.h"

TcpSocket::TcpSocket(QObject* parent)
	: NetworkBase(parent)
{
	// 挂到本对象，随线程/生命周期销毁，避免野指针
	m_pSocket = new QTcpSocket(this);
	connect(m_pSocket, &QTcpSocket::readyRead, this, &TcpSocket::onRecvData);
	connect(m_pSocket, &QTcpSocket::disconnected, this, [this]() {
		emit emitDisConnected(m_pSocket ? m_pSocket->socketDescriptor() : 0);
	});
}

TcpSocket::~TcpSocket()
{
	// m_pSocket 由 QObject 树释放
	m_pSocket = nullptr;
}

// 仅当 TCP 真正进入 ConnectedState 才返回 true（拒绝/超时/半开均失败）
bool TcpSocket::initConnection(const QString& address, int port)
{
	if (!m_pSocket)
		return false;
	if (address.trimmed().isEmpty() || port <= 0 || port > 65535)
		return false;

	m_pSocket->abort();
	m_pSocket->connectToHost(address.trimmed(), static_cast<quint16>(port));
	// 与 Native TcpClientTransport 对齐：有界等待 5s
	if (!m_pSocket->waitForConnected(5000)) {
		m_pSocket->abort();
		return false;
	}
	// 等待返回后再次确认状态，避免瞬时接通又被对端掐断仍报成功
	if (m_pSocket->state() != QAbstractSocket::ConnectedState) {
		m_pSocket->abort();
		return false;
	}
	return true;
}

bool TcpSocket::closeConnection()
{
	if (!m_pSocket)
		return true;
	if (m_pSocket->state() != QAbstractSocket::UnconnectedState) {
		m_pSocket->disconnectFromHost();
		if (m_pSocket->state() != QAbstractSocket::UnconnectedState)
			m_pSocket->waitForDisconnected(1000);
		if (m_pSocket->state() != QAbstractSocket::UnconnectedState)
			m_pSocket->abort();
	}
	return true;
}

bool TcpSocket::sendData(const QString& data)
{
	if (m_pSocket->state() != QAbstractSocket::ConnectedState) {
		return false;
	}
	qint64 written = m_pSocket->write(data.toLatin1());
	m_pSocket->flush();
	return written != -1;
}

bool TcpSocket::sendData(const char* data, const qint64 length)
{
	if (m_pSocket->state() != QAbstractSocket::ConnectedState) {
		return false;
	}
	qint64 written = m_pSocket->write(data, length);
	m_pSocket->flush();
	return written != -1;
}

void TcpSocket::sendData(const char* data)
{
	if (m_pSocket->state() == QAbstractSocket::ConnectedState) {
		m_pSocket->write(data);
		m_pSocket->flush();
	}
}

bool TcpSocket::sendData(const QByteArray& data)
{
	if (m_pSocket->state() != QAbstractSocket::ConnectedState) {
		return false;
	}
	qint64 written = m_pSocket->write(data);
	m_pSocket->flush();
	return written != -1;
}

void TcpSocket::onRecvData()
{
	//获取套接字中的数据
	//QString mes = QString::fromUtf8(this->readAll());
	emit emitNewMessage(m_pSocket->readAll());
}
