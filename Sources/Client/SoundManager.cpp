// SoundManager.cpp: Sound subsystem extracted from CGame
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include "SoundManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoundManager::CSoundManager()
{
	m_pBGM = NULL;
	m_bSoundFlag = FALSE;
	m_bSoundStat = TRUE;
	m_bMusicStat = TRUE;
	m_cSoundVolume = 100;
	m_cMusicVolume = 100;
}

CSoundManager::~CSoundManager()
{
	Cleanup();
}

BOOL CSoundManager::Init(HWND hWnd)
{
	m_bSoundFlag = m_DSound.Create(hWnd);
	if (!m_bSoundFlag)
		m_bSoundStat = m_bMusicStat = FALSE;
	return m_bSoundFlag;
}

void CSoundManager::LoadSounds()
{
	if (!m_bSoundFlag) return;

	char cTxt[64];
	for (int i = 1; i <= 24; i++) {
		wsprintf(cTxt, "sounds\\C%d.wav", i);
		m_pCSound[i] = new CSoundBuffer(m_DSound.m_pXAudio2, cTxt);
	}
	for (int i = 1; i <= 156; i++) {
		wsprintf(cTxt, "sounds\\M%d.wav", i);
		m_pMSound[i] = new CSoundBuffer(m_DSound.m_pXAudio2, cTxt);
	}
	for (int i = 1; i <= 53; i++) {
		wsprintf(cTxt, "sounds\\E%d.wav", i);
		m_pESound[i] = new CSoundBuffer(m_DSound.m_pXAudio2, cTxt);
	}
}

void CSoundManager::PlaySfx(char cType, int iNum, int iDist, long lPan)
{
	if (!m_bSoundFlag) return;
	if (!m_bSoundStat) return;

	if (iDist > 10) iDist = 10;

	int iVol = (m_cSoundVolume - 100) * 20;
	iVol += -200 * iDist;

	if (iVol > 0)      iVol = 0;
	if (iVol < -10000)  iVol = -10000;

	if (iVol > -2000) {
		std::unordered_map<int, CSoundBuffer *>::iterator it;
		switch (cType) {
		case 'C':
			it = m_pCSound.find(iNum);
			if (it == m_pCSound.end() || it->second == NULL) return;
			it->second->Play(FALSE, lPan, iVol);
			break;
		case 'M':
			it = m_pMSound.find(iNum);
			if (it == m_pMSound.end() || it->second == NULL) return;
			it->second->Play(FALSE, lPan, iVol);
			break;
		case 'E':
			it = m_pESound.find(iNum);
			if (it == m_pESound.end() || it->second == NULL) return;
			it->second->Play(FALSE, lPan, iVol);
			break;
		}
	}
}

void CSoundManager::StartBGM(const char * cLocation, BOOL bIsXmas, char cWhetherType)
{
	if (!m_bSoundFlag) {
		if (m_pBGM != NULL) {
			m_pBGM->bStop();
			delete m_pBGM;
			m_pBGM = NULL;
		}
		return;
	}

	char cWavFileName[32];
	ZeroMemory(cWavFileName, sizeof(cWavFileName));

	if ((bIsXmas == TRUE) && (cWhetherType >= 4))
		strcpy(cWavFileName, "music\\Carol.wav");
	else {
		if (memcmp(cLocation, "aresden", 7) == 0)           strcpy(cWavFileName, "music\\aresden.wav");
		else if (memcmp(cLocation, "elvine", 6) == 0)       strcpy(cWavFileName, "music\\elvine.wav");
		else if (memcmp(cLocation, "dglv", 4) == 0)         strcpy(cWavFileName, "music\\dungeon.wav");
		else if (memcmp(cLocation, "middled1", 8) == 0)     strcpy(cWavFileName, "music\\dungeon.wav");
		else if (memcmp(cLocation, "middleland", 10) == 0)  strcpy(cWavFileName, "music\\middleland.wav");
		else if (memcmp(cLocation, "druncncity", 10) == 0)  strcpy(cWavFileName, "music\\druncncity.wav");
		else if (memcmp(cLocation, "inferniaA", 9) == 0)    strcpy(cWavFileName, "music\\middleland.wav");
		else if (memcmp(cLocation, "inferniaB", 9) == 0)    strcpy(cWavFileName, "music\\middleland.wav");
		else if (memcmp(cLocation, "maze", 4) == 0)         strcpy(cWavFileName, "music\\dungeon.wav");
		else if (memcmp(cLocation, "abaddon", 7) == 0)      strcpy(cWavFileName, "music\\abaddon.wav");
		else if (strcmp(cLocation, "istria") == 0)           strcpy(cWavFileName, "music\\istria.wav");
		else if (strcmp(cLocation, "astoria") == 0)          strcpy(cWavFileName, "music\\astoria.wav");
		else strcpy(cWavFileName, "music\\MainTm.wav");
	}

	if (m_pBGM != NULL) {
		if (strcmp(m_pBGM->m_cWavFileName, cWavFileName) == 0) return;
		m_pBGM->bStop();
		delete m_pBGM;
		m_pBGM = NULL;
	}

	int iVolume = (m_cMusicVolume - 100) * 20;
	if (iVolume > 0)      iVolume = 0;
	if (iVolume < -10000)  iVolume = -10000;

	m_pBGM = new CSoundBuffer(m_DSound.m_pXAudio2, cWavFileName, TRUE);
	m_pBGM->Play(TRUE, 0, iVolume);
}

void CSoundManager::StopBGM()
{
	if (m_pBGM != NULL)
		m_pBGM->bStop();
}

void CSoundManager::UpdateBGMVolume()
{
	if (m_pBGM == NULL) return;
	int iVol = (m_cMusicVolume - 100) * 20;
	if (iVol > 0)      iVol = 0;
	if (iVol < -10000)  iVol = -10000;
	m_pBGM->bStop(TRUE);
	m_pBGM->Play(FALSE, 0, iVol);
}

void CSoundManager::PlayWeatherSound()
{
	if (m_bSoundStat && m_bSoundFlag && m_pESound.count(38))
		m_pESound[38]->Play(TRUE);
}

void CSoundManager::StopWeatherSound()
{
	if (m_pESound.count(38))
		m_pESound[38]->bStop();
}

void CSoundManager::ReleaseUnusedBuffers(DWORD dwCurrentTime)
{
	for (auto & kv : m_pCSound)
		if (kv.second != NULL && !kv.second->m_bIsLooping && (dwCurrentTime - kv.second->m_dwTime) > 30000)
			kv.second->_ReleaseSoundBuffer();
	for (auto & kv : m_pMSound)
		if (kv.second != NULL && !kv.second->m_bIsLooping && (dwCurrentTime - kv.second->m_dwTime) > 30000)
			kv.second->_ReleaseSoundBuffer();
	for (auto & kv : m_pESound)
		if (kv.second != NULL && !kv.second->m_bIsLooping && (dwCurrentTime - kv.second->m_dwTime) > 30000)
			kv.second->_ReleaseSoundBuffer();
}

void CSoundManager::Cleanup()
{
	for (auto & kv : m_pCSound) delete kv.second;
	for (auto & kv : m_pMSound) delete kv.second;
	for (auto & kv : m_pESound) delete kv.second;
	m_pCSound.clear();
	m_pMSound.clear();
	m_pESound.clear();

	if (m_pBGM != NULL) {
		delete m_pBGM;
		m_pBGM = NULL;
	}
}
