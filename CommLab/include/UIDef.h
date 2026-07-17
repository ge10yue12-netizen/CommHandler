#pragma once
#include <cstdint>
#include <QString>
#include <comutil.h>
#include <QStringList>

// 五个模块数据类型
enum COMM_DATA_TYPE {
	COMM_LVE = 0,
	COMM_2DDIC,
	COMM_STRAIN,
	COMM_MOVE,
	COMM_ANGLE,
};

// 通讯类型
enum CommType
{
	NETWORK = 0,
	SERIAL,
	VOLTAGE,
	PULSE
};

// 单位
enum CommUnit
{
	UNKNOWN_UNIT = 0,
	N = 1,
	KN = 2,
	DEGREE_CENTIGRADE = 3,	// 温度 摄氏度
};

enum CommCtrlCmd
{
	UnknownCmd = 0,
	ControlSoftStatus,		// 控制软件状态
	UpdateView,				// 刷新UI
};

// 视窗ID
enum CommViewID
{
	UNKNOWN_VIEW_ID = 0,		// 未知
	MainView_ID = 1,
	SerialPort_ID = 2,
	Socket_ID = 3,
	PulseView_ID = 4,
};

enum VolotageModel
{
	AD = 0,
	DA = 1,
	CI = 2,
	DI = 3,
};

enum CommTypeState {
	TYPE_NETWORK = 1 << 0,	 // 0001
	TYPE_SERIAL = 1 << 1,	 // 0010
	TYPE_VOLTAGE = 1 << 2,	 // 0100
	TYPE_PULSE = 1 << 3		 // 1000
};

struct SocketProtol
{
	// 协议一：
	typedef struct tag_SOCKETDATA
	{
		double dForce;	// 力值
		double dMove;	// 位移
		double dStrain;	// 应变
	}SOCKETDATA;

	// 协议二： 南航DIC双轴拉伸机
#pragma pack(push, 1)

	enum MessageType {
		CONTROL_MESSAGE = 0x01,		// 控制命令(十进制 1)
		DATA_MESSAGE = 0x02,		// 数据信息
		RETURN_MESSAGE = 0x03,		// 返回信息
		SERVER_DATA_MESSAGE = 0x04,      // 服务器数据消息 (新增)
	};
	// 控制命令
	enum ControlCommand {
		START_CALC = 0x0A,			// 同步开始命令 (十进制 10)
		STOP_CALC = 0x0B,			// 同步结束命令
		DATA_COLL_POLLING = 0x0C,	// 数据轮询采集命令
		DATA_COLL_FLOW = 0x0D,		// 数据流采集命令
		ACQ_STATE = 0x0E,			// 获取通讯状态命令
	};
	// 返回软件状态信息命令
	enum ReturnCommand {
		START_RUNNING = 0x64,		// 通讯已处于连接状态(十进制100)
		CAMERA_NOT_OPEN = 0x65,		// 相机未打开 
		DISTANCE_NOT_SET = 0x66,	// 未设置标距
		NORMAL_RUNNING = 0x67,		// 软件开始计算
		ALREADY_RUNNING = 0x68,		// 已经在计算
		NORMAL_STOP = 0x69,			// 软件正常执行停止计算
		ALREADY_STOPPED = 0x6A,		// 软件已经处于停止计算
	};
	//struct MessageHeader {
	//	uint8_t msgType; // 控制信息或数据信息
	//};

	// 控制命令结构体 和 返回软件状态信息结构体
	struct ControlMessage {
		uint8_t msgType;
		uint8_t command;
	};



	struct ForceData {
		uint8_t msgType;
		double forceValues[4];	// 包含四通道力值，分别依次是:x1,y1,x2,y2
	};

	//struct FreqMessage {
	//	uint8_t   msgType;
	//	int 	frequency;// 控制试验机发出力值数据的频率，frequency=1000 则表示控制试验机每秒发送1000次力值数据
	//};

	struct ServerData {
		uint8_t msgType;        // 消息类型, 固定为 SERVER_DATA_MESSAGE (0x04)
		double valueX;          // 水平轴 数据 为-1时，则表示此通道没有数据
		double valueY;          // 垂向轴 数据 为-1时，则表示没有数据
	};

#pragma pack(pop)

};

struct Data
{
	double dForce;	// 力值
	double dMove;	// 位移
	double dStrain;	// 应变
};

struct DataInput
{
	int iChannelState;			// 通道状态
	int  iChannelSize;			// 通道数
	int  iTriggerCtrl;			// 触发控制
	int  iTransferType;			// 传输形式 0-数据流 1 - 轮询 

	int  iDataType[4];				// 数据类型 0-力值 1-温度
	int  iUnit[4];					// 单位
};

struct DataOutput
{
	int iChannelState;			// 通道状态
	int  iChannelSize;			// 通道数
	int  iTriggerCtrl;			// 触发控制
	int  iTransferType;			// 传输形式	0-数据流 1 - 轮询 

	int  iDataModel[8];			// 模块
	int  iDataType[8];			// 标距 横向-纵向
	int  iData[8];				// 类型 位移-应变
	bool bReverse[8];			// 数据取反
	//int  iUnit[8];				// 单位
};

struct SocketInfo
{
	int			iTransferType;			// 0 - TCP		1 - UDP
	int			iModel;					// 0 - 服务器端 1 - 客户端
	QStringList AllIp;
	QString		sLocalIP;				// 本机IP地址
	int			iLocalPort;				// 本机端口
	QString		sPurIP;					// 目的IP
	int			iPurPort;				// 目的端口	
	int			iProtoType;				// 协议一 联恒光科 协议二 - json 协议三 - 万测  协议四 - 中机通讯 协议五 - 三思实验  
	bool		bOpenCMD[3];			// 对应开始 结束 退出 控制命令是否开启
	char		cCMD[3][16];			// 控制命令
	int			iSymbolType;			// 分割符号的类型	1,逗号  2， 空格  3，分号
	bool		bOnlineCollect;			// 是否实时计算
	bool		bAllOpenCamera;			// 相机是否全部打开
	bool		bConntction;			// 是否处于连接状态 
	bool        bInquireSendFlag;
	DataInput   input;
	DataOutput  output;
};

