#define _CRT_SECURE_NO_WARNINGS
#include "BonDriverProxyEx.h"

#if _DEBUG
#define DETAILLOG	0
#define DETAILLOG2	1
#endif

#ifdef HAVE_UI
#define WM_TASKTRAY			(WM_USER + 1)
#define ID_TASKTRAY			0
#define ID_TASKTRAY_SHOW	1
#define ID_TASKTRAY_HIDE	2
#define ID_TASKTRAY_RELOAD	3
#define ID_TASKTRAY_EXIT	4
HINSTANCE g_hInstance;
HWND g_hWnd;
HMENU g_hMenu;
#endif

#ifdef USE_B25_DECODER_DLL
int B25DecoderAdapter::strip        = 1;
int B25DecoderAdapter::emm_proc     = 0;
int B25DecoderAdapter::multi2_round = 4;
using B25Decoder = B25DecoderAdapter;
#endif // USE_B25_DECODER_DLL

static int g_b25_enable;
// �A�C�R���̖��O���i�[���邽�߂̃o�b�t�@
char szIconName[256];
wchar_t wszIconName[256];
// �O���[�o���ϐ�
TCHAR g_szAppName[MAX_PATH];

static int Init(HMODULE hModule)
{
	char szIniPath[MAX_PATH + 16] = { '\0' };
	GetModuleFileNameA(hModule, szIniPath, MAX_PATH);
	char *p = strrchr(szIniPath, '.');
	if (!p)
		return -1;
	p++;
	strcpy(p, "ini");

	HANDLE hFile = CreateFileA(szIniPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return -2;
	CloseHandle(hFile);

	GetPrivateProfileStringA("OPTION", "ADDRESS", "127.0.0.1", g_Host, sizeof(g_Host), szIniPath);
	GetPrivateProfileStringA("OPTION", "PORT", "1192", g_Port, sizeof(g_Port), szIniPath);
	g_OpenTunerRetDelay = GetPrivateProfileIntA("OPTION", "OPENTUNER_RETURN_DELAY", 0, szIniPath);
	g_SandBoxedRelease = GetPrivateProfileIntA("OPTION", "SANDBOXED_RELEASE", 0, szIniPath);
	g_DisableUnloadBonDriver = GetPrivateProfileIntA("OPTION", "DISABLE_UNLOAD_BONDRIVER", 0, szIniPath);
	g_b25_enable = GetPrivateProfileIntA("OPTION", "B25", 0, szIniPath);
	// INI�t�@�C������A�C�R���̖��O���擾
	GetPrivateProfileStringA("OPTION", "ICON", "BDPEX_ICON_DEFAULT", szIconName, sizeof(szIconName), szIniPath);
	MultiByteToWideChar(CP_ACP, 0, szIconName, -1, wszIconName, sizeof(wszIconName) / sizeof(wszIconName[0]));

	if (g_b25_enable)
	{
		B25Decoder::strip = GetPrivateProfileIntA("OPTION", "STRIP", 1, szIniPath);
		B25Decoder::emm_proc = GetPrivateProfileIntA("OPTION", "EMM", 0, szIniPath);
		B25Decoder::multi2_round = GetPrivateProfileIntA("OPTION", "MULTI2ROUND", 4, szIniPath);
	}

	g_PacketFifoSize = GetPrivateProfileIntA("SYSTEM", "PACKET_FIFO_SIZE", 64, szIniPath);
	g_TsPacketBufSize = GetPrivateProfileIntA("SYSTEM", "TSPACKET_BUFSIZE", (188 * 1024), szIniPath);

	char szPriority[128];
	GetPrivateProfileStringA("SYSTEM", "PROCESSPRIORITY", "NORMAL", szPriority, sizeof(szPriority), szIniPath);
	if (strcmp(szPriority, "REALTIME") == 0)
		g_ProcessPriority = REALTIME_PRIORITY_CLASS;
	else if (strcmp(szPriority, "HIGH") == 0)
		g_ProcessPriority = HIGH_PRIORITY_CLASS;
	else if (strcmp(szPriority, "ABOVE_NORMAL") == 0)
		g_ProcessPriority = ABOVE_NORMAL_PRIORITY_CLASS;
	else if (strcmp(szPriority, "BELOW_NORMAL") == 0)
		g_ProcessPriority = BELOW_NORMAL_PRIORITY_CLASS;
	else if (strcmp(szPriority, "IDLE") == 0)
		g_ProcessPriority = IDLE_PRIORITY_CLASS;
	else
		g_ProcessPriority = NORMAL_PRIORITY_CLASS;
	SetPriorityClass(GetCurrentProcess(), g_ProcessPriority);

	GetPrivateProfileStringA("SYSTEM", "THREADPRIORITY_TSREADER", "NORMAL", szPriority, sizeof(szPriority), szIniPath);
	if (strcmp(szPriority, "CRITICAL") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_TIME_CRITICAL;
	else if (strcmp(szPriority, "HIGHEST") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_HIGHEST;
	else if (strcmp(szPriority, "ABOVE_NORMAL") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_ABOVE_NORMAL;
	else if (strcmp(szPriority, "BELOW_NORMAL") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_BELOW_NORMAL;
	else if (strcmp(szPriority, "LOWEST") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_LOWEST;
	else if (strcmp(szPriority, "IDLE") == 0)
		g_ThreadPriorityTsReader = THREAD_PRIORITY_IDLE;
	else
		g_ThreadPriorityTsReader = THREAD_PRIORITY_NORMAL;

	GetPrivateProfileStringA("SYSTEM", "THREADPRIORITY_SENDER", "NORMAL", szPriority, sizeof(szPriority), szIniPath);
	if (strcmp(szPriority, "CRITICAL") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_TIME_CRITICAL;
	else if (strcmp(szPriority, "HIGHEST") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_HIGHEST;
	else if (strcmp(szPriority, "ABOVE_NORMAL") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_ABOVE_NORMAL;
	else if (strcmp(szPriority, "BELOW_NORMAL") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_BELOW_NORMAL;
	else if (strcmp(szPriority, "LOWEST") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_LOWEST;
	else if (strcmp(szPriority, "IDLE") == 0)
		g_ThreadPrioritySender = THREAD_PRIORITY_IDLE;
	else
		g_ThreadPrioritySender = THREAD_PRIORITY_NORMAL;
{
	// [OPTION]
	// BONDRIVER=PT-T
	// [BONDRIVER]
	// 00=PT-T;BonDriver_PT3-T0.dll;BonDriver_PT3-T1.dll
	// 01=PT-S;BonDriver_PT3-S0.dll;BonDriver_PT3-S1.dll
	// DIR_PATH=C:\BonDriverProxyEx\BonDriver

	int cntD, cntT = 0; // �h���C�o�J�E���g�̏�����
	size_t dirPathLen, bufLen = 0U; // �p�X���ƃo�b�t�@���̏�����
	char *str, *strPath, *pos, *pp[MAX_DRIVERS], **ppDriver; // ������|�C���^�̐錾
	char tag[4], buf[MAX_PATH * 4]; // �^�O�ƃo�b�t�@�̐錾
	char lastChr = '\0'; // �Ō�̕����̏�����

	// ini�t�@�C������DIR_PATH���擾
	GetPrivateProfileStringA("BONDRIVER", "DIR_PATH", "", g_DriverDir, sizeof(g_DriverDir), szIniPath);
	dirPathLen = strlen(g_DriverDir);
	if (dirPathLen)
	{
		lastChr = g_DriverDir[dirPathLen - 1];
		if (lastChr != '\\' && lastChr != '/')
		{
			if (dirPathLen + 1 >= sizeof(g_DriverDir))
			{
				g_DriverDir[0] = '\0';
				dirPathLen = 0;
			}
			else
			{
				g_DriverDir[dirPathLen] = '\\';
				++dirPathLen;
			}
		}
	}

	// �h���C�o�̓ǂݍ��݃��[�v
	for (int i = 0; i < MAX_DRIVERS; i++)
	{
		// �^�O�̐���
		tag[0] = (char)('0' + (i / 10));
		tag[1] = (char)('0' + (i % 10));
		tag[2] = '\0';

		// ini�t�@�C������h���C�o�����擾
		GetPrivateProfileStringA("BONDRIVER", tag, "", buf, sizeof(buf), szIniPath);
		if (buf[0] == '\0')
		{
			g_ppDriver[cntT] = NULL;
			break;
		}

		// �h���C�o���̉��
		// format: GroupName;BonDriver1;BonDriver2;BonDriver3...
		// e.g.  : PT-T;BonDriver_PT3-T0.dll;BonDriver_PT3-T1.dll
		bufLen = strlen(buf);
		str = new char[bufLen + 1];
		strcpy(str, buf);
		pos = pp[0] = str;
		cntD = 1;
		for (;;)
		{
			p = strchr(pos, ';');
			if (p)
			{
				*p = '\0';
				pos = pp[cntD++] = p + 1;
				if (cntD > (MAX_DRIVERS - 1))
					break;
			}
			else
				break;
		}
		if (cntD == 1)
		{
			delete[] str;
			continue;
		}
		else if (dirPathLen)
		{
			pp[cntD] = NULL;
			strPath = new char[(dirPathLen * (cntD - 1)) + bufLen + 1];
			strcpy(strPath, str);
			pp[0] = strPath;
			p = &strPath[strlen(strPath)];
			pos = pp[1];
			for (int j = 1; pos != NULL;)
			{
				++p;
				memcpy(p, g_DriverDir, dirPathLen);
				memcpy(p + dirPathLen, pos, strlen(pos) + 1);
				pp[j] = p;
				p += strlen(p);
				pos = pp[++j];
			}
			delete[] str;
		}
		ppDriver = g_ppDriver[cntT++] = new char *[cntD];
		memcpy(ppDriver, pp, sizeof(char *) * cntD);
		std::vector<stDriver> &vstDriver = DriversMap.emplace(ppDriver[0], cntD - 1).first->second;
		for (int j = 1; j < cntD; j++)
		{
			vstDriver[j - 1].strBonDriver = ppDriver[j];
			vstDriver[j - 1].hModule = NULL;
			vstDriver[j - 1].bUsed = FALSE;
		}
	}
}


#if _DEBUG && DETAILLOG2
	for (int i = 0; i < MAX_DRIVERS; i++)
	{
		if (g_ppDriver[i] == NULL)
			break;
		_RPT2(_CRT_WARN, "%02d: %s\n", i, g_ppDriver[i][0]);
		std::vector<stDriver> &v = DriversMap.at(g_ppDriver[i][0]);
		for (size_t j = 0; j < v.size(); j++)
			_RPT1(_CRT_WARN, "  : %s\n", v[j].strBonDriver);
	}
#endif

	OSVERSIONINFOEXA osvi = {};
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	osvi.dwMajorVersion = 6;	// >= Vista
	if (VerifyVersionInfoA(&osvi, VER_MAJORVERSION, VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL)))
		g_ThreadExecutionState = ES_SYSTEM_REQUIRED | ES_CONTINUOUS | ES_AWAYMODE_REQUIRED;
	else
		g_ThreadExecutionState = ES_SYSTEM_REQUIRED | ES_CONTINUOUS;

	return 0;
}

#if defined(HAVE_UI) || defined(BUILD_AS_SERVICE)
static void ShutdownInstances()
{
	// �V���b�g�_�E���C�x���g�g���K
	if (!g_ShutdownEvent.IsSet())
		g_ShutdownEvent.Set();

	// �܂��҂��󂯃X���b�h�̏I����҂�
	g_Lock.Enter();
	if (g_hListenThread != NULL)
	{
		WaitForSingleObject(g_hListenThread, INFINITE);
		CloseHandle(g_hListenThread);
		g_hListenThread = NULL;
	}
	g_Lock.Leave();

	// �S�N���C�A���g�C���X�^���X�̏I����҂�
	for (;;)
	{
		// g_InstanceList�̐��m�F�ł킴�킴���b�N���Ă�̂́AcProxyServerEx�C���X�^���X��
		// "���X�g����͍폜����Ă��Ă��f�X�g���N�^���I�����Ă��Ȃ�"��Ԃ�r�������
		g_Lock.Enter();
		size_t num = g_InstanceList.size();
		g_Lock.Leave();
		if (num == 0)
			break;
		Sleep(10);
	}

	// �V���b�g�_�E���C�x���g�N���A
	g_ShutdownEvent.Reset();
}
#endif

static void CleanUp()
{
	// �h���C�o�̃N���[���A�b�v����
	for (int i = 0; i < MAX_DRIVERS; i++)
	{
		if (g_ppDriver[i] == NULL)
			break;
		// �h���C�o�̃��X�g���擾
		std::vector<stDriver> &v = DriversMap.at(g_ppDriver[i][0]);
		for (size_t j = 0; j < v.size(); j++)
		{
			if (v[j].hModule != NULL)
				FreeLibrary(v[j].hModule); // ���[�h���ꂽ���W���[�������
		}
		delete[] g_ppDriver[i][0]; // �h���C�o���̃����������
		delete[] g_ppDriver[i]; // �h���C�o���X�g�̃����������
		g_ppDriver[i] = NULL;
	}
	DriversMap.clear(); // �h���C�o�}�b�v���N���A
}

cProxyServerEx::cProxyServerEx() : m_Error(TRUE, FALSE)
{
	// �����o�ϐ��̏�����
	m_s = INVALID_SOCKET;
	m_hModule = NULL;
	m_pIBon = m_pIBon2 = m_pIBon3 = NULL;
	m_bTunerOpen = FALSE;
	m_bChannelLock = 0;
	m_hTsRead = NULL;
	m_pTsReaderArg = NULL;
	m_dwSpace = m_dwChannel = 0x7fffffff; // INT_MAX
	m_pDriversMapKey = NULL;
	m_iDriverNo = -1;
	m_iDriverUseOrder = 0;
}

cProxyServerEx::~cProxyServerEx()
{
	LOCK(g_Lock); // �O���[�o�����b�N���擾
	BOOL bRelease = TRUE;
	std::list<cProxyServerEx *>::iterator it = g_InstanceList.begin();
	while (it != g_InstanceList.end())
	{
		if (*it == this)
			g_InstanceList.erase(it++); // �C���X�^���X���X�g���玩�g���폜
		else
		{
			if ((m_hModule != NULL) && (m_hModule == (*it)->m_hModule))
				bRelease = FALSE; // ���̃C���X�^���X���������W���[�����g�p���Ă���ꍇ
			++it;
		}
	}
	if (bRelease)
	{
		if (m_hTsRead)
		{
			m_pTsReaderArg->StopTsRead = TRUE;
			::WaitForSingleObject(m_hTsRead, INFINITE);
			::CloseHandle(m_hTsRead);
			delete m_pTsReaderArg;
		}

		Release(); // ���\�[�X�̉��

		if (m_hModule)
		{
			std::vector<stDriver> &vstDriver = DriversMap.at(m_pDriversMapKey);
			vstDriver[m_iDriverNo].bUsed = FALSE;
			if (!g_DisableUnloadBonDriver)
			{
				::FreeLibrary(m_hModule); // ���W���[�������
				vstDriver[m_iDriverNo].hModule = NULL;
			}
		}
	}
	else
	{
		if (m_hTsRead)
			StopTsReceive(); // TS��M���~
	}
	if (m_s != INVALID_SOCKET)
		::closesocket(m_s); // �\�P�b�g�����
}

DWORD WINAPI cProxyServerEx::Reception(LPVOID pv)
{
	cProxyServerEx *pProxy = static_cast<cProxyServerEx *>(pv);

	// ������COM���g�p���Ă���BonDriver�ɑ΂���΍�
	HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	// �ڑ��N���C�A���g������Ԃ̓X���[�v�}�~
	EXECUTION_STATE es = ::SetThreadExecutionState(g_ThreadExecutionState);

	// �v���L�V�T�[�o�[�̏��������s
	DWORD ret = pProxy->Process();
	delete pProxy; // �v���L�V�T�[�o�[�I�u�W�F�N�g���폜

#ifdef HAVE_UI
	// UI���L���ȏꍇ�A�E�B���h�E���ĕ`��
	::InvalidateRect(g_hWnd, NULL, TRUE);
#endif

	// �X���b�h���s��Ԃ����ɖ߂�
	if (es != NULL)
		::SetThreadExecutionState(es);

	// COM���C�u�����̏�����������
	if (SUCCEEDED(hr))
		::CoUninitialize();

	return ret;
}

DWORD cProxyServerEx::Process()
{
	HANDLE hThread[2];
	hThread[0] = ::CreateThread(NULL, 0, cProxyServerEx::Sender, this, 0, NULL);
	if (hThread[0] == NULL)
		return 1;
	::SetThreadPriority(hThread[0], g_ThreadPrioritySender);

	hThread[1] = ::CreateThread(NULL, 0, cProxyServerEx::Receiver, this, 0, NULL);
	if (hThread[1] == NULL)
	{
		m_Error.Set();
		::WaitForSingleObject(hThread[0], INFINITE);
		::CloseHandle(hThread[0]);
		return 2;
	}

	HANDLE h[3] = { m_Error, m_fifoRecv.GetEventHandle(), g_ShutdownEvent };
	for (;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(3, h, FALSE, INFINITE);
		switch (dwRet)
		{
		case WAIT_OBJECT_0:
			goto end;

		case WAIT_OBJECT_0 + 1:
		{
			// �R�}���h�����̑S�̂����b�N����̂ŁABonDriver_Proxy�����[�h���Ď������g��
			// �ڑ�������ƃf�b�h���b�N����
			// �������������Ȃ���΍���󋵂ƌ����̂͑��������Ǝv���̂ŁA����͎d�l�ƌ�������
			LOCK(g_Lock);
			cPacketHolder *pPh;
			m_fifoRecv.Pop(&pPh);
#if _DEBUG && DETAILLOG2
			{
				char *CommandName[]={
					"eSelectBonDriver",
					"eCreateBonDriver",
					"eOpenTuner",
					"eCloseTuner",
					"eSetChannel1",
					"eGetSignalLevel",
					"eWaitTsStream",
					"eGetReadyCount",
					"eGetTsStream",
					"ePurgeTsStream",
					"eRelease",

					"eGetTunerName",
					"eIsTunerOpening",
					"eEnumTuningSpace",
					"eEnumChannelName",
					"eSetChannel2",
					"eGetCurSpace",
					"eGetCurChannel",

					"eGetTotalDeviceNum",
					"eGetActiveDeviceNum",
					"eSetLnbPower",

					"eGetClientInfo",
				};
				if (pPh->GetCommand() <= eGetClientInfo)
				{
					_RPT2(_CRT_WARN, "Recieve Command : [%s] / this[%p]\n", CommandName[pPh->GetCommand()], this);
				}
				else
				{
					_RPT2(_CRT_WARN, "Illegal Command : [%d] / this[%p]\n", (int)(pPh->GetCommand()), this);
				}
			}
#endif
			switch (pPh->GetCommand())
			{
			case eSelectBonDriver:
			{
				if (pPh->GetBodyLength() <= sizeof(char))
					makePacket(eSelectBonDriver, FALSE);
				else
				{
					char *p;
					if ((p = ::strrchr((char *)(pPh->m_pPacket->payload), ':')) != NULL)
					{
						if (::strcmp(p, ":desc") == 0)	// �~��
						{
							*p = '\0';
							m_iDriverUseOrder = 1;
						}
						else if (::strcmp(p, ":asc") == 0)	// ����
							*p = '\0';
					}
					BOOL b = SelectBonDriver((LPCSTR)(pPh->m_pPacket->payload), 0);
					if (b)
						g_InstanceList.push_back(this);
					makePacket(eSelectBonDriver, b);
				}
				break;
			}

			case eCreateBonDriver:
			{
				if (m_pIBon == NULL)
				{
					BOOL bFind = FALSE;
					for (auto pInstance : g_InstanceList)
					{
						if (pInstance == this)
							continue;
						if (m_hModule == pInstance->m_hModule)
						{
							if (pInstance->m_pIBon != NULL)
							{
								bFind = TRUE;	// �����ɗ���̂͂��Ȃ�̃��A�P�[�X�̃n�Y
								m_pIBon = pInstance->m_pIBon;
								m_pIBon2 = pInstance->m_pIBon2;
								m_pIBon3 = pInstance->m_pIBon3;
								break;
							}
							// �����ɗ���̂͏���X�Ƀ��A�P�[�X
							// �ꉞ���X�g�̍Ō�܂Ō������Ă݂āA����ł�������Ȃ�������
							// CreateBonDriver()����点�Ă݂�
						}
					}
					if (!bFind)
					{
						if ((CreateBonDriver() != NULL) && (m_pIBon2 != NULL))
							makePacket(eCreateBonDriver, TRUE);
						else
						{
							makePacket(eCreateBonDriver, FALSE);
							m_Error.Set();
						}
					}
					else
						makePacket(eCreateBonDriver, TRUE);
				}
				else
					makePacket(eCreateBonDriver, TRUE);
				break;
			}

			case eOpenTuner:
			{
				BOOL bFind = FALSE;
				{
					for (auto pInstance : g_InstanceList)
					{
						if (pInstance == this)
							continue;
						if ((m_pIBon != NULL) && (m_pIBon == pInstance->m_pIBon))
						{
							if (pInstance->m_bTunerOpen)
							{
								bFind = TRUE;
								m_bTunerOpen = TRUE;
								break;
							}
						}
					}
				}
				if (!bFind)
					m_bTunerOpen = OpenTuner();
				makePacket(eOpenTuner, m_bTunerOpen);
				break;
			}

			case eCloseTuner:
			{
				BOOL bFind = FALSE;
				for (auto pInstance : g_InstanceList)
				{
					if (pInstance == this)
						continue;
					if ((m_pIBon != NULL) && (m_pIBon == pInstance->m_pIBon))
					{
						if (pInstance->m_bTunerOpen)
						{
							bFind = TRUE;
							break;
						}
					}
				}
				if (!bFind)
				{
					if (m_hTsRead)
					{
						m_pTsReaderArg->StopTsRead = TRUE;
						::WaitForSingleObject(m_hTsRead, INFINITE);
						::CloseHandle(m_hTsRead);
						delete m_pTsReaderArg;
					}
					CloseTuner();
				}
				else
				{
					if (m_hTsRead)
						StopTsReceive();
				}
				m_bChannelLock = 0;
				m_dwSpace = m_dwChannel = 0x7fffffff;	// INT_MAX
				m_hTsRead = NULL;
				m_pTsReaderArg = NULL;
				m_bTunerOpen = FALSE;
				break;
			}

			case ePurgeTsStream:
			{
				if (m_hTsRead)
				{
					m_pTsReaderArg->TsLock.Enter();
					if (m_pTsReaderArg->TsReceiversList.size() <= 1)
					{
						PurgeTsStream();
						m_pTsReaderArg->pos = 0;
					}
#if _DEBUG && DETAILLOG2
					_RPT2(_CRT_WARN, "ePurgeTsStream : [%d] / size[%zu]\n", (m_pTsReaderArg->TsReceiversList.size() <= 1) ? 1 : 0, m_pTsReaderArg->TsReceiversList.size());
#endif
					m_pTsReaderArg->TsLock.Leave();
					makePacket(ePurgeTsStream, TRUE);
				}
				else
					makePacket(ePurgeTsStream, FALSE);
				break;
			}

			case eRelease:
				m_Error.Set();
				break;

			case eEnumTuningSpace:
			{
				if (pPh->GetBodyLength() != sizeof(DWORD))
					makePacket(eEnumTuningSpace, _T(""));
				else
				{
					LPCTSTR p = EnumTuningSpace(::ntohl(*(DWORD *)(pPh->m_pPacket->payload)));
					if (p)
						makePacket(eEnumTuningSpace, p);
					else
						makePacket(eEnumTuningSpace, _T(""));
				}
				break;
			}

			case eEnumChannelName:
			{
				if (pPh->GetBodyLength() != (sizeof(DWORD) * 2))
					makePacket(eEnumChannelName, _T(""));
				else
				{
					LPCTSTR p = EnumChannelName(::ntohl(*(DWORD *)(pPh->m_pPacket->payload)), ::ntohl(*(DWORD *)&(pPh->m_pPacket->payload[sizeof(DWORD)])));
					if (p)
						makePacket(eEnumChannelName, p);
					else
						makePacket(eEnumChannelName, _T(""));
				}
				break;
			}

			case eSetChannel2:
			{
				if (pPh->GetBodyLength() != ((sizeof(DWORD) * 2) + sizeof(BYTE)))
					makePacket(eSetChannel2, (DWORD)0xff);
				else
				{
					BYTE bChannelLock = pPh->m_pPacket->payload[sizeof(DWORD) * 2];
					DWORD dwReqSpace = ::ntohl(*(DWORD *)(pPh->m_pPacket->payload));
					DWORD dwReqChannel = ::ntohl(*(DWORD *)&(pPh->m_pPacket->payload[sizeof(DWORD)]));
					if ((dwReqSpace == m_dwSpace) && (dwReqChannel == m_dwChannel))
					{
						// ���Ƀ��N�G�X�g���ꂽ�`�����l����I�Ǎς�
#if _DEBUG && DETAILLOG2
						_RPT2(_CRT_WARN, "** already tuned! ** : m_dwSpace[%d] / m_dwChannel[%d]\n", dwReqSpace, dwReqChannel);
#endif
						// �K��true�̃n�Y�����ǁA�ꉞ
						if (m_hTsRead)
						{
							// ���̃C���X�^���X���v�����Ă���D��x��255�ł������ꍇ��
							if (bChannelLock == 0xff)
							{
								// ���݂̔z�M���X�g�ɂ͗D��x255�̃C���X�^���X�����ɂ��邩�H
								BOOL bFind = FALSE;
								m_pTsReaderArg->TsLock.Enter();
								for (auto pReceiver : m_pTsReaderArg->TsReceiversList)
								{
									if ((pReceiver != this) && (pReceiver->m_bChannelLock == 0xff))
									{
										bFind = TRUE;
										break;
									}
								}
								m_pTsReaderArg->TsLock.Leave();
								if (bFind)
								{
									// �����ꍇ�́A���̃C���X�^���X�̗D��x���b��I��254�ɂ���
									bChannelLock = 0xfe;
									// �r�����擾�҂����X�g�ɂ܂����g���܂܂�Ă��Ȃ���Βǉ�
									bFind = FALSE;
									for (auto pPriv : m_pTsReaderArg->WaitExclusivePrivList)
									{
										if (pPriv == this)
										{
											bFind = TRUE;
											break;
										}
									}
									if (!bFind)
										m_pTsReaderArg->WaitExclusivePrivList.push_back(this);
#if _DEBUG && DETAILLOG2
									_RPT2(_CRT_WARN, "** exclusive tuner! ** : wait-exclusivepriv-list size[%zu] / added[%d]\n", m_pTsReaderArg->WaitExclusivePrivList.size(), bFind ? 0 : 1);
#endif
								}
							}
						}
						m_bChannelLock = bChannelLock;
						makePacket(eSetChannel2, (DWORD)0x00);
					}
					else
					{
						BOOL bSuccess;
						BOOL bLocked = FALSE;
						BOOL bShared = FALSE;
						BOOL bSetChannel = FALSE;
						cProxyServerEx *pHavePriv = NULL;
						for (auto pInstance1 : g_InstanceList)
						{
							if (pInstance1 == this)
								continue;
							// �ЂƂ܂����݂̃C���X�^���X�����L����Ă��邩�ǂ������m�F���Ă���
							if ((m_pIBon != NULL) && (m_pIBon == pInstance1->m_pIBon))
								bShared = TRUE;

							// �Ώ�BonDriver�Q�̒��Ń`���[�i���I�[�v�����Ă������
							if (m_pDriversMapKey == pInstance1->m_pDriversMapKey && pInstance1->m_pIBon != NULL && pInstance1->m_bTunerOpen)
							{
								// ���N���C�A���g����̗v���Ɠ���`�����l����I�����Ă������
								if (pInstance1->m_dwSpace == dwReqSpace && pInstance1->m_dwChannel == dwReqChannel)
								{
									// ���N���C�A���g���I�[�v�����Ă���`���[�i�Ɋւ���
									if (m_pIBon != NULL)
									{
										BOOL bModule = FALSE;
										BOOL bIBon = FALSE;
										BOOL bTuner = FALSE;
										for (auto pInstance2 : g_InstanceList)
										{
											if (pInstance2 == this)
												continue;
											if (m_hModule == pInstance2->m_hModule)
											{
												bModule = TRUE;	// ���W���[���g�p�җL��
												if (m_pIBon == pInstance2->m_pIBon)
												{
													bIBon = TRUE;	// �C���X�^���X�g�p�җL��
													if (pInstance2->m_bTunerOpen)
													{
														bTuner = TRUE;	// �`���[�i�g�p�җL��
														break;
													}
												}
											}
										}

										// �`���[�i�g�p�Җ����Ȃ�N���[�Y
										if (!bTuner)
										{
											if (m_hTsRead)
											{
												m_pTsReaderArg->StopTsRead = TRUE;
												::WaitForSingleObject(m_hTsRead, INFINITE);
												::CloseHandle(m_hTsRead);
												//m_hTsRead = NULL;
												delete m_pTsReaderArg;
												//m_pTsReaderArg = NULL;
											}
											CloseTuner();
											//m_bTunerOpen = FALSE;
											// ���C���X�^���X�g�p�҂������Ȃ�C���X�^���X�����[�X
											if (!bIBon)
											{
												Release();
												// m_pIBon = NULL;
												// �����W���[���g�p�҂������Ȃ烂�W���[�������[�X
												if (!bModule)
												{
													std::vector<stDriver> &vstDriver = DriversMap.at(m_pDriversMapKey);
													vstDriver[m_iDriverNo].bUsed = FALSE;
													if (!g_DisableUnloadBonDriver)
													{
														::FreeLibrary(m_hModule);
														vstDriver[m_iDriverNo].hModule = NULL;
													}
													// m_hModule = NULL;
												}
											}
										}
										else	// ���Ƀ`���[�i�g�p�җL��̏ꍇ
										{
											// ����TS�X�g���[���z�M���Ȃ炻�̔z�M�Ώۃ��X�g���玩�g���폜
											if (m_hTsRead)
												StopTsReceive();
										}
									}

									// ���̃C���X�^���X���v�����Ă���D��x��255�ł������ꍇ��
									if (bChannelLock == 0xff)
									{
										// �؂�ւ���`���[�i�ɑ΂��ėD��x255�̃C���X�^���X�����ɂ��邩�H
										for (auto pInstance2 : g_InstanceList)
										{
											if (pInstance2 == this)
												continue;
											if (pInstance2->m_pIBon == pInstance1->m_pIBon)
											{
												if (pInstance2->m_bChannelLock == 0xff)
												{
													// �����ꍇ�́A���̃C���X�^���X�̗D��x���b��I��254�ɂ���
													// (�������Ȃ��ƁA�D��x255�̃C���X�^���X���`�����l���ύX�ł��Ȃ��Ȃ��)
													bChannelLock = 0xfe;
													pHavePriv = pInstance2;
													break;
												}
											}
										}
									}

									// �C���X�^���X�؂�ւ�
									m_hModule = pInstance1->m_hModule;
									m_iDriverNo = pInstance1->m_iDriverNo;
									m_pIBon = pInstance1->m_pIBon;
									m_pIBon2 = pInstance1->m_pIBon2;
									m_pIBon3 = pInstance1->m_pIBon3;
									m_bTunerOpen = TRUE;
									m_hTsRead = pInstance1->m_hTsRead;	// ���̎��_�ł�NULL�̉\���̓[���ł͂Ȃ�
									m_pTsReaderArg = pInstance1->m_pTsReaderArg;
									if (m_hTsRead)
									{
										m_pTsReaderArg->TsLock.Enter();
										m_pTsReaderArg->TsReceiversList.push_back(this);
										m_pTsReaderArg->TsLock.Leave();
									}
#if _DEBUG && DETAILLOG2
									_RPT3(_CRT_WARN, "** found! ** : m_hModule[%p] / m_iDriverNo[%d] / m_pIBon[%p]\n", m_hModule, m_iDriverNo, m_pIBon);
									_RPT3(_CRT_WARN, "             : m_dwSpace[%d] / m_dwChannel[%d] / m_bChannelLock[%d]\n", dwReqSpace, dwReqChannel, bChannelLock);
#endif
									// ���̃C���X�^���X�̗D��x��������ꂽ�ꍇ
									if (pHavePriv != NULL)
									{
										if (m_hTsRead)
										{
											// �r�����擾�҂����X�g�ɂ܂����g���܂܂�Ă��Ȃ���Βǉ�
											BOOL bFind = FALSE;
											for (auto pPriv : m_pTsReaderArg->WaitExclusivePrivList)
											{
												if (pPriv == this)
												{
													bFind = TRUE;
													break;
												}
											}
											if (!bFind)
												m_pTsReaderArg->WaitExclusivePrivList.push_back(this);
#if _DEBUG && DETAILLOG2
											_RPT2(_CRT_WARN, "** exclusive tuner! ** : wait-exclusivepriv-list size[%zu] / added[%d]\n", m_pTsReaderArg->WaitExclusivePrivList.size(), bFind ? 0 : 1);
#endif
										}
										else
										{
											// ���̏����̈Ӑ}�͏������̓��������̃R�����g�Q��
											pHavePriv->m_bChannelLock = 0;
											bChannelLock = 0xff;
										}
									}
									goto ok;	// ����͍���
								}
							}
						}

						// ����`�����l�����g�p���̃`���[�i�͌����炸�A���݂̃`���[�i�͋��L����Ă�����
						if (bShared)
						{
							// �o����Ζ��g�p�A�����Ȃ�Ȃ�ׂ����b�N����ĂȂ��`���[�i��I�����āA
							// ��C�Ƀ`���[�i�I�[�v����Ԃɂ܂Ŏ����čs��
							if (SelectBonDriver(m_pDriversMapKey, bChannelLock))
							{
								if (m_pIBon == NULL)
								{
									// ���g�p�`���[�i��������
									if ((CreateBonDriver() == NULL) || (m_pIBon2 == NULL))
									{
										makePacket(eSetChannel2, (DWORD)0xff);
										m_Error.Set();
										break;
									}
								}
								if (!m_bTunerOpen)
								{
									m_bTunerOpen = OpenTuner();
									if (!m_bTunerOpen)
									{
										makePacket(eSetChannel2, (DWORD)0xff);
										m_Error.Set();
										break;
									}
								}
							}
							else
							{
								makePacket(eSetChannel2, (DWORD)0xff);
								m_Error.Set();
								break;
							}

							// �g�p�`���[�i�̃`�����l�����b�N��Ԋm�F
							for (auto pInstance : g_InstanceList)
							{
								if (pInstance == this)
									continue;
								if (m_pIBon == pInstance->m_pIBon)
								{
									if (pInstance->m_bChannelLock > bChannelLock)
										bLocked = TRUE;
									else if (pInstance->m_bChannelLock == 0xff)
									{
										// �Ώۃ`���[�i�ɑ΂��ėD��x255�̃C���X�^���X�����ɂ����ԂŁA���̃C���X�^���X��
										// �v�����Ă���D��x��255�̏ꍇ�A���̃C���X�^���X�̗D��x���b��I��254�ɂ���
										// (�������Ȃ��ƁA�D��x255�̃C���X�^���X���`�����l���ύX�ł��Ȃ��Ȃ��)
										bChannelLock = 0xfe;
										bLocked = TRUE;
										pHavePriv = pInstance;
									}
									if (bLocked)
										break;
								}
							}
							// ���̃C���X�^���X�̗D��x��������ꂽ�ꍇ
							if (pHavePriv != NULL)
							{
								if (m_hTsRead)
								{
									// �r�����擾�҂����X�g�ɂ܂����g���܂܂�Ă��Ȃ���Βǉ�
									BOOL bFind = FALSE;
									for (auto pPriv : m_pTsReaderArg->WaitExclusivePrivList)
									{
										if (pPriv == this)
										{
											bFind = TRUE;
											break;
										}
									}
									if (!bFind)
										m_pTsReaderArg->WaitExclusivePrivList.push_back(this);
#if _DEBUG && DETAILLOG2
									_RPT2(_CRT_WARN, "** exclusive tuner! ** : wait-exclusivepriv-list size[%zu] / added[%d]\n", m_pTsReaderArg->WaitExclusivePrivList.size(), bFind ? 0 : 1);
#endif
								}
								else
								{
									// ���̃C���X�^���X�̗D��x��������ꂽ���A�r�����������Ă���C���X�^���X�ւ̔z�M��
									// �J�n����Ă��Ȃ��ꍇ�́A���̃C���X�^���X����r������D��
									// �������鎖�������Ƃ��Ė]�܂����̂��ǂ����͔��������A�������������ɗ���̂́A
									// ���Y�C���X�^���X�ł�SetChannel()�̎��s��A���������ɐڑ����������Ă����Ԃł���A
									// �\���Ƃ��Ă̓[���ł͂Ȃ����̂́A���Ȃ�̃��A�P�[�X�Ɍ�����͂�
									pHavePriv->m_bChannelLock = 0;
									bChannelLock = 0xff;
								}
							}
						}

#if _DEBUG && DETAILLOG2
						_RPT2(_CRT_WARN, "eSetChannel2 : bShared[%d] / bLocked[%d]\n", bShared, bLocked);
						_RPT3(_CRT_WARN, "             : dwReqSpace[%d] / dwReqChannel[%d] / m_bChannelLock[%d]\n", dwReqSpace, dwReqChannel, bChannelLock);
#endif

						if (bLocked)
						{
							// ���b�N����Ă鎞�͒P���Ƀ��b�N����Ă鎖��ʒm
							// ���̏ꍇ�N���C�A���g�A�v���ւ�SetChannel()�̖߂�l�͐����ɂȂ�
							// (�����炭�v���I�Ȗ��ɂ͂Ȃ�Ȃ�)
							makePacket(eSetChannel2, (DWORD)0x01);
						}
						else
						{
							if (m_hTsRead)
								m_pTsReaderArg->TsLock.Enter();
							bSuccess = SetChannel(dwReqSpace, dwReqChannel);
							if (m_hTsRead)
							{
								// ��U���b�N���O���ƃ`�����l���ύX�O�̃f�[�^�����M����Ȃ�����ۏ؂ł��Ȃ��Ȃ�ׁA
								// �`�����l���ύX�O�̃f�[�^�̔j����CNR�̍X�V�w���͂����ōs��
								if (bSuccess)
								{
									// ����`�����l�����g�p���̃`���[�i��������Ȃ������ꍇ�́A���̃��N�G�X�g��
									// �C���X�^���X�̐؂�ւ����������Ă����Ƃ��Ă��A���̎��_�ł͂ǂ����`�����l����
									// �ύX����Ă���̂ŁA�����M�o�b�t�@��j�����Ă��ʂɖ��ɂ͂Ȃ�Ȃ��n�Y
									m_pTsReaderArg->pos = 0;
									m_pTsReaderArg->ChannelChanged = TRUE;
								}
								m_pTsReaderArg->TsLock.Leave();
							}
							if (bSuccess)
							{
								bSetChannel = TRUE;
							ok:
								m_dwSpace = dwReqSpace;
								m_dwChannel = dwReqChannel;
								m_bChannelLock = bChannelLock;
								makePacket(eSetChannel2, (DWORD)0x00);
								if (m_hTsRead == NULL)
								{
									m_pTsReaderArg = new stTsReaderArg();
									m_pTsReaderArg->TsReceiversList.push_back(this);
									m_pTsReaderArg->pIBon = m_pIBon;
									const BOOL bDesiredUseB25 = static_cast<BOOL>(pPh->m_pPacket->head.m_bReserved1 & eDesireToUseB25);
									if (bDesiredUseB25 && g_b25_enable)
									{
#ifdef USE_B25_DECODER_DLL
										if (m_pTsReaderArg->b25.load() == FALSE)
											g_b25_enable = FALSE;
#else
										m_pTsReaderArg->b25.init();
#endif // USE_B25_DECODER_DLL
									}
									m_pTsReaderArg->bB25Enable = bDesiredUseB25 && g_b25_enable;
									m_hTsRead = ::CreateThread(NULL, 0, cProxyServerEx::TsReader, m_pTsReaderArg, 0, NULL);
									if (m_hTsRead == NULL)
									{
										delete m_pTsReaderArg;
										m_pTsReaderArg = NULL;
										m_Error.Set();
									}
									else
										::SetThreadPriority(m_hTsRead, g_ThreadPriorityTsReader);
								}
								if (bSetChannel)
								{
									// SetChannel()���s��ꂽ�ꍇ�́A����BonDriver�C���X�^���X���g�p���Ă���C���X�^���X�̕ێ��`�����l����ύX
									for (auto pInstance : g_InstanceList)
									{
										if (pInstance == this)
											continue;
										if (m_pIBon == pInstance->m_pIBon)
										{
											pInstance->m_dwSpace = dwReqSpace;
											pInstance->m_dwChannel = dwReqChannel;
											// �ΏۃC���X�^���X���܂���x��SetChannel()���s���Ă��Ȃ������ꍇ
											if (pInstance->m_hTsRead == NULL)
											{
												// �����I�ɔz�M�J�n
												pInstance->m_bTunerOpen = TRUE;
												pInstance->m_hTsRead = m_hTsRead;
												pInstance->m_pTsReaderArg = m_pTsReaderArg;
												if (m_hTsRead)
												{
													m_pTsReaderArg->TsLock.Enter();
													m_pTsReaderArg->TsReceiversList.push_back(pInstance);
													m_pTsReaderArg->TsLock.Leave();
												}
											}
										}
									}
								}
							}
							else
								makePacket(eSetChannel2, (DWORD)0xff);
						}
					}
#ifdef HAVE_UI
					::InvalidateRect(g_hWnd, NULL, TRUE);
#endif
				}
				break;
			}

			case eGetTotalDeviceNum:
				makePacket(eGetTotalDeviceNum, GetTotalDeviceNum());
				break;

			case eGetActiveDeviceNum:
				makePacket(eGetActiveDeviceNum, GetActiveDeviceNum());
				break;

			case eSetLnbPower:
			{
				if (pPh->GetBodyLength() != sizeof(BYTE))
					makePacket(eSetLnbPower, FALSE);
				else
					makePacket(eSetLnbPower, SetLnbPower((BOOL)(pPh->m_pPacket->payload[0])));
				break;
			}

			case eGetClientInfo:
			{
				union {
					SOCKADDR_STORAGE ss;
					SOCKADDR_IN si4;
					SOCKADDR_IN6 si6;
				};
				char addr[INET6_ADDRSTRLEN], buf[2048], info[4096], *p, *exinfo;
				int port, len, num = 0;
				size_t left, size;
				p = info;
				p[0] = '\0';
				left = size = sizeof(info);
				exinfo = NULL;
				for (auto pInstance : g_InstanceList)
				{
					len = sizeof(ss);
					if (::getpeername(pInstance->m_s, (SOCKADDR *)&ss, &len) == 0)
					{
						if (ss.ss_family == AF_INET)
						{
							// IPv4
							::inet_ntop(AF_INET, &(si4.sin_addr), addr, sizeof(addr));
							port = ::ntohs(si4.sin_port);
						}
						else
						{
							// IPv6
							::inet_ntop(AF_INET6, &(si6.sin6_addr), addr, sizeof(addr));
							port = ::ntohs(si6.sin6_port);
						}
					}
					else
					{
						::lstrcpyA(addr, "unknown host...");
						port = 0;
					}
					std::vector<stDriver> &vstDriver = DriversMap.at(pInstance->m_pDriversMapKey);
					len = ::wsprintfA(buf, "%02d: [%s]:[%d] / [%s][%s] / space[%u] ch[%u] / Lock[%d]\n", num, addr, port, pInstance->m_pDriversMapKey, vstDriver[pInstance->m_iDriverNo].strBonDriver, pInstance->m_dwSpace, pInstance->m_dwChannel, pInstance->m_bChannelLock);
					if ((size_t)len >= left)
					{
						left += size;
						size *= 2;
						if (exinfo != NULL)
						{
							char *bp = exinfo;
							exinfo = new char[size];
							::lstrcpyA(exinfo, bp);
							delete[] bp;
						}
						else
						{
							exinfo = new char[size];
							::lstrcpyA(exinfo, info);
						}
						p = exinfo + ::lstrlenA(exinfo);
					}
					::lstrcpyA(p, buf);
					p += len;
					left -= len;
					num++;
				}
				if (exinfo != NULL)
				{
					size = (p - exinfo) + 1;
					p = exinfo;
				}
				else
				{
					size = (p - info) + 1;
					p = info;
				}
				cPacketHolder *ph = new cPacketHolder(eGetClientInfo, size);
				::memcpy(ph->m_pPacket->payload, p, size);
				m_fifoSend.Push(ph);
				if (exinfo != NULL)
					delete[] exinfo;
				break;
			}

			default:
				break;
			}
			delete pPh;
			break;
		}

		case WAIT_OBJECT_0 + 2:
			// �I���v��
			// fall-through
		default:
			// �����̃G���[
			m_Error.Set();
			goto end;
		}
	}
end:
	::WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	::CloseHandle(hThread[0]);
	::CloseHandle(hThread[1]);
	return 0;
}

int cProxyServerEx::ReceiverHelper(char *pDst, DWORD left)
{
	int len, ret;
	fd_set rd;
	timeval tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	while (left > 0)
	{
		if (m_Error.IsSet())
			return -1;

		FD_ZERO(&rd);
		FD_SET(m_s, &rd);
		if ((len = ::select((int)(m_s + 1), &rd, NULL, NULL, &tv)) == SOCKET_ERROR)
		{
			ret = -2;
			goto err;
		}

		if (len == 0)
			continue;

		// MSDN��recv()�̃\�[�X��Ƃ��������A"SOCKET_ERROR"�����̒l�Ȃ͕̂ۏ؂���Ă���ۂ�
		if ((len = ::recv(m_s, pDst, left, 0)) <= 0)
		{
			ret = -3;
			goto err;
		}
		left -= len;
		pDst += len;
	}
	return 0;
err:
	m_Error.Set();
	return ret;
}

DWORD WINAPI cProxyServerEx::Receiver(LPVOID pv)
{
	cProxyServerEx *pProxy = static_cast<cProxyServerEx *>(pv);
	char *p;
	DWORD left, ret;
	cPacketHolder *pPh = NULL;

	for (;;)
	{
		pPh = new cPacketHolder(16);
		left = sizeof(stPacketHead);
		p = (char *)&(pPh->m_pPacket->head);
		if (pProxy->ReceiverHelper(p, left) != 0)
		{
			ret = 201;
			goto end;
		}

		if (!pPh->IsValid())
		{
			pProxy->m_Error.Set();
			ret = 202;
			goto end;
		}

		left = pPh->GetBodyLength();
		if (left == 0)
		{
			pProxy->m_fifoRecv.Push(pPh);
			continue;
		}

		if (left > 16)
		{
			if (left > 512)
			{
				pProxy->m_Error.Set();
				ret = 203;
				goto end;
			}
			cPacketHolder *pTmp = new cPacketHolder(left);
			pTmp->m_pPacket->head = pPh->m_pPacket->head;
			delete pPh;
			pPh = pTmp;
		}

		p = (char *)(pPh->m_pPacket->payload);
		if (pProxy->ReceiverHelper(p, left) != 0)
		{
			ret = 204;
			goto end;
		}

		pProxy->m_fifoRecv.Push(pPh);
	}
end:
	delete pPh;
	return ret;
}

void cProxyServerEx::makePacket(enumCommand eCmd, BOOL b)
{
	cPacketHolder *p = new cPacketHolder(eCmd, sizeof(BYTE));
	p->m_pPacket->payload[0] = (BYTE)b;
	m_fifoSend.Push(p);
}

void cProxyServerEx::makePacket(enumCommand eCmd, DWORD dw)
{
	cPacketHolder *p = new cPacketHolder(eCmd, sizeof(DWORD));
	DWORD *pos = (DWORD *)(p->m_pPacket->payload);
	*pos = ::htonl(dw);
	m_fifoSend.Push(p);
}

void cProxyServerEx::makePacket(enumCommand eCmd, LPCTSTR str)
{
	register size_t size = (::_tcslen(str) + 1) * sizeof(TCHAR);
	cPacketHolder *p = new cPacketHolder(eCmd, size);
	::memcpy(p->m_pPacket->payload, str, size);
	m_fifoSend.Push(p);
}

void cProxyServerEx::makePacket(enumCommand eCmd, BYTE *pSrc, DWORD dwSize, float fSignalLevel)
{
	register size_t size = (sizeof(DWORD) * 2) + dwSize;
	cPacketHolder *p = new cPacketHolder(eCmd, size);
	union {
		DWORD dw;
		float f;
	} u;
	u.f = fSignalLevel;
	DWORD *pos = (DWORD *)(p->m_pPacket->payload);
	*pos++ = ::htonl(dwSize);
	*pos++ = ::htonl(u.dw);
	if (dwSize > 0)
		::memcpy(pos, pSrc, dwSize);
	m_fifoSend.Push(p);
}

DWORD WINAPI cProxyServerEx::Sender(LPVOID pv)
{
	cProxyServerEx *pProxy = static_cast<cProxyServerEx *>(pv);
	DWORD ret;
	HANDLE h[2] = { pProxy->m_Error, pProxy->m_fifoSend.GetEventHandle() };
	for (;;)
	{
		DWORD dwRet = ::WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (dwRet)
		{
		case WAIT_OBJECT_0:
			ret = 101;
			goto end;

		case WAIT_OBJECT_0 + 1:
		{
			cPacketHolder *pPh;
			pProxy->m_fifoSend.Pop(&pPh);
			int left = (int)pPh->m_Size;
			char *p = (char *)(pPh->m_pPacket);
			while (left > 0)
			{
				int len = ::send(pProxy->m_s, p, left, 0);
				if (len == SOCKET_ERROR)
				{
					pProxy->m_Error.Set();
					break;
				}
				left -= len;
				p += len;
			}
			delete pPh;
			break;
		}

		default:
			// �����̃G���[
			pProxy->m_Error.Set();
			ret = 102;
			goto end;
		}
	}
end:
	return ret;
}

DWORD WINAPI cProxyServerEx::TsReader(LPVOID pv)
{
	stTsReaderArg *pArg = static_cast<stTsReaderArg *>(pv);
	IBonDriver *pIBon = pArg->pIBon;
	std::atomic<BOOL> &StopTsRead = pArg->StopTsRead;
	std::atomic<BOOL> &ChannelChanged = pArg->ChannelChanged;
	DWORD &pos = pArg->pos;
	std::list<cProxyServerEx *> &TsReceiversList = pArg->TsReceiversList;
	cCriticalSection &TsLock = pArg->TsLock;
	DWORD dwSize, dwRemain, now, before = 0;
	float fSignalLevel = 0;
	DWORD ret = 300;
	const DWORD TsPacketBufSize = g_TsPacketBufSize;
	BYTE *pBuf, *pTsBuf = new BYTE[TsPacketBufSize];
#if _DEBUG && DETAILLOG
	DWORD Counter = 0;
#endif

	// ������COM���g�p���Ă���BonDriver�ɑ΂���΍�
	HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
	// TS�ǂݍ��݃��[�v
	while (!StopTsRead)
	{
		dwSize = dwRemain = 0;
		{
			LOCK(TsLock);
			if ((((now = ::GetTickCount()) - before) >= 1000) || ChannelChanged)
			{
				if (pArg->IsB25Enabled() && ChannelChanged)
					pArg->b25.reset();
				fSignalLevel = pIBon->GetSignalLevel();
				before = now;
				ChannelChanged = FALSE;
			}
			if (pIBon->GetTsStream(&pBuf, &dwSize, &dwRemain) && (dwSize != 0))
			{
				if (pArg->IsB25Enabled())
				{
					pArg->b25.decode(pBuf, dwSize, &pBuf, &dwSize);
					if (dwSize == 0)
						goto next;
				}
				if ((pos + dwSize) < TsPacketBufSize)
				{
					::memcpy(&pTsBuf[pos], pBuf, dwSize);
					pos += dwSize;
					if (dwRemain == 0)
					{
						for (auto &&receiver : TsReceiversList)
							receiver->makePacket(eGetTsStream, pTsBuf, pos, fSignalLevel);
#if _DEBUG && DETAILLOG
						_RPT3(_CRT_WARN, "makePacket0() : %u : size[%x] / dwRemain[%d]\n", Counter++, pos, dwRemain);
#endif
						pos = 0;
					}
				}
				else
				{
					DWORD left, dwLen = TsPacketBufSize - pos;
					::memcpy(&pTsBuf[pos], pBuf, dwLen);
					for (auto &&receiver : TsReceiversList)
						receiver->makePacket(eGetTsStream, pTsBuf, TsPacketBufSize, fSignalLevel);
#if _DEBUG && DETAILLOG
					_RPT3(_CRT_WARN, "makePacket1() : %u : size[%x] / dwRemain[%d]\n", Counter++, TsPacketBufSize, dwRemain);
#endif
					left = dwSize - dwLen;
					pBuf += dwLen;
					while (left >= TsPacketBufSize)
					{
						for (auto &&receiver : TsReceiversList)
							receiver->makePacket(eGetTsStream, pBuf, TsPacketBufSize, fSignalLevel);
#if _DEBUG && DETAILLOG
						_RPT2(_CRT_WARN, "makePacket2() : %u : size[%x]\n", Counter++, TsPacketBufSize);
#endif
						left -= TsPacketBufSize;
						pBuf += TsPacketBufSize;
					}
					if (left != 0)
					{
						if (dwRemain == 0)
						{
							for (auto &&receiver : TsReceiversList)
								receiver->makePacket(eGetTsStream, pBuf, left, fSignalLevel);
#if _DEBUG && DETAILLOG
							_RPT3(_CRT_WARN, "makePacket3() : %u : size[%x] / dwRemain[%d]\n", Counter++, left, dwRemain);
#endif
							left = 0;
						}
						else
							::memcpy(pTsBuf, pBuf, left);
					}
					pos = left;
				}
			}
		}
	next:
		if (dwRemain == 0)
			::Sleep(WAIT_TIME);
	}
	if (SUCCEEDED(hr))
		::CoUninitialize();
	delete[] pTsBuf;
	return ret;
}

void cProxyServerEx::StopTsReceive()
{
	// ���̃��\�b�h�͕K���A
	// 1. �O���[�o���ȃC���X�^���X���b�N��
	// 2. ���ATS��M��(m_hTsRead != NULL)
	// ��2�𖞂�����ԂŌĂяo����
	m_pTsReaderArg->TsLock.Enter();
	std::list<cProxyServerEx *>::iterator it = m_pTsReaderArg->TsReceiversList.begin();
	while (it != m_pTsReaderArg->TsReceiversList.end())
	{
		if (*it == this)
		{
			m_pTsReaderArg->TsReceiversList.erase(it);
			break;
		}
		++it;
	}
	m_pTsReaderArg->TsLock.Leave();

	// ���̃C���X�^���X�̓`�����l���r�����������Ă��邩�H
	if (m_bChannelLock == 0xff)
	{
		// �����Ă����ꍇ�́A�r�����擾�҂��̃C���X�^���X�͑��݂��Ă��邩�H
		if (m_pTsReaderArg->WaitExclusivePrivList.size() > 0)
		{
			// ���݂���ꍇ�́A���X�g�擪�̃C���X�^���X�ɔr�����������p���A���X�g����폜
			cProxyServerEx *p = m_pTsReaderArg->WaitExclusivePrivList.front();
			m_pTsReaderArg->WaitExclusivePrivList.pop_front();
			p->m_bChannelLock = 0xff;
		}
	}
	else
	{
		// �����Ă��Ȃ��ꍇ�́A�r�����擾�҂����X�g�Ɏ��g���܂܂�Ă��邩������Ȃ��̂ō폜
		m_pTsReaderArg->WaitExclusivePrivList.remove(this);
	}

	// �������Ō�̎�M�҂������ꍇ�́ATS�z�M�X���b�h����~
	if (m_pTsReaderArg->TsReceiversList.empty())
	{
		m_pTsReaderArg->StopTsRead = TRUE;
		::WaitForSingleObject(m_hTsRead, INFINITE);
		::CloseHandle(m_hTsRead);
		m_hTsRead = NULL;
		delete m_pTsReaderArg;
		m_pTsReaderArg = NULL;
	}
}

BOOL cProxyServerEx::SelectBonDriver(LPCSTR p, BYTE bChannelLock)
{
	char *pKey = NULL;
	std::vector<stDriver> *pvstDriver = NULL;
	for (auto &&pair : DriversMap)
	{
		if (::strcmp(p, pair.first) == 0)
		{
			pKey = pair.first;
			pvstDriver = &(pair.second);
			break;
		}
	}
	if (pvstDriver == NULL)
	{
		m_hModule = NULL;
		return FALSE;
	}

	// ���ݎ������擾���Ă���
	SYSTEMTIME stNow;
	FILETIME ftNow;
	::GetLocalTime(&stNow);
	::SystemTimeToFileTime(&stNow, &ftNow);

	// �܂��g���ĂȂ��̂�T��
	std::vector<stDriver> &vstDriver = *pvstDriver;
	int i;
	if (m_iDriverUseOrder == 0)
		i = 0;
	else
		i = (int)(vstDriver.size() - 1);
	for (;;)
	{
		if (vstDriver[i].bUsed)
			goto next;
		HMODULE hModule;
		if (vstDriver[i].hModule != NULL)
			hModule = vstDriver[i].hModule;
		else
		{
			hModule = ::LoadLibraryA(vstDriver[i].strBonDriver);
			if (hModule == NULL)
				goto next;
			vstDriver[i].hModule = hModule;
		}
		m_hModule = hModule;
		vstDriver[i].bUsed = TRUE;
		vstDriver[i].ftLoad = ftNow;
		m_pDriversMapKey = pKey;
		m_iDriverNo = i;

		// �e�퍀�ڍď������̑O�ɁA����TS�X�g���[���z�M���Ȃ炻�̔z�M�Ώۃ��X�g���玩�g���폜
		if (m_hTsRead)
			StopTsReceive();

		// eSetChannel2������Ă΂��̂ŁA�e�퍀�ڍď�����
		m_pIBon = m_pIBon2 = m_pIBon3 = NULL;
		m_bTunerOpen = FALSE;
		m_hTsRead = NULL;
		m_pTsReaderArg = NULL;
		return TRUE;
	next:
		if (m_iDriverUseOrder == 0)
		{
			if (i >= (int)(vstDriver.size() - 1))
				break;
			i++;
		}
		else
		{
			if (i <= 0)
				break;
			i--;
		}
	}

	// �S���g���Ă���(���邢��LoadLibrary()�o���Ȃ����)�A����C���X�^���X���g�p���Ă���
	// �N���C�A���g�Q�̃`�����l���D��x�̍ő�l���ł��Ⴂ�C���X�^���X��I������
	// ���l�̕��������������ꍇ��BonDriver�̃��[�h����(�������͎g�p�v������)���Â�����D��
	cProxyServerEx *pCandidate = NULL;
	std::vector<cProxyServerEx *> vpCandidate;
	for (auto pInstance1 : g_InstanceList)
	{
		if (pInstance1 == this)
			continue;
		if (pKey == pInstance1->m_pDriversMapKey)	// ���̒i�K�ł͕������r�ł���K�v�͖���
		{
			// ��⃊�X�g�Ɋ��ɓ���Ă���Ȃ�Ȍ�̃`�F�b�N�͕s�v
			for (i = 0; i < (int)vpCandidate.size(); i++)
			{
				if (vpCandidate[i]->m_hModule == pInstance1->m_hModule)
					break;
			}
			if (i != (int)vpCandidate.size())
				continue;
			// �b����
			pCandidate = pInstance1;
			// ���̎b���₪�g�p���Ă���C���X�^���X�̓��b�N����Ă��邩�H
			BOOL bLocked = FALSE;
			for (auto pInstance2 : g_InstanceList)
			{
				if (pInstance2 == this)
					continue;
				if (pCandidate->m_hModule == pInstance2->m_hModule)
				{
					if ((pInstance2->m_bChannelLock > bChannelLock) || (pInstance2->m_bChannelLock == 0xff))	// ���b�N����Ă�
					{
						bLocked = TRUE;
						break;
					}
				}
			}
			if (!bLocked)	// ���b�N����Ă��Ȃ���Ό�⃊�X�g�ɒǉ�
				vpCandidate.push_back(pCandidate);
		}
	}

#if _DEBUG && DETAILLOG2
	_RPT1(_CRT_WARN, "** SelectBonDriver ** : vpCandidate.size[%zd]\n", vpCandidate.size());
#endif

	// ��⃊�X�g����łȂ����(==���b�N����Ă��Ȃ��C���X�^���X���������Ȃ�)
	if (vpCandidate.size() != 0)
	{
		pCandidate = vpCandidate[0];
		// ���ɑI���̗]�n�͂��邩�H
		if (vpCandidate.size() > 1)
		{
			// �ڑ��N���C�A���g�Q�̃`�����l���D��x�̍ő�l���ł��Ⴂ�C���X�^���X��T��
			// �Ȃ��኱��₱�������A���g���r�����������Ă���A���r�����擾�҂��������ꍇ���A
			// ���̃C���X�^���X�͂��̎��_�ł̌�⃊�X�g�Ɋ܂܂�Ă���
			BYTE bGroupMaxPriv, bMinPriv;
			std::vector<BYTE> vbGroupMaxPriv(vpCandidate.size());
			bMinPriv = 0xff;
			for (i = 0; i < (int)vpCandidate.size(); i++)
			{
				bGroupMaxPriv = 0;
				for (auto pInstance : g_InstanceList)
				{
					if (pInstance == this)
						continue;
					if (vpCandidate[i]->m_hModule == pInstance->m_hModule)
					{
						if (pInstance->m_bChannelLock > bGroupMaxPriv)
							bGroupMaxPriv = pInstance->m_bChannelLock;
					}
				}
				vbGroupMaxPriv[i] = bGroupMaxPriv;
				if (bMinPriv > bGroupMaxPriv)
					bMinPriv = bGroupMaxPriv;
			}
			std::vector<cProxyServerEx *> vpCandidate2;
			for (i = 0; i < (int)vpCandidate.size(); ++i)
			{
#if _DEBUG && DETAILLOG2
				_RPT2(_CRT_WARN, "                      : vbGroupMaxPriv[%d] : [%d]\n", i, vbGroupMaxPriv[i]);
#endif
				if (vbGroupMaxPriv[i] == bMinPriv)
					vpCandidate2.push_back(vpCandidate[i]);
			}
#if _DEBUG && DETAILLOG2
			_RPT1(_CRT_WARN, "                      : vpCandidate2.size[%zd]\n", vpCandidate2.size());
#endif
			// eSetChannel2����̌Ăяo���̏ꍇ
			if (m_pIBon)
			{
				for (i = 0; i < (int)vpCandidate2.size(); ++i)
				{
					// ���̎��_�ł̌�⃊�X�g�Ɍ��݂̃C���X�^���X���܂܂�Ă����ꍇ��
					if (m_hModule == vpCandidate2[i]->m_hModule)
					{
						// ���݂̃C���X�^���X���p���g�p
						// �u���l�̕��������������ꍇ��BonDriver�̃��[�h����(�������͎g�p�v������)���Â�����D��v
						// ������Ȃ����ɂȂ�ꍇ�����邪�A�����D��
						vstDriver[m_iDriverNo].ftLoad = ftNow;
						return TRUE;
					}
				}
			}

			// vpCandidate2.size()��1�ȏ�Ȃ͕̂ۏ؂���Ă���
			pCandidate = vpCandidate2[0];
			// �C���X�^���X���̍ő�D��x�̍ŏ��l�������C���X�^���X�œ��l�ł������ꍇ
			if (vpCandidate2.size() > 1)
			{
				// BonDriver�̃��[�h��������ԌÂ��̂�T��
				FILETIME ft = vstDriver[vpCandidate2[0]->m_iDriverNo].ftLoad;
				for (i = 1; i < (int)vpCandidate2.size(); i++)
				{
					if (::CompareFileTime(&ft, &(vstDriver[vpCandidate2[i]->m_iDriverNo].ftLoad)) > 0)
					{
						ft = vstDriver[vpCandidate2[i]->m_iDriverNo].ftLoad;
						pCandidate = vpCandidate2[i];
					}
				}
			}
		}
		else
		{
			// eSetChannel2����̌Ăяo���̏ꍇ
			if (m_pIBon)
			{
				// �B��̌�₪���݂̃C���X�^���X�Ɠ����������ꍇ��
				if (m_hModule == pCandidate->m_hModule)
				{
					// ���݂̃C���X�^���X���p���g�p
					vstDriver[m_iDriverNo].ftLoad = ftNow;
					return TRUE;
				}
			}
		}

		// eSetChannel2����̌Ăяo���̏ꍇ������TS�X�g���[���z�M���������Ȃ�A
		// �C���X�^���X���؂�ւ��̂ŁA���݂̔z�M�Ώۃ��X�g���玩�g���폜
		if (m_pIBon && m_hTsRead)
			StopTsReceive();
	}
	else
	{
		// eSetChannel2����̌Ăяo���̏ꍇ
		if (m_pIBon)
		{
			// ���b�N����Ă��Ȃ��C���X�^���X�����������̂Ō��݂̃C���X�^���X���p���g�p
			vstDriver[m_iDriverNo].ftLoad = ftNow;
			return TRUE;
		}
	}

	// NULL�ł��鎖�͖����n�Y������
	if (pCandidate != NULL)
	{
		m_hModule = pCandidate->m_hModule;
		m_pDriversMapKey = pCandidate->m_pDriversMapKey;
		m_iDriverNo = pCandidate->m_iDriverNo;
		m_pIBon = pCandidate->m_pIBon;	// pCandidate->m_pIBon��NULL�̉\���̓[���ł͂Ȃ�
		m_pIBon2 = pCandidate->m_pIBon2;
		m_pIBon3 = pCandidate->m_pIBon3;
		m_bTunerOpen = pCandidate->m_bTunerOpen;
		m_hTsRead = pCandidate->m_hTsRead;
		m_pTsReaderArg = pCandidate->m_pTsReaderArg;
		m_dwSpace = pCandidate->m_dwSpace;
		m_dwChannel = pCandidate->m_dwChannel;
		// �g�p����BonDriver�̃��[�h����(�g�p�v������)�����ݎ����ōX�V
		vstDriver[m_iDriverNo].ftLoad = ftNow;
	}

	// �I�������C���X�^���X������TS�X�g���[���z�M���Ȃ�A���̔z�M�Ώۃ��X�g�Ɏ��g��ǉ�
	if (m_hTsRead)
	{
		m_pTsReaderArg->TsLock.Enter();
		m_pTsReaderArg->TsReceiversList.push_back(this);
		m_pTsReaderArg->TsLock.Leave();
	}

	return (m_hModule != NULL);
}

IBonDriver *cProxyServerEx::CreateBonDriver()
{
	if (m_hModule)
	{
		IBonDriver *(*f)() = (IBonDriver *(*)())::GetProcAddress(m_hModule, "CreateBonDriver");
		if (f)
		{
			try { m_pIBon = f(); }
			catch (...) {}
			if (m_pIBon)
			{
				m_pIBon2 = dynamic_cast<IBonDriver2 *>(m_pIBon);
				m_pIBon3 = dynamic_cast<IBonDriver3 *>(m_pIBon);
			}
		}
	}
	return m_pIBon;
}

const BOOL cProxyServerEx::OpenTuner(void)
{
	BOOL b = FALSE;
	if (m_pIBon)
		b = m_pIBon->OpenTuner();
	if (g_OpenTunerRetDelay != 0)
		::Sleep(g_OpenTunerRetDelay);
	return b;
}

void cProxyServerEx::CloseTuner(void)
{
	if (m_pIBon)
		m_pIBon->CloseTuner();
}

void cProxyServerEx::PurgeTsStream(void)
{
	if (m_pIBon)
		m_pIBon->PurgeTsStream();
}

void cProxyServerEx::Release(void)
{
	if (m_pIBon)
	{
		if (g_SandBoxedRelease)
		{
			__try { m_pIBon->Release(); }
			__except (EXCEPTION_EXECUTE_HANDLER){}
		}
		else
			m_pIBon->Release();
	}
}

LPCTSTR cProxyServerEx::EnumTuningSpace(const DWORD dwSpace)
{
	LPCTSTR pStr = NULL;
	if (m_pIBon2)
		pStr = m_pIBon2->EnumTuningSpace(dwSpace);
	return pStr;
}

LPCTSTR cProxyServerEx::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	LPCTSTR pStr = NULL;
	if (m_pIBon2)
		pStr = m_pIBon2->EnumChannelName(dwSpace, dwChannel);
	return pStr;
}

const BOOL cProxyServerEx::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	BOOL b = FALSE;
	if (m_pIBon2)
		b = m_pIBon2->SetChannel(dwSpace, dwChannel);
	return b;
}

const DWORD cProxyServerEx::GetTotalDeviceNum(void)
{
	DWORD d = 0;
	if (m_pIBon3)
		d = m_pIBon3->GetTotalDeviceNum();
	return d;
}

const DWORD cProxyServerEx::GetActiveDeviceNum(void)
{
	DWORD d = 0;
	if (m_pIBon3)
		d = m_pIBon3->GetActiveDeviceNum();
	return d;
}

const BOOL cProxyServerEx::SetLnbPower(const BOOL bEnable)
{
	BOOL b = FALSE;
	if (m_pIBon3)
		b = m_pIBon3->SetLnbPower(bEnable);
	return b;
}

#if defined(HAVE_UI) || defined(BUILD_AS_SERVICE)
struct HostInfo{
	char *host;
	char *port;
};
static DWORD WINAPI Listen(LPVOID pv)
{
	HostInfo *phi = static_cast<HostInfo *>(pv);
	char *host = phi->host;
	char *port = phi->port;
	delete phi;
#else
static int Listen(char *host, char *port)
{
#endif
	addrinfo hints, *results, *rp;
	SOCKET lsock[MAX_HOSTS], csock;
	int i, j, nhost, len;
	char *p, *hostbuf, *h[MAX_HOSTS];
	fd_set rd;
	timeval tv;

	hostbuf = new char[strlen(host) + 1];
	strcpy(hostbuf, host);
	nhost = 0;
	p = hostbuf;
	do
	{
		h[nhost++] = p;
		if ((p = strchr(p, ',')) != NULL)
		{
			char *q = p - 1;
			while (*q == ' ' || *q == '\t')
				*q-- = '\0';
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				*p++ = '\0';
		}
		if (nhost >= MAX_HOSTS)
			break;
	} while ((p != NULL) && (*p != '\0'));

	for (i = 0; i < nhost; i++)
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
		if (getaddrinfo(h[i], port, &hints, &results) != 0)
		{
			hints.ai_flags = AI_PASSIVE;
			if (getaddrinfo(h[i], port, &hints, &results) != 0)
			{
				for (j = 0; j < i; j++)
					closesocket(lsock[j]);
				delete[] hostbuf;
				return 1;
			}
		}

		for (rp = results; rp != NULL; rp = rp->ai_next)
		{
			lsock[i] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (lsock[i] == INVALID_SOCKET)
				continue;

			BOOL exclusive = TRUE;
			setsockopt(lsock[i], SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&exclusive, sizeof(exclusive));

			if (bind(lsock[i], rp->ai_addr, (int)(rp->ai_addrlen)) != SOCKET_ERROR)
				break;

			closesocket(lsock[i]);
		}
		freeaddrinfo(results);
		if (rp == NULL)
		{
			for (j = 0; j < i; j++)
				closesocket(lsock[j]);
			delete[] hostbuf;
			return 2;
		}

		if (listen(lsock[i], 4) == SOCKET_ERROR)
		{
			for (j = 0; j <= i; j++)
				closesocket(lsock[j]);
			delete[] hostbuf;
			return 3;
		}
	}
	delete[] hostbuf;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	while (!g_ShutdownEvent.IsSet())
	{
		FD_ZERO(&rd);
		for (i = 0; i < nhost; i++)
			FD_SET(lsock[i], &rd);
		if ((len = select(0/*(int)(max(lsock) + 1)*/, &rd, NULL, NULL, &tv)) == SOCKET_ERROR)
		{
			for (i = 0; i < nhost; i++)
				closesocket(lsock[i]);
			return 4;
		}
		if (len > 0)
		{
			for (i = 0; i < nhost; i++)
			{
				if (FD_ISSET(lsock[i], &rd))
				{
					len--;
					if ((csock = accept(lsock[i], NULL, NULL)) != INVALID_SOCKET)
					{
						cProxyServerEx *pProxy = new cProxyServerEx();
						pProxy->setSocket(csock);
						HANDLE hThread = CreateThread(NULL, 0, cProxyServerEx::Reception, pProxy, 0, NULL);
						if (hThread)
							CloseHandle(hThread);
						else
							delete pProxy;
					}
				}
				if (len == 0)
					break;
			}
		}
	}

	for (i = 0; i < nhost; i++)
		closesocket(lsock[i]);
	return 0;
}

#ifndef BUILD_AS_SERVICE
#ifdef HAVE_UI
void NotifyIcon(int mode)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = g_hWnd;
	nid.uID = ID_TASKTRAY;
	if (mode == 0)
	{
		// ADD
		nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		nid.uCallbackMessage = WM_TASKTRAY;
		nid.hIcon = LoadIcon(g_hInstance, wszIconName);
		lstrcpy(nid.szTip, g_szAppName);
		for (;;)
		{
			if (Shell_NotifyIcon(NIM_ADD, &nid))
				break;	// �o�^����
			if (GetLastError() != ERROR_TIMEOUT)
				break;	// �^�C���A�E�g�ȊO�̃G���[�Ȃ̂Œ��߂�
			Sleep(500);	// ������Ƒ҂��Ă���m�F
			if (Shell_NotifyIcon(NIM_MODIFY, &nid))
				break;	// �o�^�������Ă�
		}
	}
	else
	{
		// DEL
		nid.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT s_iTaskbarRestart;

	switch (iMsg)
	{
	case WM_CREATE:
		s_iTaskbarRestart = RegisterWindowMessage(_T("TaskbarCreated"));
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		ModifyMenu(g_hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_SHOW, _T("���E�B���h�E�\��"));
		ShowWindow(hWnd, SW_HIDE);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDc = BeginPaint(hWnd, &ps);
		TEXTMETRIC tm;
		GetTextMetrics(hDc, &tm);
		union {
			SOCKADDR_STORAGE ss;
			SOCKADDR_IN si4;
			SOCKADDR_IN6 si6;
		};
		char addr[INET6_ADDRSTRLEN];
		char host[NI_MAXHOST];
		int port, len, num = 0;
		char buf[2048];
		g_Lock.Enter();
		for (auto pInstance : g_InstanceList)
		{
			len = sizeof(ss);
			if (getpeername(pInstance->m_s, (SOCKADDR *)&ss, &len) == 0)
			{
				if (ss.ss_family == AF_INET)
				{
					// IPv4
					inet_ntop(AF_INET, &(si4.sin_addr), addr, sizeof(addr));
					port = ntohs(si4.sin_port);
				}
				else
				{
					// IPv6
					inet_ntop(AF_INET6, &(si6.sin6_addr), addr, sizeof(addr));
					port = ntohs(si6.sin6_port);
				}
				// �z�X�g�����擾
				if (getnameinfo((SOCKADDR *)&ss, len, host, sizeof(host), NULL, 0, NI_NAMEREQD) != 0)
				{
					lstrcpyA(host, "None");
				}
			}
			else
			{
				lstrcpyA(addr, "unknown host...");
				lstrcpyA(host, "None");
				port = 0;
			}
			std::vector<stDriver> &vstDriver = DriversMap.at(pInstance->m_pDriversMapKey);
			wsprintfA(buf, "%02d: [%s:%s]:[%d] / Group:[%s] Space[%u] Ch[%u] Lock[%d] / Path:[%s]", num, addr, host, port, pInstance->m_pDriversMapKey, pInstance->m_dwSpace, pInstance->m_dwChannel, pInstance->m_bChannelLock, vstDriver[pInstance->m_iDriverNo].strBonDriver);
			TextOutA(hDc, 5, 5 + (num * tm.tmHeight), buf, lstrlenA(buf));
			num++;
		}
		g_Lock.Leave();
		EndPaint(hWnd, &ps);
		return 0;
	}

	case WM_TASKTRAY:
	{
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(g_hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
			PostMessage(hWnd, WM_NULL, 0, 0);
			return 0;
		}
		break;
	}

	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_TASKTRAY_SHOW:
		{
			ModifyMenu(g_hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_HIDE, _T("���E�B���h�E��\��"));
			ShowWindow(hWnd, SW_SHOW);
			return 0;
		}

		case ID_TASKTRAY_HIDE:
		{
			ModifyMenu(g_hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_SHOW, _T("���E�B���h�E�\��"));
			ShowWindow(hWnd, SW_HIDE);
			return 0;
		}

		case ID_TASKTRAY_RELOAD:
		{
			if (!g_InstanceList.empty())
			{
				if (MessageBox(hWnd, _T("�ڑ����̃N���C�A���g�����݂��Ă��܂��B�ؒf����܂�����낵���ł����H"), _T("Caution"), MB_YESNO) != IDYES)
					return 0;
			}
			ShutdownInstances();
			CleanUp();
			if (Init(g_hInstance) != 0)
			{
				MessageBox(NULL, _T("ini�t�@�C����������܂���B�������ݒu�����̂��ēǂݍ��݂��ĉ������B"), _T("Error"), MB_OK);
				return 0;
			}
			HostInfo *phi = new HostInfo;
			phi->host = g_Host;
			phi->port = g_Port;
			g_hListenThread = CreateThread(NULL, 0, Listen, phi, 0, NULL);
			if (g_hListenThread == NULL)
			{
				delete phi;
				MessageBox(NULL, _T("�҂��󂯃X���b�h�̍쐬�Ɏ��s���܂����B�I�����܂��B"), _T("Error"), MB_OK);
				PostQuitMessage(0);
			}
			else
				MessageBox(hWnd, _T("�ēǂݍ��݂��܂����B"), _T("Info"), MB_OK);
			return 0;
		}

		case ID_TASKTRAY_EXIT:
		{
			if (!g_InstanceList.empty())
			{
				if (MessageBox(hWnd, _T("�ڑ����̃N���C�A���g�����݂��Ă��܂����A��낵���ł����H"), _T("Caution"), MB_YESNO) != IDYES)
					return 0;
			}
			PostQuitMessage(0);
			return 0;
		}
		}
		break;
	}

	default:
		if (iMsg == s_iTaskbarRestart)
			NotifyIcon(0);
		break;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE/*hPrevInstance*/, LPSTR/*lpCmdLine*/, int/*nCmdShow*/)
{
#if _DEBUG
	// �f�o�b�O�p�̃��O�t�@�C�����쐬
	HANDLE hLogFile = CreateFile(_T("BDProxyEx_Debug.log"), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	_CrtMemState ostate, nstate, dstate;
	_CrtMemCheckpoint(&ostate);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, hLogFile);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_ERROR, hLogFile);
	_RPT0(_CRT_WARN, "--- PROCESS_START ---\n");
//  int *p = new int[2];    // ���[�N���o�e�X�g�p
#endif

	// ����������
	if (Init(hInstance) != 0)
	{
		MessageBox(NULL, _T("ini�t�@�C����������܂���B"), _T("Error"), MB_OK);
		return -1;
	}

	// Winsock�̏�����
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		MessageBox(NULL, _T("winsock�̏������Ɏ��s���܂����B"), _T("Error"), MB_OK);
		return -2;
	}

	// �z�X�g���̐ݒ�
	HostInfo *phi = new HostInfo;
	phi->host = g_Host;
	phi->port = g_Port;
	g_hListenThread = CreateThread(NULL, 0, Listen, phi, 0, NULL);
	if (g_hListenThread == NULL)
	{
		delete phi;
		MessageBox(NULL, _T("�҂��󂯃X���b�h�̍쐬�Ɏ��s���܂����B"), _T("Error"), MB_OK);
		return -3;
	}

	MSG msg;
	WNDCLASSEX wndclass;

	// �E�B���h�E�N���X�̐ݒ�
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;

	// ���s�t�@�C���̃p�X���擾
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);

	// �t�@�C�����������擾
	TCHAR* szFileNameOnly = _tcsrchr(szFileName, '\\');
	if (szFileNameOnly)
	{
		szFileNameOnly++; // �o�b�N�X���b�V�����X�L�b�v

		// �g���q���������t�@�C�������擾
		TCHAR* szExtension = _tcsrchr(szFileNameOnly, '.');
		if (szExtension)
		{
			*szExtension = '\0'; // �g���q���폜
		}

		// �O���[�o���ϐ��ɒl���R�s�[
		_tcscpy_s(g_szAppName, _countof(g_szAppName), szFileNameOnly);

		// �E�B���h�E�N���X�̖��O��ݒ�
		wndclass.lpszClassName = szFileNameOnly;
		wndclass.hIcon = LoadIcon(hInstance, wszIconName);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.hIconSm = LoadIcon(hInstance, wszIconName);

		// �E�B���h�E�N���X��o�^
		RegisterClassEx(&wndclass);

		// �E�B���h�E���쐬
		g_hWnd = CreateWindow(szFileNameOnly, szFileNameOnly, WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME, CW_USEDEFAULT, 0, 640, 320, NULL, NULL, hInstance, NULL);
	}

//  ShowWindow(g_hWnd, nCmdShow);
//  UpdateWindow(g_hWnd);

	// �O���[�o���ϐ��̐ݒ�
	g_hInstance = hInstance;
	g_hMenu = CreatePopupMenu();
	InsertMenu(g_hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_SHOW, _T("���E�B���h�E�\��"));
	InsertMenu(g_hMenu, 1, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_RELOAD, _T("ini�ēǂݍ���"));
	InsertMenu(g_hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TASKTRAY_EXIT, _T("�I��"));
	NotifyIcon(0);

	// ���b�Z�[�W���[�v
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// �N���[���A�b�v����
	ShutdownInstances();    // g_hListenThread�͂��̒���CloseHandle()�����
	CleanUp();              // ShutdownInstances()��DriversMap�ɃA�N�Z�X����X���b�h�͖����Ȃ��Ă���͂�

	// �ʒm�A�C�R���̍폜
	NotifyIcon(1);
	DestroyMenu(g_hMenu);

	// Winsock�̃N���[���A�b�v
	WSACleanup();

#if _DEBUG
	// ���������[�N�`�F�b�N
	_CrtMemCheckpoint(&nstate);
	if (_CrtMemDifference(&dstate, &ostate, &nstate))
	{
		_CrtMemDumpStatistics(&dstate);
		_CrtMemDumpAllObjectsSince(&ostate);
	}
	_RPT0(_CRT_WARN, "--- PROCESS_END ---\n");
	CloseHandle(hLogFile);
#endif

	return (int)msg.wParam;
}
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE/*hPrevInstance*/, LPSTR/*lpCmdLine*/, int/*nCmdShow*/)
{
	if (Init(hInstance) != 0)
		return -1;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -2;

	int ret = Listen(g_Host, g_Port);

	{
		// ���Ȃ����ǈꉞ
		LOCK(g_Lock);
		CleanUp();
	}

	WSACleanup();
	return ret;
}
#endif
#else
#include "ServiceMain.cpp"
#endif
