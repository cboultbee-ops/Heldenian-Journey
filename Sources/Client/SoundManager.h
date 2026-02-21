// SoundManager.h: Sound subsystem extracted from CGame
//
//////////////////////////////////////////////////////////////////////

#if !defined(SOUNDMANAGER_H_INCLUDED_)
#define SOUNDMANAGER_H_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif

#include <windows.h>
#include <unordered_map>
#include "DirectX\YWSound.h"
#include "DirectX\SoundBuffer.h"

class CSoundManager
{
public:
	CSoundManager();
	~CSoundManager();

	// Initialize DirectSound
	BOOL Init(HWND hWnd);

	// Load all sound effects (C1-C24, M1-M156, E1-E53)
	void LoadSounds();

	// Play a sound effect. cType: 'C'ombat, 'M'onster, 'E'ffect
	// Named PlaySfx to avoid Windows API PlaySound/PlaySoundA macro conflict
	void PlaySfx(char cType, int iNum, int iDist, long lPan = 0);

	// BGM management
	void StartBGM(const char * cLocation, BOOL bIsXmas, char cWhetherType);
	void StopBGM();
	void UpdateBGMVolume();  // Re-apply volume from m_cMusicVolume

	// Weather ambient sound (E38)
	void PlayWeatherSound();
	void StopWeatherSound();

	// Release buffers unused for >30 seconds
	void ReleaseUnusedBuffers(DWORD dwCurrentTime);

	// Cleanup all sound resources
	void Cleanup();

	// Public state — accessed by CGame UI code for volume sliders
	BOOL m_bSoundFlag;
	BOOL m_bSoundStat;    // Sound effects on/off
	BOOL m_bMusicStat;    // Music on/off
	char m_cSoundVolume;  // 0-100
	char m_cMusicVolume;  // 0-100

private:
	YWSound m_DSound;
	std::unordered_map<int, CSoundBuffer *> m_pCSound;
	std::unordered_map<int, CSoundBuffer *> m_pMSound;
	std::unordered_map<int, CSoundBuffer *> m_pESound;
	CSoundBuffer * m_pBGM;
};

#endif // !defined(SOUNDMANAGER_H_INCLUDED_)
