// DXC_dinput.h: Mouse input via Raw Input API (replaces DirectInput 7)
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
#define AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>

class DXC_dinput
{
public:
	DXC_dinput();
	virtual ~DXC_dinput();
	void UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB, char * pMB);
	void SetAcquire(BOOL bFlag);
	BOOL bInit(HWND hWnd, HINSTANCE hInst);

	void SetBounds(short sMaxX, short sMaxY);

	// Raw Input message handler - call from WndProc on WM_INPUT
	void OnRawInput(LPARAM lParam);
	// Mouse wheel handler - call from WndProc on WM_MOUSEWHEEL
	void OnMouseWheel(short zDelta);

	short m_sX, m_sY, m_sZ;
	short m_sMaxX, m_sMaxY;
	float m_fMouseScale;    // Reciprocal of GPU uniform scale (default 1.0)

private:
	HWND m_hWnd;
	BOOL m_bAcquired;
	// Accumulated deltas from WM_INPUT, consumed by UpdateMouseState
	long m_lDeltaX, m_lDeltaY;
	short m_sWheelDelta;
	// Button state from raw input (0x80 = pressed, 0 = released, matches DirectInput convention)
	char m_cButtons[3];
};

#endif // !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
