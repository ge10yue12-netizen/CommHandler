#pragma once

#include "./CommBase.h"
#include "network/NetworkBox.h"
#include "Socket/NetLhgkProtocol.h"

struct TestData {
	char status;        // 'T' = Running, 'S' = Stopped
	float load;         // Load (kN)
	float deformation;  // Deformation (mm)
	float displacement; // Displacement (mm)
	float time;         // Time (s)

	TestData() : status('S'), load(0.0f), deformation(0.0f),
		displacement(0.0f), time(0.0f) {
	}
};
class SocketComm : public CommBase
{
	Q_OBJECT
public:
	SocketComm();
	~SocketComm();

	void SetToDefault();

	// 关联槽函数
	void ConnectSlots()override;

	// 设置所有参数
	bool SetSysParInfo(void*) override;

	// 获取所有参数
	bool GetSysParInfo(void*) const override;

	// 通用参数设置接口
	void setParameter(const QString& key, const Any& value) override;

	// 通用参数获取接口
	Any getParameter(const QString& key) const override;

	// 设置外部设备发送的力值频率
	bool setExternalFreq(const int& freq) override;

	// 连接
	bool Connect(int type = -1) override;

	// 断开连接
	bool Disconnect(int type = 0) override;	// type==0 关闭当前连接   1 关闭所有连接，

	// 发送数据
	//void SendData(void*, size_t size) override;

	void SendData(const std::vector<double>& vData) override;

	void SendData(const QString& data) override { m_controlBox.sendData(data); }

	// 保存配置
	bool SaveParInfo(const QString& filename) override;

	// 读取配置
	bool LoadParInfo(const QString& filename) override;

	// 将命令配置保存到本地文件
	bool saveCmdToFile(const std::string& filename) override;

	// 从本地文件读取命令配置
	bool loadCmdFromFile(const std::string& filename) override;

private:
	// 初始化getters以及setters
	void initParameters()override;

	// 初始化成员变量
	void initInfo() override;

	// 组包
	// 协议JSON通用
	QString CreateJsonDataPack(const std::vector<double>& data);

	// 协议二 中机南航双轴装备
	QString CreateZhongJiShuangZhouFour(const std::vector<double>& data);

	// 协议三 万测
	QString CreateWanceSimbolDataPack(const std::vector<double>& data);

	// 海纳百川，自动化
	QString CreateHNBCDataPack(const std::vector<double>& data);

	// 纳百川 自动化线识别 // 纵向位移，横向位移，纵向标距，横向标距
	QString GenJsonDocument(double L1, double b1, double Le1, double bo1);
signals:
	// 新数据到来
	// type 0- 力值 1 -温度 2-四通道力值 3 - 力值 + 温度
	void emitNewData(void*, int size, int type);

	// 新的连接到来
	// iType 0- TCP server  1 - Tcp Client     2: Udp - server      3:: UDP - client
	void emitNewConn(int iType, QString ip, int port);

	// 第三方主动断开连接，
	void emitClientDisConn();

	// 解析的命令，发送event信号
	void emitEventMsg(int ctrlCmd, int ViewID, int msg);

	// 解析的命令
	// 用法： QVariantMap extra;
	//extra["x"] = 12.34;
	//extra["y"] = 56.78;
	//extra["speed"] = 3.5;
	//extra["name"] = "test";
	//extra["valid"] = true;
	void emitEventMsgAndData(const int& ctrlCmd, const int& ViewID, const int& msg, const QVariantMap& extra);

	// 收到字节但当前协议未解析出业务数据
	void emitUnparsedRx(const QByteArray& raw);
private slots:
	// 设置新连接的ip地址
	void onNewConnIpAndPortSlots(QString IP, int port);

	// 第三方主动断开连接
	void onClientDisConnSlots(qintptr);

	// 接收新数据,这里将已经解析好的数据通过emitNewDatsa传出
	void onRecvDataSlots(QByteArray);

	void handleEventMsg(int ctrlCmd, int viewID, int msg);

private:
	// 解析联恒协议
	bool handleFrame(const std::vector<uint8_t>& buf);
	// 联恒网口流式拼帧：断帧等待，完整坏帧才上报未解析
	void processLhgkRxStream(const QByteArray& chunk);

	void processNegotiationFrame(const std::vector<uint8_t>& payload);
	void processControlCommand(const std::vector<uint8_t>& payload);
	void processDataStreamControl(const std::vector<uint8_t>& payload);
	void processPollingRequest();

	void sendErrorAndClose(uint8_t errCode, const std::string& msg);

	inline void sendData(const std::vector<uint8_t>& buf)
	{
		const char* sendData = reinterpret_cast<const char*>(buf.data());
		m_controlBox.sendData(sendData, buf.size());
	}

	// 解析协议二  json
	void ParsingProtocolI(const QByteArray&);
	// 发送返回 code；助手观测开启时同步 PROTO_ACK
	void SendReturnMsg(const int& code, const QString& msg, const int& tn);
	// 助手观测：将已写出的 JSON ACK 同步为参数事件（默认关闭，不影响正式软件）
	void notifyWireTxAck(int code, const QString& message, int tn, const QString& wireJson);
	// 助手观测：未识别载荷上报（默认关闭）
	void emitAssistUnparsedRx(const QByteArray& raw);

	// 解析协议三消息   万测
	void ParsingProtocolII(const QByteArray&);

	// 协议四 中机 南航双轴实验机
	void ParsingProtocolZhongji(const QByteArray& message);
	void AnalysisZhongjiControl(const SocketProtol::ControlMessage&);		// 控制命令		

	// 协议五 三思
	void ParsingProtocolSansi(const QByteArray& message);
	// 协议六 触发存图

	// 
	// 福建威盛协议
	// Parse UDP data (only parse load)
	// Format: TL123.45E67.89D12.34M56.78
	bool parseFuJianWeishengData(const QString& message, TestData& data);

	// 海纳百川自动化
	void paeseNaBaiChuanData(const QByteArray&);

	// 纳百川自动化识别计算线条
	void paeseNaBaiChuanDataCalcLine(const QByteArray&);
private:
	// 外部控制信息
	SocketInfo m_socketInfo;
	// 助手联调观测开关（不进 SocketInfo，避免破坏参数二进制布局）
	bool m_bAssistObserve = false;

	double m_dfreq;
	NetLhgkProtocol lhgkProto_;
	// 联恒协议接收拼帧缓冲（TCP 可能半包/粘包）
	QByteArray m_lhgkRxBuffer;

	std::unordered_map<QString, std::function<void(const Any&)>> setters;
	std::unordered_map<QString, std::function<Any()>> getters;

	// 内部控制信息
	NetworkBox m_controlBox;
};

