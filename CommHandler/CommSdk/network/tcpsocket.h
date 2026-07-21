#pragma once
#include <QTcpSocket>
#include "./NetworkBase.h"

class TcpSocket :public  NetworkBase
{
	Q_OBJECT
public:
	explicit TcpSocket(QObject* parent = nullptr);
	~TcpSocket();
	// 初始化连接
	virtual bool initConnection(const QString& address, int port) override;

	// 断开连接
	virtual bool closeConnection() override;

	// 发送数据
	virtual bool sendData(const QString& data) override;
	virtual bool sendData(const char* data, const qint64 length) override;
	virtual void sendData(const char* data) override;
	bool sendData(const QByteArray& data);
private slots:
	void onRecvData();

private:
	QTcpSocket* m_pSocket;
};
