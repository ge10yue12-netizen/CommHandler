#pragma once
#include <QObject>
#include <QString>
#include <QDebug>
#include <QThread>

class NetworkBase : public QObject
{
	Q_OBJECT
public:
	explicit NetworkBase(QObject* parent = nullptr) : QObject(parent) {}

	// 初始化连接
	virtual bool initConnection(const QString& address, int port) = 0;

	// 断开连接
	virtual bool closeConnection() = 0;

	// 发送数据
	virtual bool sendData(const QString& data) = 0;
	virtual bool sendData(const char* data, const qint64 length) = 0;
	virtual void sendData(const char* data) = 0;

signals:
	void emitNewConn(QString address, int port);
	void emitNewMessage(QByteArray data);
	void emitDisConnected(qintptr socketDescriptor);
};

