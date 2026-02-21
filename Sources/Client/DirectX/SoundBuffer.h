// SoundBuffer.h: XAudio2 sound buffer wrapper (replaces DirectSound buffer)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOUNDBUFFER_H__A0D1F3DC_F322_4310_9295_88EAD41F19DA__INCLUDED_)
#define AFX_SOUNDBUFFER_H__A0D1F3DC_F322_4310_9295_88EAD41F19DA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <xaudio2.h>

#define MAXSOUNDBUFFERS		2


class CSoundBuffer
{
public:
	void _ReleaseSoundBuffer();
	void bStop(BOOL bIsNoRewind = FALSE);
	void SetVolume(LONG Volume);
	IXAudio2SourceVoice* GetIdleVoice();
	BOOL Play(BOOL bLoop = FALSE, long lPan = 0, int iVol = 0);
	BOOL _LoadWavFile();
	BOOL _CreateVoice(char cBufferIndex);
	CSoundBuffer(IXAudio2* pXAudio2, char * pWavFileName, BOOL bIsSingleLoad = FALSE);
	virtual ~CSoundBuffer();

	IXAudio2*				m_pXAudio2;

	char					m_cWavFileName[32];
	IXAudio2SourceVoice*	m_pVoice[MAXSOUNDBUFFERS];
	BYTE*					m_pWavData;
	DWORD					m_dwWavDataSize;
	WAVEFORMATEX			m_wfx;
	char					m_cCurrentBufferIndex;

	BOOL					m_bIsSingleLoad;
	BOOL					m_bIsLooping;
	DWORD					m_dwTime;
};

#endif // !defined(AFX_SOUNDBUFFER_H__A0D1F3DC_F322_4310_9295_88EAD41F19DA__INCLUDED_)
