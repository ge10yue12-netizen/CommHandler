#include "ComportControl.h"
#include "./PCOMM.H"

using namespace std;

CComportControl::CComportControl(void)
{
	m_sFLCTransData = "";
	m_sTensileTransData = "";
	m_curFLCComD.bComUsed = false;
	m_curFLCComD.iComBaud = B9600;
	m_curFLCComD.iComPort = 3;
	m_curFLCComD.iDataBits = BIT_8;
	m_curFLCComD.iStopBits = STOP_1;
	m_curFLCComD.iParity = P_NONE;
	m_curTensileComD.bComUsed = false;
	m_curTensileComD.iComBaud = B9600;
	m_curTensileComD.iComPort = 2;
	m_curTensileComD.iDataBits = BIT_8;
	m_curTensileComD.iStopBits = STOP_1;
	m_curTensileComD.iParity = P_NONE;

}


CComportControl::~CComportControl(void)
{
	CloseTensileComport();
	CloseFLCComport();
}



void CALLBACK TensilecntIrq(int port)
{
	/* You should NOT read data at this point. */
	/* Send message to let main program to read data. */
	//QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_COMPORT_RECEIVED_DATA, port, 0));
	//::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), MESSAGE_COMPORT_RECEIVED_DATA, port, 0);
}

void CALLBACK FLCcntIrq(int port)
{
	/* You should NOT read data at this point. */
	/* Send message to let main program to read data. */
	//QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_COMPORT_RECEIVED_DATA, port, 0));
	//::PostMessage(theApp.m_pMainWnd->GetSafeHwnd(), MESSAGE_COMPORT_RECEIVED_DATA, port, 0);
}

bool CComportControl::SendRequireMsg(int iStageIndex)
{
	//m_iProjectType = ProjectController::getInstance()->m_projectOperation.m_iProjectType;
	//m_iProjectIndex = ProjectController::getInstance()->m_projectOperation.GetCurProjectIndex();
	//m_iImageIndex = iStageIndex;

	//if (m_bFLCComOpen && SystemParController::getInstance()->m_sysParm.m_sysCollect.m_iForceRecType == 0)
	//{
	//	char  cData_req[] = "24 01 0D 0A";
	//	char  temp[5];
	//	int   nRlen;
	//	// 设置自动
	//	nRlen = Str2Hex(cData_req, 12, temp);
	//	sio_write(m_curFLCComD.iComPort, temp, nRlen);
	//}

	//if (m_bTensileComOpen && SystemParController::getInstance()->m_sysParm.m_sysCollect.m_iForceRecType == 0)
	//{
	//	char  cData_req[] = "24 01 0D 0A";
	//	char  temp[5];
	//	int   nRlen;
	//	// 设置自动
	//	nRlen = Str2Hex(cData_req, 12, temp);
	//	sio_write(m_curTensileComD.iComPort, temp, nRlen);
	//}
	return true;
}

bool CComportControl::SendVelCtrlMsg(int iDevAddress, int iVel)
{
	unsigned char data[7]{};
	unsigned char* pCheckCode = NULL;
	unsigned char wholePackage[9]{};
	string str = dec2hex(iDevAddress, 2);
	string str1 = dec2hex(iVel, 4);
	data[0] = 0x3E;
	data[1] = 0x03;
	data[2] = strtoul(str.c_str(), 0, 16);
	data[3] = 0x54;
	data[4] = 0x02;
	data[5] = strtoul(str1.substr(2, 2).c_str(), 0, 16);
	data[6] = strtoul(str1.substr(0, 2).c_str(), 0, 16);
	//unsigned char ch[7] = { 0x3E,0x00,0x01,0x56,0x02,0xE8,0x03 };
	pCheckCode = CRC16(data, 7);
	int i = 0;
	for (; i < 7; i++)
	{
		wholePackage[i] = data[i];
	}
	for (; i < 9; i++)
	{
		wholePackage[i] = pCheckCode[i - 7];
	}
	sio_write(m_curTensileComD.iComPort, (char*)wholePackage, 9);
	return true;

}