struct SerialPortInfo
{
	bool bComOpened;		// 是否开启
	//int iEncryptionMode;	// 加密方式     0:IEEE  1:HEX 
	int	iComPort;			// 端口号       0-COM1
	int	iComBaud;			// 波特率       0-4800 1-9600 2-19200 3-38400 4-57600 5-115200
	int	iParity;			// 校检位       0-none 1-Even 2-odd 3-space 4-mark
	int	iDataBits;			// 数据位       0-5    1-6    2-7   3-8
	int	iStopBits;			// 停止位       0-1    1-1.5  2-2
	int	iProtocolType;		// 协议类型     0-三思测控  1-长春科新   2-时代新材株洲   3 IEEE  4冠腾试验


	double dInForceRange;			// 力值量程
	bool bHexSetZero;
	bool bInDataCombinating;
	bool bInquireSendFlag;
	bool isCollecting;
	DataInput   input;
	DataOutput  output;
};

struct VoltageInfo
{
	// 模式
	//int iModel;						// 0-AD  1-DA

	// AD 
	int					  iADInputMode;			// 输入方式
	bool				       bADOpen;			// 是否有开启通道
	int					    iADChannel;			// 当前输入通道
	int						   iADMode;			// 当前输入模式
	QString				   sADName[16];			// 通道对应的数据名  
	int				   iADDataType[16];			// 数据类型
	int					   iADUnit[16];			// 单位
	bool				    bADExe[16];			// 开启的通道
	bool				bAutoADExe[16];			// 自动的通道
	double		   	   dADRange[16][2];			//  
	double			 dCompensation[16];			// 补偿值
	double    dVADCaliArray[16][10][2];			// 标定
	int					iThresholdType;			// 异常阈值类型
	double		 dAbnormalThreshold[2];
	//int						iForceUnit;			// 力值单位
	double				   dValueK[16];			// 比例系数
	double				   dValueB[16];			// 补偿参数

	// DA 模拟量输出							// 目前只使用了2个输出通道：0和1
	bool						bDAOpen;
	int						 iDAChannel;		// 当前输出通道
	bool					  bDAExe[2];		// 2个通道是否开启
	bool				  bAutoDAExe[2];			// 自动的通道
	double			 dDAVoltRange[2][2];		// 输出电压范围 
	double		   vScaleFactorArray[2];		// 电压比值
	double		 vOutputMinVoltArray[2];		// 最小电压
	double		 vOutputMaxVoltArray[2];		// 最大电压
	double			  vMinValueArray[2];		// 最小值
	double			  vMaxValueArray[2];		// 最大值

	QString				strResult[2][4];	    // 通道/模块-类型-数据 结果显示

	DataInput   input;
	DataOutput  output;

	int        iControllerType;					// 采集卡类型
	int		   iExternalTrigger;				// 外触发信号端口
	int		   iTriggerMode;				    // 触发方式
	bool       bTriggerVerify;					// 触发带校验
	int        iReadCI;							// 读取CI信号打开/关闭
	bool       bOpenDI;							// 是否开启DI
};

struct PulseInfo {
	int iPulseCardType;	// 卡类型
	bool bPulseOpened;	// 是否开启脉冲通讯
	bool bPulseSending;	// 是否正在发送数据
	double dAccuracy;	// 精度
	double dCaliValue;	// 标定值
	double dFrameRate;	// 帧率
	char* cPulseIPAddress;
	int iUSBVo;			// usb初始速度
	int iUSBVt;			// usb运行速度
	int iNETVo;			// net初始速度
	int iNETVt;			// net运行速度

	DataInput   input;
	DataOutput  output;
};

#define     W_CUSTOM												WM_USER + 10 
#define     W_CUSTOM_COMM_STARTCALC									W_CUSTOM + 1	// 开始计算
#define     W_CUSTOM_COMM_STOPCALC									W_CUSTOM + 2	// 停止计算
#define     W_CUSTOM_COMM_EXITPROG									W_CUSTOM + 3	// 退出程序
#define     W_CUSTOM_COMM_UPDATEPULSECTRL                           W_CUSTOM + 4    // 刷新脉冲
#define     W_CUSTOM_COMM_PULSECALIDONE                             W_CUSTOM + 5    // 脉冲标定完成
#define     W_CUSTOM_COMM_PULSEBUTTON                               W_CUSTOM + 6    // 刷新脉冲按钮
#define     W_CUSTOM_COMM_DATASTREAMING                             W_CUSTOM + 7    // 设置数据流 0-开始 1-关闭
#define     W_CUSTOM_COMM_DATAPOLLING                               W_CUSTOM + 8    // 轮询请求命令  
#define     W_CUSTOM_COMM_SINGALTRIIMAGESAVE                        W_CUSTOM + 9    // 开启一次信号触发存图 
#define     W_CUSTOM_COMM_RESET_LVE_LENGTH                          W_CUSTOM + 10   // 重置标距 
#define     W_CUSTOM_COMM_CALC_LVE_LENGTH                           W_CUSTOM + 11   // 识别线条标距 
#define     W_CUSTOM_ZERO_CLEARING									W_CUSTOM + 12   // 清零，清楚所有数据
#define     W_CUSTOM_COMM_AUTO_SAVE_IMAGE                           W_CUSTOM + 13   // 通讯自动存图
