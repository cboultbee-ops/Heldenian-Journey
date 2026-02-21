// SoundBuffer.cpp: XAudio2 sound buffer wrapper (replaces DirectSound buffer)
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "SoundBuffer.h"

struct Waveheader
{
	BYTE        RIFF[4];          // "RIFF"
	DWORD       dwSize;           // Size of data to follow
	BYTE        WAVE[4];          // "WAVE"
	BYTE        fmt_[4];          // "fmt "
	DWORD       dw16;             // 16
	WORD        wOne_0;           // 1
	WORD        wChnls;           // Number of Channels
	DWORD       dwSRate;          // Sample Rate
	DWORD       BytesPerSec;      // Sample Rate
	WORD        wBlkAlign;        // 1
	WORD        BitsPerSample;    // Sample size
	BYTE        DATA[4];          // "DATA"
	DWORD       dwDSize;          // Number of Samples
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoundBuffer::CSoundBuffer(IXAudio2* pXAudio2, char * pWavFileName, BOOL bIsSingleLoad)
{
	m_pXAudio2 = pXAudio2;

	ZeroMemory(m_cWavFileName, sizeof(m_cWavFileName));
	strcpy(m_cWavFileName, pWavFileName);

	for (int i = 0; i < MAXSOUNDBUFFERS; i++) m_pVoice[i] = NULL;

	m_pWavData = NULL;
	m_dwWavDataSize = 0;
	ZeroMemory(&m_wfx, sizeof(m_wfx));

	m_cCurrentBufferIndex = 0;
	m_bIsSingleLoad = bIsSingleLoad;
	m_dwTime = NULL;
	m_bIsLooping = FALSE;
}

CSoundBuffer::~CSoundBuffer()
{
	for (int i = 0; i < MAXSOUNDBUFFERS; i++)
	{
		if (m_pVoice[i] != NULL)
		{
			m_pVoice[i]->Stop(0);
			m_pVoice[i]->DestroyVoice();
			m_pVoice[i] = NULL;
		}
	}
	delete[] m_pWavData;
	m_pWavData = NULL;
}

BOOL CSoundBuffer::_LoadWavFile()
{
	if (m_pWavData != NULL) return TRUE; // Already loaded

	FILE * pFile = fopen(m_cWavFileName, "rb");
	if (pFile == NULL) return FALSE;

	Waveheader Wavhdr;
	if (fread(&Wavhdr, sizeof(Wavhdr), 1, pFile) != 1)
	{
		fclose(pFile);
		return FALSE;
	}

	// Validate WAV header
	if (memcmp(Wavhdr.RIFF, "RIFF", 4) != 0 || memcmp(Wavhdr.WAVE, "WAVE", 4) != 0) {
		fclose(pFile);
		return FALSE;
	}
	if (Wavhdr.dwSRate == 0 || Wavhdr.wBlkAlign == 0 || Wavhdr.BitsPerSample == 0) {
		fclose(pFile);
		return FALSE;
	}
	if (Wavhdr.dwDSize == 0 || Wavhdr.dwDSize > 50 * 1024 * 1024) { // Reject >50MB
		fclose(pFile);
		return FALSE;
	}

	m_dwWavDataSize = Wavhdr.dwDSize;

	// Fill WAVEFORMATEX
	ZeroMemory(&m_wfx, sizeof(m_wfx));
	m_wfx.wFormatTag = WAVE_FORMAT_PCM;
	m_wfx.nChannels = Wavhdr.wChnls;
	m_wfx.nSamplesPerSec = Wavhdr.dwSRate;
	m_wfx.wBitsPerSample = Wavhdr.BitsPerSample;
	m_wfx.nBlockAlign = Wavhdr.wBlkAlign;
	m_wfx.nAvgBytesPerSec = m_wfx.nSamplesPerSec * m_wfx.nBlockAlign;

	// Load WAV data into heap buffer
	m_pWavData = new BYTE[m_dwWavDataSize];
	if (fseek(pFile, sizeof(Waveheader), SEEK_SET) != 0) {
		delete[] m_pWavData;
		m_pWavData = NULL;
		fclose(pFile);
		return FALSE;
	}
	if (fread(m_pWavData, m_dwWavDataSize, 1, pFile) != 1) {
		delete[] m_pWavData;
		m_pWavData = NULL;
		fclose(pFile);
		return FALSE;
	}

	fclose(pFile);
	return TRUE;
}

BOOL CSoundBuffer::_CreateVoice(char cBufferIndex)
{
	if (m_pXAudio2 == NULL || m_pWavData == NULL) return FALSE;
	if (m_pVoice[cBufferIndex] != NULL) return FALSE;

	HRESULT hr = m_pXAudio2->CreateSourceVoice(&m_pVoice[cBufferIndex], &m_wfx);
	return SUCCEEDED(hr);
}

BOOL CSoundBuffer::Play(BOOL bLoop, long lPan, int iVol)
{
	if (m_pXAudio2 == NULL) return FALSE;

	IXAudio2SourceVoice* voice = GetIdleVoice();
	if (voice == NULL) return FALSE;

	// Set volume (DS dB scale → XAudio2 linear)
	SetVolume(iVol);

	// Apply pan via output matrix (assume stereo output)
	if (lPan != 0) {
		XAUDIO2_VOICE_DETAILS voiceDetails;
		voice->GetVoiceDetails(&voiceDetails);
		UINT32 srcChannels = voiceDetails.InputChannels;

		float fPan = (float)lPan / 10000.0f;
		if (fPan < -1.0f) fPan = -1.0f;
		if (fPan > 1.0f)  fPan = 1.0f;

		float left  = (fPan <= 0.0f) ? 1.0f : (1.0f - fPan);
		float right = (fPan >= 0.0f) ? 1.0f : (1.0f + fPan);

		if (srcChannels == 1) {
			float matrix[2] = { left, right };
			voice->SetOutputMatrix(NULL, 1, 2, matrix);
		} else if (srcChannels == 2) {
			float matrix[4] = { left, 0.0f, 0.0f, right };
			voice->SetOutputMatrix(NULL, 2, 2, matrix);
		}
	}

	m_bIsLooping = bLoop;

	// Stop and flush any existing playback
	voice->Stop(0);
	voice->FlushSourceBuffers();

	// Submit buffer
	XAUDIO2_BUFFER buf = {0};
	buf.AudioBytes = m_dwWavDataSize;
	buf.pAudioData = m_pWavData;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	if (bLoop) buf.LoopCount = XAUDIO2_LOOP_INFINITE;

	HRESULT hr = voice->SubmitSourceBuffer(&buf);
	if (FAILED(hr)) return FALSE;

	hr = voice->Start(0);
	if (FAILED(hr)) return FALSE;

	return TRUE;
}

IXAudio2SourceVoice* CSoundBuffer::GetIdleVoice()
{
	if (m_pXAudio2 == NULL) return NULL;

	// Ensure WAV data is loaded (lazy load)
	if (m_pWavData == NULL) {
		if (!_LoadWavFile()) return NULL;
	}

	if (m_pVoice[m_cCurrentBufferIndex] != NULL) {
		XAUDIO2_VOICE_STATE state;
		m_pVoice[m_cCurrentBufferIndex]->GetState(&state);

		if (state.BuffersQueued > 0) {
			// Currently playing
			if (m_bIsSingleLoad == TRUE) {
				m_pVoice[m_cCurrentBufferIndex]->Stop(0);
				m_pVoice[m_cCurrentBufferIndex]->FlushSourceBuffers();
				m_dwTime = timeGetTime();
				return m_pVoice[m_cCurrentBufferIndex];
			}

			// Cycle to next voice
			m_cCurrentBufferIndex++;
			if (m_cCurrentBufferIndex >= MAXSOUNDBUFFERS) m_cCurrentBufferIndex = 0;

			if (m_pVoice[m_cCurrentBufferIndex] != NULL) {
				m_pVoice[m_cCurrentBufferIndex]->GetState(&state);
				if (state.BuffersQueued > 0) {
					m_pVoice[m_cCurrentBufferIndex]->Stop(0);
					m_pVoice[m_cCurrentBufferIndex]->FlushSourceBuffers();
				}
			}
			else {
				_CreateVoice(m_cCurrentBufferIndex);
			}
		}
	}
	else {
		_CreateVoice(m_cCurrentBufferIndex);
	}

	m_dwTime = timeGetTime();
	return m_pVoice[m_cCurrentBufferIndex];
}

void CSoundBuffer::SetVolume(LONG Volume)
{
	// Convert DirectSound dB volume (-10000..0) to XAudio2 linear (0..1)
	float fVol;
	if (Volume <= -10000) fVol = 0.0f;
	else if (Volume >= 0) fVol = 1.0f;
	else fVol = powf(10.0f, (float)Volume / 2000.0f);

	for (int i = 0; i < MAXSOUNDBUFFERS; i++)
		if (m_pVoice[i] != NULL) m_pVoice[i]->SetVolume(fVol);
}

void CSoundBuffer::bStop(BOOL bIsNoRewind)
{
	for (int i = 0; i < MAXSOUNDBUFFERS; i++)
	{
		if (m_pVoice[i] != NULL)
		{
			m_pVoice[i]->Stop(0);
			if (bIsNoRewind == FALSE) m_pVoice[i]->FlushSourceBuffers();
		}
	}
}

void CSoundBuffer::_ReleaseSoundBuffer()
{
	for (int i = 0; i < MAXSOUNDBUFFERS; i++)
	{
		if (m_pVoice[i] != NULL) {
			m_pVoice[i]->DestroyVoice();
			m_pVoice[i] = NULL;
		}
	}
}