bool CComportControl::SendRelativePosCtrlMsg(int iDevAddress, int iCount)
{
	unsigned char data[7]{};
	unsigned char* pCheckCode = NULL;
	unsigned char wholePackage[9]{};
	string str = dec2hex(iDevAddress, 2);
	string str1 = dec2hex(iCount, 4);
	data[0] = 0x3E;
	data[1] = 0x00;
	data[2] = strtoul(str.c_str(), 0, 16);
	data[3] = 0x56;
	data[4] = 0x02;
	data[5] = strtoul(str1.substr(2, 2).c_str(), 0, 16);
	data[6] = strtoul(str1.substr(0, 2).c_str(), 0, 16);
	//unsigned char ch[7] = { 0x3E,0x00,0x01,0x56,0x02,0xE8,0x03 };
	pCheckCode = CRC16(data, 7);
	int i = 0;
	for (; i < 7; i++)
	{
		wholePackage[i] = data[i];
	}
	for (; i < 9; i++)
	{
		wholePackage[i] = pCheckCode[i - 7];
	}
	sio_write(m_curTensileComD.iComPort, (char*)wholePackage, 9);
	return true;
}

bool CComportControl::SetCurPosAsOrigin(int iDevAddress)
{
	unsigned char data[5]{};
	unsigned char* pCheckCode = NULL;
	unsigned char wholePackage[7]{};
	string str = dec2hex(iDevAddress, 2);
	data[0] = 0x3E;
	data[1] = 0x01;
	data[2] = strtoul(str.c_str(), 0, 16);
	data[3] = 0x21;
	data[4] = 0x00;
	pCheckCode = CRC16(data, 5);
	int i = 0;
	for (; i < 5; i++)
	{
		wholePackage[i] = data[i];
	}
	for (; i < 7; i++)
	{
		wholePackage[i] = pCheckCode[i - 5];
	}
	sio_write(m_curTensileComD.iComPort, (char*)wholePackage, 7);
	return true;
}

bool CComportControl::BackToOrigin(int iDevAddress)
{
	unsigned char data[5]{};
	unsigned char* pCheckCode = NULL;
	unsigned char wholePackage[7]{};
	string str = dec2hex(iDevAddress, 2);
	data[0] = 0x3E;
	data[1] = 0x02;
	data[2] = strtoul(str.c_str(), 0, 16);
	data[3] = 0x52;
	data[4] = 0x00;
	pCheckCode = CRC16(data, 5);
	int i = 0;
	for (; i < 5; i++)
	{
		wholePackage[i] = data[i];
	}
	for (; i < 7; i++)
	{
		wholePackage[i] = pCheckCode[i - 5];
	}
	sio_write(m_curTensileComD.iComPort, (char*)wholePackage, 7);
	return true;
}

bool CComportControl::CloseElecMachine(int iDevAddress)
{
	unsigned char data[5]{};
	unsigned char* pCheckCode = NULL;
	unsigned char wholePackage[7]{};
	string str = dec2hex(iDevAddress, 2);
	data[0] = 0x3E;
	data[1] = 0x04;
	data[2] = strtoul(str.c_str(), 0, 16);
	data[3] = 0x50;
	data[4] = 0x00;
	pCheckCode = CRC16(data, 5);
	int i = 0;
	for (; i < 5; i++)
	{
		wholePackage[i] = data[i];
	}
	for (; i < 7; i++)
	{
		wholePackage[i] = pCheckCode[i - 5];
	}
	sio_write(m_curTensileComD.iComPort, (char*)wholePackage, 7);
	return true;

}

bool CComportControl::LoadCommList()
{
	/*QString sPath = ProjectController::getInstance()->m_projectProperties.m_strExeConfigPath + "CaliCommList.dat";
	ifstream inFile;
	inFile.open(LPCTSTR(sPath.utf16()), ios::in);
	if (inFile.is_open())
	{
		Context::getInstance()->m_vCommandList.clear();
		int iSize = -1;
		inFile >> iSize;
		if (iSize == -1)
		{
			return false;
		}
		for (int i = 0; i < iSize; i++)
		{
			string str = "";
			inFile >> str;
			Context::getInstance()->m_vCommandList.push_back(QString::fromStdString(str));
		}
		return true;
	}*/
	return false;
}

bool CComportControl::SaveCommList()
{
	/*QString sPath = ProjectController::getInstance()->m_projectProperties.m_strExeConfigPath + "CaliCommList.dat";
	ofstream outFile;
	outFile.open(LPCTSTR(sPath.utf16()), ios::out | ios::binary);
	if (outFile.is_open())
	{
		int iSize = Context::getInstance()->m_vCommandList.size();
		outFile << iSize << endl;
		for (int i = 0; i < iSize; i++)
		{
			string str = Context::getInstance()->m_vCommandList[i].toStdString();
			outFile << str << endl;
		}
		return true;
	}*/
	return false;
}

