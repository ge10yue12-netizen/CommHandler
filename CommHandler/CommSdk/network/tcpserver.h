#pragma once
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include "./NetworkBase.h"

class TcpServer : public  NetworkBase
{
	Q_OBJECT
public:
	TcpServer(QObject* parent = nullptr);

	// 初始化连接
	virtual bool initConnection(const QString& address, int port) override;

	// 断开连接
	virtual bool closeConnection() override;

	// 发送数据
	virtual bool sendData(const QString& data) override;
	virtual bool sendData(const char* data, const qint64 length) override;
	virtual void sendData(const char* data) override;
	// 获取本机IP地址s
	QHostAddress get_Host_IP_Address();
	QList<QString> get_All_Host_IP_Address() const;

private:
	QTcpServer* m_pServer;
	QList<QTcpSocket*> m_clientSockets;  // 管理多个客户端连接
	bool m_bConnected;
};

