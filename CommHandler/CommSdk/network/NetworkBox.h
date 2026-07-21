#ifndef NETWORKBOX_H
#define NETWORKBOX_H

#include <QObject>
#include <QHash>
#include <QHostAddress>

class TcpServer;
class TcpSocket;
class udpsocket;
class NetworkBase;
class  NetworkBox : public QObject
{
	Q_OBJECT
public:
	NetworkBox(QObject* parent = nullptr);
	virtual ~NetworkBox();

	enum NetType
	{
		iTcpServer = 0,
		iTcpClient,
		iUdp
	};

	// 设置通讯类型
	void SetNetType(NetType type);

	// 设置udp端口号以及ip地址
	// 设置目的主机ip和端口号，
	// QHostAddress::Broadcast 局域网内广播的方式发送数据
	void SetDestIpAndPort(QHostAddress, quint16);

	// 获取本机的所有ip地址
	QStringList GetHostAllIPAddress();

	// 初始化连接
	bool initConnection(const QString& address, int port);

	// 断开连接
	bool closeConnection();

	// 发送数据
	bool sendData(const QString& data);
	void sendData(const char* data, const qint64 length);
	void sendData(const char* data);

signals:
	void emitNewConn(QString, int);	// 新的连接到来
	void emitNewMessage(QByteArray); 			// 新数据到来
	void emitDisConnected(qintptr);

private slots:
	void handleNewConn(QString, int);
	void handleNewMessage(QByteArray);
	void handleDisConn(qintptr);

private:
	NetworkBase* m_pCurNetModule;
	QHash<NetType, NetworkBase*> m_pNetModules;
};

#endif // NETWORKBOX_H