// 基本的打开与关闭
bool CComportControl::OpenTensileComport()
{
	// 如果已经打开，则先关闭
	if (m_bTensileComOpen)
	{
		CloseTensileComport();
	}

	m_curTensileComD = SystemParController::getInstance()->m_sysParm.m_sysCollect.m_tensileComConfig;
	if (!m_curTensileComD.bComUsed)
	{
		return false;//打开失败
	}

	if (sio_open(m_curTensileComD.iComPort) != SIO_OK)
		return false;

	// 已打开标识符
	m_bTensileComOpen = true;

	if (sio_lctrl(m_curTensileComD.iComPort, 0) != SIO_OK)
		return false;
	if (sio_ioctl(m_curTensileComD.iComPort, m_curTensileComD.iComBaud, m_curTensileComD.iParity | m_curTensileComD.iDataBits | m_curTensileComD.iStopBits) != SIO_OK)
		return false;
	if (sio_cnt_irq(m_curTensileComD.iComPort, TensilecntIrq, 1) != SIO_OK)
		return false;
	m_sTensileTransData.clear();
	return true;
}
bool CComportControl::OpenFLCComport()
{
	//// 如果已经打开，则先关闭
	//if (m_bFLCComOpen)
	//{
	//	CloseFLCComport();
	//}

	//m_curFLCComD = SystemParController::getInstance()->m_sysParm.m_sysCollect.m_flcComConfig;
	//if (!m_curFLCComD.bComUsed)
	//{
	//	return false;//打开失败
	//}

	//if (sio_open(m_curFLCComD.iComPort) != SIO_OK)
	//	return false;
	//// 已打开标识符
	//m_bFLCComOpen = true;

	//if (sio_ioctl(m_curFLCComD.iComPort, m_curFLCComD.iComBaud, m_curFLCComD.iParity | m_curFLCComD.iDataBits | m_curFLCComD.iStopBits) != SIO_OK)
	//	return false;
	//if (sio_cnt_irq(m_curFLCComD.iComPort, FLCcntIrq, 1) != SIO_OK)
	//	return false;
	//m_sFLCTransData.clear();
	return true;
}
bool CComportControl::CloseTensileComport()
{
	if (!m_bTensileComOpen)
	{
		return true;
	}
	sio_cnt_irq(m_curTensileComD.iComPort, NULL, 0);
	sio_close(m_curTensileComD.iComPort);
	m_bTensileComOpen = false;
	return true;
}
bool CComportControl::CloseFLCComport()
{
	/*if (!m_bFLCComOpen)
	{
		return true;
	}
	sio_cnt_irq(m_curFLCComD.iComPort, NULL, 0);
	sio_close(m_curFLCComD.iComPort);
	m_bFLCComOpen = false;*/
	return true;
}

// 消息处理
bool CComportControl::PushComChar(int iport)
{
	if (iport == m_curTensileComD.iComPort && m_bTensileComOpen)
	{
		return PushTensileComChar();
	}
	/*else if (iport == m_curFLCComD.iComPort && m_bFLCComOpen)
	{
		return PushFLCComChar();
	}*/
	return false;
}

bool CComportControl::PushTensileComChar()
{
	if (!m_bTensileComOpen)//未打开，或者当前的Port与系统port口不相同
		return false;
	char temp[256] = { 0 };
	int nLen = sio_read(m_curTensileComD.iComPort, temp, 256);
	if (nLen <= 0)
	{
		return false;
	}
	for (int i = 0; i < nLen; i++)
	{
		QString strTemp;
		int iTemp = 0;
		memcpy(&iTemp, &temp[i], 1);
		strTemp = QString("%1").arg(iTemp, 2, 16, QLatin1Char('0')).toUpper();
		m_sTensileTransData += strTemp;
	}

	if (SystemParController::getInstance()->m_sysParm.m_sysCollect.m_iForceRecType == 0 && m_sTensileTransData.left(2) == "24" && m_sTensileTransData.right(4) == "0D0A")
	{
		QString strNum = QString("%1---").arg(QString::number(m_sTensileTransData.length()));
		//m_pMainFrame->m_pProcessLogView->AddLog(strNum + m_sTensileTransData);
		//m_pMainFrame->m_pProcessLogView->UpdateData(false);
		bool bRelst = TranslateTensileData();
		m_sTensileTransData.clear();
		return bRelst;
	}
	else if (SystemParController::getInstance()->m_sysParm.m_sysCollect.m_iForceRecType == 1 && Context::getInstance()->m_isSave)
	{
		int length = m_sTensileTransData.size();
		QString sForce = "";
		for (int i = 0; i < length; i++)
		{
			if (i < length - 9 && m_sTensileTransData.at(i) == 'E' && m_sTensileTransData.at(i + 1) == '2')
			{
				sForce = m_sTensileTransData.mid(i + 2, 8);
				break;
			}
		}
		if (sForce != "")
		{
			QByteArray ba = sForce.toLatin1();
			bool ok = false;
			int iForce = ba.toInt(&ok, 16);
			double dForce = iForce / 1000000.0 / 100.0 * SystemParController::getInstance()->m_sysParm.m_sysCollect.m_dForceRange;
			if (ok)
			{
				std::lock_guard<std::mutex> lockGuard(Context::getInstance()->m_mutexSerialComm);
				Context::getInstance()->m_mForceMap[m_iImageIndex] = dForce;
				Context::getInstance()->m_mStatusMap[m_iImageIndex] = DeviceDataStruct::COMDATA_STATUS_ENABLE;
			}
		}
		m_sTensileTransData.clear();
		return true;
	}
	return false;
}

