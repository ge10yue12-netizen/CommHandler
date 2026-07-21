#pragma once

#include "./CommBase.h"
#include "Serial/SerLhgkProtocol.h"
#include <QSerialport>

struct DeviceDataStruct
{
	enum ComDataStatus
	{
		COMDATA_STATUS_DISABLE,
		COMDATA_STATUS_ENABLE,
	};
};


class SerialPortComm : public CommBase
{
	Q_OBJECT
public:
	SerialPortComm();
	~SerialPortComm();

	void SetToDefault();

	// 关联槽函数
	void ConnectSlots() override;

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

	void SendData(const QString& data) override {};

	// 保存配置
	bool SaveParInfo(const QString& filename) override;

	// 读取配置
	bool LoadParInfo(const QString& filename) override;

	// 将命令配置保存到本地文件
	bool saveCmdToFile(const std::string& filename) override;

	// 从本地文件读取命令配置
	bool loadCmdFromFile(const std::string& filename) override;

	//数字量输出
	void SendData2ComPort(float fStrain1, float fStrain2, float fMovement1, float fMovement2, float fReserve, float fTime);

	bool DataPackAndWrite(int value);

	void WaitForBytesWritten(const int& ms);

private:
	// 初始化getters以及setters
	void initParameters()override;

	// 初始化成员变量
	void initInfo() override;
signals:
	// 新数据到来
	// type 0- 力值 1 -温度
	void emitNewData(void*, int size, int type);

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

	// 收到字节但当前协议未解析出业务数据（调试助手可观测）
	void emitUnparsedRx(const QByteArray& raw);
private:
	// 数据组合后通知数据接收
	void MsgDataInput(QString sData, double* dData, int& size);
	// 匹配控制指令(HEX协议二)
	bool MatchCtrlData(QString sData);
	// 字符转换
	int     Str2Hex(const char* str, int nLen, char* data);
	QString ASCDecode(QString& strData);
	double  GetData(QString& strHex);
	int     Str2Hex_t(const QString str, int nLen, TCHAR* data);

	// 十六进制字符串转二进制
	QString HexToBin(QString& strHex);
	// 二进制转double
	double  BinToDec(QString& strBin, long nIndex);

	int  _HexChar(char ch);
	int  _HexFromQString(QString str);

	//模拟量输出
	double OutputVoltage(int iType, double dVol);
	//数据封装
	unsigned char* DataPack();

	void FloatToHex(unsigned char* str, int iRowNum, float fValue, int iSegSize);
	unsigned char* FloatToHex(float fValue1, float fValue2);

	void FixedDoubleToHex(unsigned char* str, int iRowNum, double dValue);
	QString formatValue(double value);

	double HexToFloat(QByteArray array);

private:
	//转换
	void HandleArray(unsigned char arr1[], int i);

	std::string ByteArrayToHex(unsigned char arr[], int i);

	float GetNFromData(float val, int n);

	inline void sendData(const std::vector<uint8_t>& buf)
	{
		QByteArray byteArray(reinterpret_cast<const char*>(buf.data()), static_cast<int>(buf.size()));
		m_serialPort.write(byteArray);
	}

	inline void SendOKResponse()
	{
		// Send OK response: 4F 4B 0D
		QByteArray response;
		response.append(0x4F);  // 'O'
		response.append(0x4B);  // 'K'
		response.append(0x0D);  // CR (Carriage Return)
		m_serialPort.write(response);
	}

	// 解析联恒协议 v1.1
	bool handleFrame(const std::vector<uint8_t>& buf);
	// 联恒串口流式拼帧：断帧等待，完整坏帧才上报未解析
	void processLhgkRxStream(const QByteArray& chunk);
	void processNegotiationFrame(const std::vector<uint8_t>& payload);
	void processControlCommand(const std::vector<uint8_t>& payload);
	void processDataStreamControl(const std::vector<uint8_t>& payload);
	void processPollingRequest();

	void sendErrorAndClose(uint8_t errCode, const std::string& msg);

private slots:
	// 接收新数据,这里将已经解析好的数据通过emitNewDatsa传出
	void onRecvDataSlots();

	void handleEventMsg(int ctrlCmd, int viewID, int msg);
private:
	QSerialPort m_serialPort;
	unsigned char m_tmpS1[4]{}, m_tmpS2[4]{}, m_tmpS3[4]{}, m_tmpS4[4]{}, m_tmpS5[4]{}, m_tmpS6[4]{}, m_tmpS7[2]{};
	std::string m_sX1[52]{};
	unsigned char m_sX2[56]{};
	QString m_sCtrlData;
	QString m_sInputData;
	QByteArray  g_serialBuffer;
	// 联恒协议接收拼帧缓冲（与 g_serialBuffer 分用，避免协议串扰）
	QByteArray m_lhgkRxBuffer;
	double m_dfreq;
	int m_iDataBits;
	// 外部控制信息
	SerialPortInfo m_SerialPortInfo;
	SerLhgkProtocol m_SerLhgkProtocol;

	std::unordered_map<QString, std::function<void(const Any&)>> setters;
	std::unordered_map<QString, std::function<Any()>> getters;
};

