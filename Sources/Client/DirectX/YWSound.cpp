// YWSound.cpp: XAudio2 sound device wrapper (replaces DirectSound)
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "YWSound.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YWSound::YWSound()
{
	m_pXAudio2 = NULL;
	m_pMasterVoice = NULL;
}

YWSound::~YWSound()
{
	if (m_pMasterVoice) {
		m_pMasterVoice->DestroyVoice();
		m_pMasterVoice = NULL;
	}
	if (m_pXAudio2) {
		m_pXAudio2->Release();
		m_pXAudio2 = NULL;
	}
	CoUninitialize();
}

//////////////////////////////////////////////////////////////////////////////////
// YWSound Create
//////////////////////////////////////////////////////////////////////////////////
bool YWSound::Create(HWND hWnd)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
		OutputDebugString("CoInitializeEx error...\n");
		return FALSE;
	}

	hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) {
		OutputDebugString("XAudio2Create error...\n");
		return FALSE;
	}

	hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice);
	if (FAILED(hr)) {
		OutputDebugString("CreateMasteringVoice error...\n");
		m_pXAudio2->Release();
		m_pXAudio2 = NULL;
		return FALSE;
	}

	return TRUE;
}
