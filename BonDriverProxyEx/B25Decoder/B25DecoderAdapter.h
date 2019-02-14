#ifndef __B25_DECODER_ADAPTER_H__
#define __B25_DECODER_ADAPTER_H__

#ifdef USE_B25_DECODER_DLL

#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
// IB25Decoderインターフェイスの外部DLLを
// B25Decoderクラスに適合させるアダプタクラス
/////////////////////////////////////////////////////////////////////////////

class B25DecoderAdapter {
public:
	B25DecoderAdapter()
		: m_pB25(NULL)
		, m_hModule(NULL)
	{};
	~B25DecoderAdapter() { unload(); };
	inline int init() { return m_pB25->Initialize(multi2_round); };
	inline int reset() { return m_pB25->Reset(); };
	inline void release() { m_pB25->Release(); };
	inline void decode(BYTE *pSrc, DWORD dwSrcSize, BYTE **ppDst, DWORD *pdwDstSize) { m_pB25->Decode(pSrc, dwSrcSize, ppDst, pdwDstSize); };

	BOOL load()
	{
		unload();

		m_hModule = ::LoadLibrary(_T(".\\B25Decoder.dll"));
		if (m_hModule == NULL)
		{
			m_hModule = ::LoadLibrary(_T(".\\libaribb25.dll"));
			if (m_hModule == NULL)
			{
				return FALSE;
			}
		}
		IB25Decoder *(*func)();
		func = (IB25Decoder *(*)())::GetProcAddress(m_hModule, "CreateB25Decoder");
		if (!func)
		{
			unload();
			return FALSE;
		}
		IB25Decoder *pB25 = func();
		BOOL ret = TRUE;
		if (pB25->Initialize(multi2_round) == FALSE)
		{
			pB25->Release();
			ret = FALSE;
		}
		else
		{
			try
			{
				m_pB25 = dynamic_cast<IB25Decoder2 *>(pB25);
				if (m_pB25 != NULL)
				{
					m_pB25->DiscardNullPacket(static_cast<bool>(strip));
					m_pB25->DiscardScramblePacket(false);
					m_pB25->EnableEmmProcess(static_cast<bool>(emm_proc));
				}
			}
			catch (std::__non_rtti_object&)
			{
				m_pB25 = NULL;
				pB25->Release();
				ret = FALSE;
			}
		}
		if (ret == FALSE)
		{
			unload();
		}

		return ret;
	};
	void unload()
	{
		if (m_pB25 != NULL)
		{
			m_pB25->Release();
			m_pB25 = NULL;
		}
		if (m_hModule != NULL)
		{
			::FreeLibrary(m_hModule);
			m_hModule = NULL;
		}
	};
	BOOL is_loaded() { return m_pB25 != NULL; };

	// initialize parameter
	static int strip;
	static int emm_proc;
	static int multi2_round;

private:
	IB25Decoder2 *m_pB25;
	HMODULE m_hModule;
};

#endif	// USE_B25_DECODER_DLL

#endif	// __B25_DECODER_ADAPTER_H__