bool CComportControl::PushFLCComChar()
{
	//if (!m_bFLCComOpen)//未打开，或者当前的Port与系统port口不相同
	//	return false;

	//char temp[256] = { 0 };
	//int iLen = sio_read(m_curFLCComD.iComPort, temp, 256);
	//if (iLen <= 0)
	//{
	//	return false;
	//}
	//for (int i = 0; i < iLen; i++)
	//{
	//	QString strTemp;
	//	int iTemp = 0;
	//	memcpy(&iTemp, &temp[i], 1);
	//	strTemp = QString("%1").arg(iTemp, 2, 16, QLatin1Char('0')).toUpper();
	//	m_sFLCTransData += strTemp;
	//}

	//if (m_sFLCTransData.left(2) == "24" && m_sFLCTransData.right(4) == "0D0A")
	//{
	//	QString strNum = QString("%1---").arg(QString::number(m_sFLCTransData.length()));
	//	//m_pMainFrame->m_pProcessLogView->AddLog(strNum + m_sFLCTransData);
	//	//m_pMainFrame->m_pProcessLogView->UpdateData(false);
	//	bool bRelst = TranslateFLCData();
	//	m_sFLCTransData.clear();
	//	return bRelst;
	//}
	return false;
}

bool CComportControl::TranslateTensileData()
{
	QString strHeader = m_sTensileTransData.mid(2, 2);//第二个字符串
	if (strHeader == "00")//开始自动采集
	{
		if (ProjectController::getInstance()->m_staticWndManager.m_pCameraControlView->m_bExternalControl)//接受外部指令
		{
			if (Context::getInstance()->m_iPageIndex == 1)
			{
				QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_UPDATE_CONNECT_STATUS, 1, 0));
				QApplication::postEvent(Context::getInstance()->getCameraControlView(), new UpdateEvent(MESSAGE_UPDATE_STARTSNAP));
			}
			else
			{
				MessageBox(NULL, _T("当前窗口非控制界面！"), _T("提示"), MB_OK);
			}
		}
	}
	else if (strHeader == "FF")//停止采集
	{
		if (ProjectController::getInstance()->m_staticWndManager.m_pCameraControlView->m_bExternalControl)//接受外部指令
		{
			QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_UPDATE_CONNECT_STATUS, 0, 0));
			QApplication::postEvent(Context::getInstance()->getCameraControlView(), new UpdateEvent(MESSAGE_UPDATE_STOPSNAP));
		}
	}
	else if (strHeader == "01")//数据区
	{
		if (m_iImageIndex >= 0)
		{
			std::lock_guard<std::mutex> lockGuard(Context::getInstance()->m_mutexSerialComm);
			// 获得力信号
			QString strValue = m_sTensileTransData.mid(4, 16);
			QString strTemp = ASCDecode(strValue);
			Context::getInstance()->m_mForceMap[m_iImageIndex] = GetData(strTemp);

			// 位移
			strValue = m_sTensileTransData.mid(20, 16);
			strTemp = ASCDecode(strValue);
			Context::getInstance()->m_mDispMap[m_iImageIndex] = GetData(strTemp);

			// 小变形
			strValue = m_sTensileTransData.mid(36, 16);
			strTemp = ASCDecode(strValue);
			Context::getInstance()->m_mSmallDeformMap[m_iImageIndex] = GetData(strTemp);

			// 大变形
			strValue = m_sTensileTransData.mid(52, 16);
			strTemp = ASCDecode(strValue);
			Context::getInstance()->m_mBigDeformMap[m_iImageIndex] = GetData(strTemp);

			// 时间
			strValue = m_sTensileTransData.mid(84, 16);
			strTemp = ASCDecode(strValue);
			Context::getInstance()->m_mTimeMap[m_iImageIndex] = GetData(strTemp);

			Context::getInstance()->m_mStatusMap[m_iImageIndex] = DeviceDataStruct::COMDATA_STATUS_ENABLE;
		}
	}
	m_sTensileTransData.clear();
	return true;
}

