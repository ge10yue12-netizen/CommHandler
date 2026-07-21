#pragma once
#include "../UIDef.h"

#include <QString>
#include <string>

class CComportControl
{
public:
	CComportControl(void);
	~CComportControl(void);

	// 基本的打开与关闭
	bool OpenTensileComport();
	bool OpenFLCComport();
	bool CloseTensileComport();
	bool CloseFLCComport();

	// 消息处理
	bool PushComChar(int iport);
	bool PushTensileComChar();
	bool PushFLCComChar();
	bool TranslateTensileData();
	bool TranslateFLCData();

	// 发送获取消息指令
	bool SendRequireMsg(int iStageIndex);
	// 发送电机控制信号
	// 速度闭环控制
	bool SendVelCtrlMsg(int iDevAddress, int iVel);//iVel单位：0.1RPM
	// 相对位置闭环控制
	bool SendRelativePosCtrlMsg(int iDevAddress, int iCount);
	// 设置当前位置为原点
	bool SetCurPosAsOrigin(int iDevAddress);
	// 回到原点(最短距离)
	bool BackToOrigin(int iDevAddress);
	// 关闭电机
	bool CloseElecMachine(int iDevAddress);

	// 读取电机命令列表
	bool LoadCommList();
	// 保存电机命令列表
	bool SaveCommList();

private:
	bool m_bTensileComOpen;
	COMCONFIG  m_curFLCComD;
	COMCONFIG  m_curTensileComD;
	QString    m_sTensileTransData;
	QString    m_sFLCTransData;

protected:
	QString ASCDecode(QString& strData);
	int     Str2Hex(const char* str, int nLen, char* data);
	int     Str2Hex_t(const QString str, int nLen, TCHAR* data);

	// 十六进制字符串转二进制
	QString HexToBin(QString& strHex);
	// 二进制转double
	double  BinToDec(QString& strBin, long nIndex);
	double  GetData(QString& strHex);
	std::string dec2hex(int i, int width);
	// CRC16校验
	unsigned char* CRC16(unsigned char* pArray, unsigned int iLen);
};
