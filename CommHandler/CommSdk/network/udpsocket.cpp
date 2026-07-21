#include "udpsocket.h"
#include <QNetworkDatagram>
#include <QElapsedTimer>

udpsocket::udpsocket(QObject* parent)
	: NetworkBase(parent)
	, m_bindResult(-1)
{
	m_socket = nullptr;
	m_bindAddress = QHostAddress::LocalHost;
	m_port = 8005;	// 默认端口号8005
	m_bRunning = false;
	m_bGetIP = false;

	qRegisterMetaType<QHostAddress>("QHostAddress");
	m_DestIP = QHostAddress::Broadcast;
	m_Destport = 8805;

	connect(&m_thread, &QThread::finished, this, [this]()
		{
			m_bRunning = false;
			if (m_socket)
			{
				m_socket->close();
				m_socket->deleteLater();
				m_socket = nullptr;
			}
		});

	connect(this, &udpsocket::requestSendData, this, &udpsocket::doSendData, Qt::QueuedConnection);
}

udpsocket::~udpsocket()
{
	closeConnection();
	m_thread.quit();
	m_thread.wait();
}

// 必须等线程内 bind 结果；端口占用/地址非法返回 false，禁止空报成功
bool udpsocket::initConnection(const QString& address, int port)
{
	if (m_bRunning || m_thread.isRunning())
		return false;
	if (port <= 0 || port > 65535)
		return false;

	if (address.isEmpty() || address == "0.0.0.0")
	{
		m_bindAddress = QHostAddress::AnyIPv4;
	}
	else
	{
		QHostAddress bindAddress(address);
		if (bindAddress.isNull())
			return false;
		m_bindAddress = bindAddress;
	}

	m_port = static_cast<quint16>(port);
	m_bGetIP = true;
	m_bindResult.store(-1);

	disconnect(&m_thread, &QThread::started, this, &udpsocket::run);
	connect(&m_thread, &QThread::started, this, &udpsocket::run, Qt::QueuedConnection);
	moveToThread(&m_thread);
	m_thread.start();

	QElapsedTimer timer;
	timer.start();
	while (m_bindResult.load() < 0 && timer.elapsed() < 3000)
		QThread::msleep(5);

	if (m_bindResult.load() != 1) {
		closeConnection();
		return false;
	}
	return true;
}

bool udpsocket::closeConnection()
{
	if (m_thread.isRunning())
	{
		m_thread.quit();
		m_thread.wait(3000);
	}
	m_bRunning = false;
	m_bindResult.store(-1);
	return true;
}

bool udpsocket::sendData(const QString& data)
{
	if (m_bRunning)
	{
		emit requestSendData(data.toLocal8Bit());
		return true;
	}
	return false;
}

bool udpsocket::sendData(const char* data, const qint64 length)
{
	if (m_bRunning)
	{
		emit requestSendData(QByteArray(data, static_cast<int>(length)));
		return true;
	}
	return false;
}

void udpsocket::sendData(const char* data)
{
	if (m_bRunning)
	{
		emit requestSendData(QByteArray(data));
	}
}

void udpsocket::SetDestIpAndPort(QHostAddress ip, quint16 port)
{
	m_DestIP = ip;
	m_Destport = port;
}

bool udpsocket::isRUN()
{
	return m_bRunning;
}

void udpsocket::OnReadyRead()
{
	QNetworkDatagram datagram;
	while (m_socket && m_socket->hasPendingDatagrams())
	{
		datagram = m_socket->receiveDatagram();
		if (m_bGetIP)
		{
			m_bGetIP = false;
			emit emitNewConn(datagram.senderAddress().toString(), datagram.senderPort());
		}

		emit emitNewMessage(datagram.data());
	}
}

void udpsocket::run()
{
	if (m_socket)
	{
		delete m_socket;
		m_socket = nullptr;
	}

	m_socket = new QUdpSocket();
	if (!m_socket->bind(m_bindAddress, m_port))
	{
		qDebug() << "UDP socket bind failed:" << m_bindAddress.toString() << m_port << m_socket->errorString();
		delete m_socket;
		m_socket = nullptr;
		m_bRunning = false;
		m_bindResult.store(0);
		return;
	}

	connect(m_socket, &QUdpSocket::readyRead, this, &udpsocket::OnReadyRead, Qt::DirectConnection);

	m_bRunning = true;
	m_bindResult.store(1);
	qDebug() << "UDP socket started, listening on:" << m_bindAddress.toString() << ":" << m_port;
}

void udpsocket::doSendData(const QByteArray& data)
{
	if (m_socket && m_bRunning)
	{
		m_socket->writeDatagram(data, m_DestIP, m_Destport);
	}
}
