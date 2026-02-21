// DXC_dinput.cpp: Mouse input via Raw Input API (replaces DirectInput 7)
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_hWnd       = NULL;
	m_bAcquired  = FALSE;
	m_sX         = 0;
	m_sY         = 0;
	m_sZ         = 0;
	m_sMaxX      = 639;
	m_sMaxY      = 479;
	m_lDeltaX    = 0;
	m_lDeltaY    = 0;
	m_sWheelDelta = 0;
	m_cButtons[0] = 0;
	m_cButtons[1] = 0;
	m_cButtons[2] = 0;
	m_fMouseScale = 1.0f;
}

DXC_dinput::~DXC_dinput()
{
	if (m_hWnd != NULL) {
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
}

BOOL DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
	m_hWnd = hWnd;

	// Get initial cursor position
	POINT Point;
	GetCursorPos(&Point);
	ScreenToClient(hWnd, &Point);
	m_sX = (short)(Point.x);
	m_sY = (short)(Point.y);

	// Register for Raw Input mouse events
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
	rid.usUsage     = 0x02;          // HID_USAGE_GENERIC_MOUSE
	rid.dwFlags     = 0;             // Receive input even when not foreground (SetAcquire handles focus)
	rid.hwndTarget  = hWnd;

	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
		return FALSE;

	// Start acquired (matches DirectInput DISCL_FOREGROUND behavior)
	SetAcquire(TRUE);

	return TRUE;
}

void DXC_dinput::SetAcquire(BOOL bFlag)
{
	if (m_hWnd == NULL) return;

	if (bFlag == TRUE) {
		m_bAcquired = TRUE;
		// Confine cursor to window (replaces DISCL_EXCLUSIVE)
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		POINT pt = {0, 0};
		ClientToScreen(m_hWnd, &pt);
		OffsetRect(&rc, pt.x, pt.y);
		ClipCursor(&rc);
		ShowCursor(FALSE);
	} else {
		m_bAcquired = FALSE;
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
}

void DXC_dinput::OnRawInput(LPARAM lParam)
{
	UINT dwSize = 0;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize == 0) return;

	// Use stack buffer for small inputs (typical RAWINPUT is ~48 bytes)
	BYTE stackBuf[64];
	BYTE * lpb = (dwSize <= sizeof(stackBuf)) ? stackBuf : new BYTE[dwSize];

	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
		if (lpb != stackBuf) delete[] lpb;
		return;
	}

	RAWINPUT * raw = (RAWINPUT *)lpb;
	if (raw->header.dwType == RIM_TYPEMOUSE) {
		RAWMOUSE & rm = raw->data.mouse;

		// Accumulate relative movement deltas
		if ((rm.usFlags & MOUSE_MOVE_ABSOLUTE) == 0) {
			m_lDeltaX += rm.lLastX;
			m_lDeltaY += rm.lLastY;
		}

		// Track button state
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) m_cButtons[0] = (char)0x80;
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_1_UP)   m_cButtons[0] = 0;
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) m_cButtons[1] = (char)0x80;
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_2_UP)   m_cButtons[1] = 0;
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) m_cButtons[2] = (char)0x80;
		if (rm.usButtonFlags & RI_MOUSE_BUTTON_3_UP)   m_cButtons[2] = 0;

		// Mouse wheel (vertical)
		if (rm.usButtonFlags & RI_MOUSE_WHEEL) {
			m_sWheelDelta = (short)rm.usButtonData;
		}
	}

	if (lpb != stackBuf) delete[] lpb;
}

void DXC_dinput::OnMouseWheel(short zDelta)
{
	m_sWheelDelta = zDelta;
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB, char * pMB)
{
	if (!m_bAcquired) return;

	// Apply accumulated deltas, scaled by reciprocal of viewport scale
	// This makes cursor movement match the visual scale on screen
	m_sX += (short)(m_lDeltaX * m_fMouseScale);
	m_sY += (short)(m_lDeltaY * m_fMouseScale);
	m_lDeltaX = 0;
	m_lDeltaY = 0;

	// Wheel
	if (m_sWheelDelta != 0) {
		m_sZ = m_sWheelDelta;
		m_sWheelDelta = 0;
	}

	// Clamp to bounds
	if (m_sX < 0)       m_sX = 0;
	if (m_sY < 0)       m_sY = 0;
	if (m_sX > m_sMaxX) m_sX = m_sMaxX;
	if (m_sY > m_sMaxY) m_sY = m_sMaxY;

	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = m_cButtons[0];
	*pRB = m_cButtons[1];
	*pMB = m_cButtons[2];
}

void DXC_dinput::SetBounds(short sMaxX, short sMaxY)
{
	m_sMaxX = sMaxX;
	m_sMaxY = sMaxY;
}
