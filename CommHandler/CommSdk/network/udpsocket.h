#pragma once
#include <QUdpSocket>
#include <atomic>
#include "./NetworkBase.h"

class udpsocket : public  NetworkBase
{
	Q_OBJECT
public:
	explicit udpsocket(QObject* parent = nullptr);	// 默认端口号8005
	~udpsocket();

	// 初始化连接（须等到 bind 成功/失败才返回）
	virtual bool initConnection(const QString& address, int port) override;

	// 断开连接
	virtual bool closeConnection() override;

	// 发送数据
	virtual bool sendData(const QString& data) override;
	virtual bool sendData(const char* data, const qint64 length) override;
	virtual void sendData(const char* data) override;
	// 设置目的主机ip和端口号，
	// QHostAddress::Broadcast 局域网内广播的方式发送数据
	void SetDestIpAndPort(QHostAddress, quint16);

	bool isRUN();

private slots:
	void OnReadyRead();			// 接收数据槽函数
	void doSendData(const QByteArray& data);  // 线程安全的发送槽函数

private:
	void run();

signals:
	void requestSendData(const QByteArray& data);  // 请求发送数据的信号

private:
	QUdpSocket* m_socket;
	QHostAddress m_bindAddress;
	quint16		 m_port;		// 监听端口号
	bool		 m_bGetIP;		// 是否可以获取sender IP	true - 可以， false - 不可以
	bool		 m_bRunning;	// 是否正在运行
	QHostAddress m_DestIP;		// 目标ip
	quint16		 m_Destport;	// 目标端口号
	QThread m_thread;
	// -1 等待中；0 bind 失败；1 bind 成功
	std::atomic<int> m_bindResult;
};
