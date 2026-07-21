#include "VoltageBox.h"
#include "./ART_DAQ/ART_ArtDAQ.h"
#include "./ART_NET5640/ART_ARTNET5640.h"
#include "./NI_DAQ/NI_DAQ.h"

class DAQFactory
{
public:
	static ARTBase* CreateART(ArtType type)
	{
		ARTBase* m_pArt = NULL;

		switch (type)
		{
		case UNKNOWN:
			assert(false);
			break;
		case ART3134A:
			m_pArt = new CART_ArtDAQ();
			break;
		case ART5640_NET:
			m_pArt = new ART_ARTNET5640();
			break;
		case NI_:
			m_pArt = new NI_DAQ();
			break;
		default:
			assert(false);
			break;
		}

		return m_pArt;
	}
};


Voltage::Voltage()
{
	m_mArtDevice = nullptr;
	m_artType = ART3134A;
	//m_bOpen = false;
	for (int i = 0; i < 16; i++)
	{
		m_dValueK[i] = 1;
		m_dValueB[i] = 0;
	}
	m_bReadAD = false;
}

Voltage::~Voltage()
{
	//if (m_bOpen)//å¦‚æžœæŽ§åˆ¶ç®±å¤„äºŽæ‰“å¼€æ¨¡å¼�ï¼Œåˆ™å…³é—­
	//{
	CloseCtrlBox();
	//}
}

void Voltage::RegisterListener(CARTListener* listener)
{
	if (m_mArtDevice)
	{
		m_mArtDevice->RegisterListener(listener);
	}
}

void Voltage::SetBoxType(const ArtType& iType)
{
	m_artType = iType;
}

// æ‰“å¼€æŽ§åˆ¶ç®?
bool Voltage::OpenCtrlBox()
{
	// å¦‚æžœå·²ç»�æ‰“å¼€ï¼Œåˆ™å…ˆå…³é—?
	if (m_mArtDevice)
	{
		CloseCtrlBox();
	}
	m_mArtDevice = DAQFactory::CreateART(m_artType);
	if (!m_mArtDevice)
	{
		return false;
	}
	return true;
}
// å…³é—­æŽ§åˆ¶ç®?
bool Voltage::CloseCtrlBox()
{
	if (m_mArtDevice)
	{
		m_mArtDevice->Release();
		delete m_mArtDevice;
		m_mArtDevice = nullptr;
	}

	return true;
}

bool Voltage::ChangeCtrlBox()
{
	CloseCtrlBox();
	OpenCtrlBox();

	return true;
}


bool Voltage::ReadADData(double* pdData, double* pGainData, double* truthData, bool bVoltInputExe[], double dCompensation[], bool bGetVolt)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	bool bUsed = false;
	for (int i = 0; i < 16; i++)
	{
		if (bVoltInputExe[i])
		{
			bUsed = true;
			break;
		}
	}
	if (!bUsed)
	{
		return false;
	}
	if (!m_mArtDevice->ReadDeviceAD(pGainData, bVoltInputExe))
	{
		return false;
	}

	if (bGetVolt)
	{
		return true;
	}
	for (int i = 0; i < 16; i++)
	{
		truthData[i] = pGainData[i];
		if (!bVoltInputExe[i])
		{
			continue;
		}
		else
		{
			///pGainData[i] += dCompensation[i];
			pdData[i] = m_dValueK[i] * pGainData[i] + m_dValueB[i];
		}
	}
	return true;
}

bool Voltage::WriteDAData(double* pdData, bool bVoltOutputExe[])
{
	bool bUsed = false;
	for (int i = 0; i < DACHANNEL_SIZE; i++)
	{
		if (bVoltOutputExe[i])
		{
			bUsed = true;
			break;
		}
	}
	if (!bUsed || !m_mArtDevice)
	{
		return false;
	}
	if (!m_mArtDevice->VolOut(pdData, bVoltOutputExe))
	{
		return false;
	}

	return true;
}

bool Voltage::ReadCtrlAD(double* pdData)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReadCtrlAD(pdData);
}

bool Voltage::CreateADTask(uint32_t artIndex, bool* bOpen, double* dVolt)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->CreateADTask(artIndex, bOpen, dVolt);;
}

bool Voltage::CreateDATask(uint32_t artIndex, bool* bOpen, double* dVolt)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->CreateDATask(artIndex, bOpen, dVolt);
}

bool Voltage::CreateCITask()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->CreateCITask();
}

bool Voltage::CreateDITask(uint32_t artIndex)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->CreateDITask(artIndex);
}

bool Voltage::ReleaseADTask()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReleaseADTask();
}

bool Voltage::ReleaseDATask()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReleaseDATask();
}

bool Voltage::ReleaseHardTrigger()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReleaseHardTriggerTask();
}

bool Voltage::ReleaseCITask()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReleaseCITask();;
}

bool Voltage::ReleaseDITask()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReleaseDITask();
}

bool Voltage::HardTrigger(double dValue, double dutyCycle)
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->HardTrigger(dValue, dutyCycle);
}

bool Voltage::ReadCIFreq()
{
	if (!m_mArtDevice)
	{
		return false;
	}
	return m_mArtDevice->ReadCIFreq();
}