bool CComportControl::TranslateFLCData()
{
	QString strHeader = m_sTensileTransData.mid(2, 2);//第二个字符串
	if (strHeader == "00")//开始自动采集
	{
		if (ProjectController::getInstance()->m_staticWndManager.m_pCameraControlView->m_bExternalControl)//接受外部指令
		{
			if (Context::getInstance()->m_iPageIndex == 1)
			{
				QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_UPDATE_CONNECT_STATUS, 1, 0));
				QApplication::postEvent(Context::getInstance()->getCameraControlView(), new UpdateEvent(MESSAGE_UPDATE_STARTSNAP));
			}
			else
			{
				MessageBox(NULL, _T("当前窗口非控制界面！"), _T("提示"), MB_OK);
			}
		}
	}
	else if (strHeader == "FF")//停止采集
	{
		if (ProjectController::getInstance()->m_staticWndManager.m_pCameraControlView->m_bExternalControl)//接受外部指令
		{
			QApplication::postEvent(Context::getInstance()->getMain(), new MyEvent(MESSAGE_UPDATE_CONNECT_STATUS, 0, 0));
			QApplication::postEvent(Context::getInstance()->getCameraControlView(), new UpdateEvent(MESSAGE_UPDATE_STOPSNAP));
		}
	}
	else if (strHeader == "01")//数据区
	{
		StageDataStruct::STAGE* pStage = NULL;
		if (ProjectController::getInstance()->m_mainDataManager.GetDataAt(DATATYPE_STAGE, m_iProjectType, CDataIndex(m_iProjectIndex), m_iImageIndex, (LPVOID&)pStage))//获得状态
		{
			// 获得力信号
			QString strValue = m_sTensileTransData.mid(4, 16);
			QString strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dForce = GetData(strTemp);

			// 压边力
			strValue = m_sTensileTransData.mid(20, 16);
			strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dForceBHF = GetData(strTemp);

			// 杯突值
			strValue = m_sTensileTransData.mid(36, 16);
			strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dSmallDeform = GetData(strTemp);

			// 位移
			strValue = m_sTensileTransData.mid(52, 16);
			strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dBigDeform = GetData(strTemp);

			// 速度
			strValue = m_sTensileTransData.mid(68, 16);
			strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dSpeed = GetData(strTemp);

			// 时间
			strValue = m_sTensileTransData.mid(84, 16);
			strTemp = ASCDecode(strValue);
			pStage->CollectData.ComData.dTime = GetData(strTemp);

			pStage->CollectData.ComData.iStatus = DeviceDataStruct::COMDATA_STATUS_ENABLE;
		}
	}
	m_sTensileTransData.clear();
	return true;
}

QString CComportControl::ASCDecode(QString& strData)
{
	QString strtmp, strd;
	strd = "";
	TCHAR data[2];
	int nLength = strData.length() / 2;
	for (int i = 0; i < nLength; i++)
	{
		strtmp = strData.mid(2 * i, 2);
		if (strtmp[0] != "3")
		{
			Str2Hex_t(strtmp, 2, data);
			strd += data[0];
		}
		else
		{
			strd += strtmp[1];
		}
	}
	return strd;
}

int  CComportControl::Str2Hex(const char* str, int nLen, char* data)
{
	int t, t1;
	int ilen = 0;
	for (int i = 0; i < nLen;)
	{
		char l, h = str[i];
		if (h == ' ')
		{
			i++;
			continue;
		}
		i++;
		if (i >= nLen)
			break;
		l = str[i];
		t = ProjectController::getInstance()->m_fileFun._HexChar(h);
		t1 = ProjectController::getInstance()->m_fileFun._HexChar(l);
		if ((t == 16) || (t1 == 16))
			break;
		else
			t = t * 16 + t1;
		i++;
		data[ilen] = (char)t;
		ilen++;
	}
	return ilen;
}

