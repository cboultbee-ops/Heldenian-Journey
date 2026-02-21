// YWSound.h: XAudio2 sound device wrapper (replaces DirectSound)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YWSOUND_H__AB055FE0_F550_11D1_8255_00002145AAC4__INCLUDED_)
#define AFX_YWSOUND_H__AB055FE0_F550_11D1_8255_00002145AAC4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <xaudio2.h>

class YWSound
{
public:
	YWSound();
	virtual ~YWSound();
	bool Create(HWND hWnd);
	IXAudio2* m_pXAudio2;
	IXAudio2MasteringVoice* m_pMasterVoice;
};

#endif // !defined(AFX_YWSOUND_H__AB055FE0_F550_11D1_8255_00002145AAC4__INCLUDED_)
