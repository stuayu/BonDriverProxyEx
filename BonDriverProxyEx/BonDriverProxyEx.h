#ifndef __BONDRIVER_PROXYEX_H__
#define __BONDRIVER_PROXYEX_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <process.h>
#include <list>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <typeinfo>
#include <iostream>
#include <string>
#include "Common.h"
#include "IBonDriver3.h"
#ifdef USE_B25_DECODER_DLL
#include "B25Decoder/IB25Decoder.h"
#include "B25Decoder/B25DecoderAdapter.h"
#else
#include "aribb25/B25Decoder.h"
#endif // USE_B25_DECODER_DLL

#define HAVE_UI
#ifdef BUILD_AS_SERVICE
#undef HAVE_UI
#endif

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define WAIT_TIME	10	// GetTsStream()�̌�ŁAdwRemain��0�������ꍇ�ɑ҂���(ms)

////////////////////////////////////////////////////////////////////////////////

#define MAX_HOSTS	8	// listen()�ł���\�P�b�g�̍ő吔
static char g_Host[512];
static char g_Port[8];
static char g_DriverDir[MAX_PATH + 16];
static size_t g_PacketFifoSize;
static DWORD g_TsPacketBufSize;
static DWORD g_OpenTunerRetDelay;
static BOOL g_SandBoxedRelease;
static BOOL g_DisableUnloadBonDriver;
static DWORD g_ProcessPriority;		// �s�v���Ǝv�����Ǖێ����Ă���
static int g_ThreadPriorityTsReader;
static int g_ThreadPrioritySender;
static EXECUTION_STATE g_ThreadExecutionState;

#include "BdpPacket.h"

#define MAX_DRIVERS	64		// �h���C�o�̃O���[�v���ƃO���[�v���̐��̗���
static char **g_ppDriver[MAX_DRIVERS];
struct stDriver {
	char *strBonDriver;
	HMODULE hModule;
	BOOL bUsed;
	FILETIME ftLoad;
};
static std::unordered_map<char *, std::vector<stDriver> > DriversMap;

////////////////////////////////////////////////////////////////////////////////

struct stTsReaderArg {
	IBonDriver *pIBon;
	std::atomic<BOOL> StopTsRead;
	std::atomic<BOOL> ChannelChanged;
	DWORD pos;
	std::list<cProxyServerEx *> TsReceiversList;
	std::list<cProxyServerEx *> WaitExclusivePrivList;
	cCriticalSection TsLock;
#ifdef USE_B25_DECODER_DLL
	B25DecoderAdapter b25;
#else
	B25Decoder b25;
#endif // USE_B25_DECODER_DLL
	BOOL bB25Enable;
	stTsReaderArg()
		: pIBon(NULL)
		, StopTsRead(FALSE)
		, ChannelChanged(TRUE)
		, pos(0)
		, TsReceiversList()
		, WaitExclusivePrivList()
		, TsLock()
		, b25()
		, bB25Enable(FALSE)
	{
	};

	BOOL IsB25Enabled()
	{
#ifdef USE_B25_DECODER_DLL
		return bB25Enable && b25.is_loaded();
#else
		return bB25Enable;
#endif // USE_B25_DECODER_DLL
	};
};

class cProxyServerEx {
#ifdef HAVE_UI
public:
#endif
	SOCKET m_s;
	DWORD m_dwSpace;
	DWORD m_dwChannel;
	char *m_pDriversMapKey;
	int m_iDriverNo;
	BYTE m_bChannelLock;
#ifdef HAVE_UI
private:
#endif
	int m_iDriverUseOrder;
	IBonDriver *m_pIBon;
	IBonDriver2 *m_pIBon2;
	IBonDriver3 *m_pIBon3;
	HMODULE m_hModule;
	cEvent m_Error;
	BOOL m_bTunerOpen;
	HANDLE m_hTsRead;
	stTsReaderArg *m_pTsReaderArg;
	cPacketFifo m_fifoSend;
	cPacketFifo m_fifoRecv;

	DWORD Process();
	int ReceiverHelper(char *pDst, DWORD left);
	static DWORD WINAPI Receiver(LPVOID pv);
	void makePacket(enumCommand eCmd, BOOL b);
	void makePacket(enumCommand eCmd, DWORD dw);
	void makePacket(enumCommand eCmd, LPCTSTR str);
	void makePacket(enumCommand eCmd, BYTE *pSrc, DWORD dwSize, float fSignalLevel);
	static DWORD WINAPI Sender(LPVOID pv);
	static DWORD WINAPI TsReader(LPVOID pv);
	void StopTsReceive();

	BOOL SelectBonDriver(LPCSTR p, BYTE bChannelLock);
	IBonDriver *CreateBonDriver();

	// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);
	void PurgeTsStream(void);
	void Release(void);

	// IBonDriver2
	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);
	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	// IBonDriver3
	const DWORD GetTotalDeviceNum(void);
	const DWORD GetActiveDeviceNum(void);
	const BOOL SetLnbPower(const BOOL bEnable);

public:
	cProxyServerEx();
	~cProxyServerEx();
	void setSocket(SOCKET s){ m_s = s; }
	static DWORD WINAPI Reception(LPVOID pv);
};

static std::list<cProxyServerEx *> g_InstanceList;
static cCriticalSection g_Lock;
static cEvent g_ShutdownEvent(TRUE, FALSE);
#if defined(HAVE_UI) || defined(BUILD_AS_SERVICE)
static HANDLE g_hListenThread;
#endif

#endif	// __BONDRIVER_PROXYEX_H__