int  CComportControl::Str2Hex_t(const QString str, int nLen, TCHAR* data)
{
	int t, t1;
	int ilen = 0;
	for (int i = 0; i < nLen;)
	{
		QString l, h = str[i];
		if (h == " ")
		{
			i++;
			continue;
		}
		i++;
		if (i >= nLen)
			break;
		l = str[i];
		t = ProjectController::getInstance()->m_fileFun._HexFromQString(h);
		t1 = ProjectController::getInstance()->m_fileFun._HexFromQString(l);
		if ((t == 16) || (t1 == 16))
			break;
		else
			t = t * 16 + t1;
		i++;
		data[ilen] = (TCHAR)t;
		ilen++;
	}
	return ilen;
}

QString CComportControl::HexToBin(QString& strHex)
{
	int iLen = strHex.length();
	QString strResult = "";
	for (int i = 0; i < iLen; i++)
	{
		int hex = strHex.mid(i, 1).toInt(0, 16);
		switch (hex)
		{
		case 0:
			strResult += "0000";
			break;
		case 1:
			strResult += "0001";
			break;
		case 2:
			strResult += "0010";
			break;
		case 3:
			strResult += "0011";
			break;
		case 4:
			strResult += "0100";
			break;
		case 5:
			strResult += "0101";
			break;
		case 6:
			strResult += "0110";
			break;
		case 7:
			strResult += "0111";
			break;
		case 8:
			strResult += "1000";
			break;
		case 9:
			strResult += "1001";
			break;
		case 10:
			strResult += "1010";
			break;
		case 11:
			strResult += "1011";
			break;
		case 12:
			strResult += "1100";
			break;
		case 13:
			strResult += "1101";
			break;
		case 14:
			strResult += "1110";
			break;
		case 15:
			strResult += "1111";
			break;
		default:
			break;
		}
	}
	return strResult;
}

double CComportControl::BinToDec(QString& strBin, long nIndex)
{
	double dRe = 0;
	int iLen = strBin.length();
	for (int i = 0; i < iLen; i++)
	{
		if (strBin[i] == "0")
			continue;
		else
		{
			double dk = 1;
			dk = dk * pow(2.0, (int)(nIndex - i));
			dRe += dk;
		}
	}
	return dRe;
}

double CComportControl::GetData(QString& strHex)
{
	QString strBin;
	strBin = HexToBin(strHex);
	QString strSign, strIndex, strData;
	strSign = strBin.left(1);
	strIndex = strBin.mid(1, 8);
	int nIndex = int(BinToDec(strIndex, strIndex.length() - 1) - 127);
	strData = "1" + strBin.right(strBin.length() - 9);
	double dRe = BinToDec(strData, nIndex);
	if (strSign == "1")
	{
		dRe = -1 * dRe;
	}
	return dRe;
}

string CComportControl::dec2hex(int i, int width)
{
	stringstream ioss; //定义字符串流 
	string s_temp; //存放转化后字符
	ioss << hex << i; //以十六制形式输出 
	ioss >> s_temp;
	if (width > s_temp.size()) {
		string s_0(width - s_temp.size(), '0'); //位数不够则补0                 
		s_temp = s_0 + s_temp; //合并
	}
	string s = s_temp.substr(s_temp.length() - width, s_temp.length()); //取右width位 
	return s;
}

unsigned char* CComportControl::CRC16(unsigned char* pArray, unsigned int iLen)
{
	unsigned int  IX, IY, CRC;
	unsigned char Rcvbuf[2] = { 0 };

	if (iLen <= 0)
		return 0;


	CRC = 0xFFFF;
	for (IX = 0; IX < iLen; IX++)		//需要处理的数据
	{
		CRC = CRC ^ (unsigned int)(pArray[IX]);
		for (IY = 0; IY <= 7; IY++)	// 处理1个字节
		{
			if ((CRC & 1) != 0)
				CRC = (CRC >> 1) ^ 0xA001;
			else
				CRC = CRC >> 1;
		}
	}

	Rcvbuf[0] = (CRC & 0x00ff); 		//低位
	Rcvbuf[1] = (CRC & 0xff00) >> 8; 	//高位

	//CRC = Rcvbuf[0] << 8;
	//CRC += Rcvbuf[1];
	return Rcvbuf;
}
