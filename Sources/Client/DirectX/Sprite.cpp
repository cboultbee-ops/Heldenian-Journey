// Sprite.cpp: implementation of the CSprite class.
//
//////////////////////////////////////////////////////////////////////

#include "Sprite.h"
#include "GPURenderer.h"
#include "../stb_image.h"

extern char G_cSpriteAlphaDegree;

extern int G_iAddTable31[64][510], G_iAddTable63[64][510];
extern int G_iAddTransTable31[510][64], G_iAddTransTable63[510][64]; 

extern long    G_lTransG100[64][64], G_lTransRB100[64][64];
extern long    G_lTransG70[64][64], G_lTransRB70[64][64];
extern long    G_lTransG50[64][64], G_lTransRB50[64][64];
extern long    G_lTransG25[64][64], G_lTransRB25[64][64];
extern long    G_lTransG2[64][64], G_lTransRB2[64][64];


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSprite::CSprite(HANDLE hPakFile, DXC_ddraw *pDDraw, char *cPakFileName, short sNthFile, bool bAlphaEffect, std::vector<int> * framePositions)
{
	DWORD  nCount;
	int iASDstart;
	DWORD nextBlockStart = 0;

	m_stBrush	= NULL;
	m_lpSurface = NULL;
	m_bIsSurfaceEmpty = TRUE;
	ZeroMemory(m_cPakFileName, sizeof(m_cPakFileName));
	m_dwBitmapFileSize = 0;

	m_cAlphaDegree = 1;
	m_bOnCriticalSection = FALSE;
	m_iTotalFrame = 0;
	m_iSpriteScale = 1;
	m_pDDraw = pDDraw;
	if (framePositions) {
		iASDstart = (*framePositions)[sNthFile];
		if (sNthFile + 1 < (short)framePositions->size())
			nextBlockStart = (DWORD)(*framePositions)[sNthFile + 1];
	}
	else {
		DWORD iTotalimage = 0;
		SetFilePointer(hPakFile, 20, NULL, FILE_BEGIN);
		ReadFile(hPakFile, &iTotalimage, 4, &nCount, NULL);
		SetFilePointer(hPakFile, 24 + sNthFile * 8, NULL, FILE_BEGIN);
		ReadFile(hPakFile, &iASDstart, 4, &nCount, NULL);
		if (sNthFile + 1 < (short)iTotalimage) {
			SetFilePointer(hPakFile, 24 + (sNthFile + 1) * 8, NULL, FILE_BEGIN);
			ReadFile(hPakFile, &nextBlockStart, 4, &nCount, NULL);
		}
	}
	//i+100       Sprite Confirm
	SetFilePointer(hPakFile, iASDstart+100, NULL, FILE_BEGIN);
	ReadFile(hPakFile, &m_iTotalFrame,  4, &nCount, NULL);
	m_dwBitmapFileStartLoc = iASDstart  + (108 + (12*m_iTotalFrame));
	m_stBrush = new stBrush[m_iTotalFrame];
	ReadFile(hPakFile, m_stBrush, 12*m_iTotalFrame, &nCount, NULL);
	// Image size for PNG/BMP-in-PAK (next block start - image start; or from file size if last sprite)
	if (nextBlockStart > m_dwBitmapFileStartLoc)
		m_dwBitmapFileSize = nextBlockStart - m_dwBitmapFileStartLoc;
	else if (hPakFile && hPakFile != INVALID_HANDLE_VALUE) {
		DWORD fileSize = GetFileSize(hPakFile, NULL);
		if (fileSize > m_dwBitmapFileStartLoc)
			m_dwBitmapFileSize = fileSize - m_dwBitmapFileStartLoc;
	}
	// PAK
	memcpy(m_cPakFileName, cPakFileName, strlen(cPakFileName));
	m_bAlphaEffect = bAlphaEffect;

	// GPU texture initialization
	m_glTextureID = 0;
	m_bIsGPUTexture = false;
}

CSprite::~CSprite()
{
	UnloadFromGPU();
	if (m_stBrush != NULL) delete[] m_stBrush;
	if (m_lpSurface != NULL) m_lpSurface->Release();
}

IDirectDrawSurface7 * CSprite::_pMakeSpriteSurface()
{
 IDirectDrawSurface7 * pdds4;
 HDC hDC;
 WORD * wp;

	m_bOnCriticalSection = TRUE;

	if( m_stBrush == NULL ) return NULL;

	CMyDib mydib(m_cPakFileName, m_dwBitmapFileStartLoc);
	m_wBitmapSizeX = mydib.m_wWidthX;
	m_wBitmapSizeY = mydib.m_wWidthY;
	pdds4 = m_pDDraw->pCreateOffScreenSurface(m_wBitmapSizeX, m_wBitmapSizeY);
    if (pdds4 == NULL) return NULL; 
	pdds4->GetDC(&hDC);
	mydib.PaintImage(hDC);
	pdds4->ReleaseDC(hDC);

	DDSURFACEDESC2  ddsd;
	ddsd.dwSize = 124;
	if (pdds4->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return NULL;
	pdds4->Unlock(NULL);

	wp = (WORD *)ddsd.lpSurface;
	m_wColorKey = *wp;

	m_bOnCriticalSection = FALSE;

    return pdds4;
}


void CSprite::PutSpriteFast(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	RECT rcRect;
	if( this == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if( m_stBrush == NULL ) return;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	dX = static_cast<short>(sX + pvx);
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) {
			LoadToGPU();
		}
		if (m_bIsGPUTexture) {
			m_pDDraw->m_pGPURenderer->QueueSprite(
				m_glTextureID, dX, dY, sx, sy, szx, szy,
				m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_COLORKEY, 1.0f, 0.0f, 0.0f, 0.0f);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	// DirectDraw fallback path
	if (m_bIsSurfaceEmpty == TRUE)
	{
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_pDDraw->m_lpBackB4->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutSpriteFastDst(LPDIRECTDRAWSURFACE7 lpDstS, int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;	
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE)
	{	if( _iOpenSprite() == FALSE ) return;
	}else // AlphaDegree
	{	if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) 
			{	_SetAlphaDegree();
			}else 
			{	_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
	}	}	}

	//SetRect(&rcRect,  sx, sy, sx + szx, sy + szy); // our fictitious sprite bitmap is 
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;
	lpDstS->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT );
	m_bOnCriticalSection = FALSE;
}


void CSprite::PutSpriteFastNoColorKey(int sX, int sY, int sFrame, DWORD dwTime)
{short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_OPAQUE, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;
  	dX = sX + pvx;
	dY = sY + pvy;
	if (dX < m_pDDraw->m_rcClipArea.left)
	{	sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0)
		{	m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{	szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) 
		{	m_rcBound.top = -1;
			return;
	}	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{	sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) 
		{	m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{	szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) 
		{	m_rcBound.top = -1;
			return;
	}	}
	
	m_dwRefTime = dwTime;
	if (m_bIsSurfaceEmpty == TRUE)
	{	if( _iOpenSprite() == FALSE ) return;
	}else 
	{	if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) 
			{	_SetAlphaDegree();
			}else 
			{	_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
	}	}	}

	//SetRect(&rcRect,  sx, sy, sx + szx, sy + szy); // our fictitious sprite bitmap is 
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	m_pDDraw->m_lpBackB4->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutSpriteFastNoColorKeyDst(LPDIRECTDRAWSURFACE7 lpDstS, int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;	
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;

	if (m_bIsSurfaceEmpty == TRUE) 
	{	if( _iOpenSprite() == FALSE ) return;
	}else 
	{	if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) 
			{	_SetAlphaDegree();
			}else 
			{	_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
	}	}	}

	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	lpDstS->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutSpriteFastFrontBuffer(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	// GPU mode has no front buffer surface
	if (m_pDDraw->m_bUseGPU) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;	
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&rcRect,  sx, sy, sx + szx, sy + szy); // our fictitious sprite bitmap is 
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	m_pDDraw->m_lpFrontB4->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutSpriteFastWidth(int sX, int sY, int sFrame, int sWidth, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			// Limit width
			if (sWidth < srcW) srcW = (short)sWidth;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_COLORKEY, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (sWidth < szx)
		szx = sWidth;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&rcRect,  sx, sy, sx + szx, sy + szy); // our fictitious sprite bitmap is 
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	m_pDDraw->m_lpBackB4->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}


void CSprite::iRestore()
{
 HDC     hDC;
	
	if (m_bIsSurfaceEmpty) return;
	if( m_stBrush == NULL ) return;
	if (m_lpSurface->IsLost() == DD_OK) return;

	m_lpSurface->Restore();
	DDSURFACEDESC2  ddsd;
	ddsd.dwSize = 124;
	if (m_lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return;
	m_pSurfaceAddr = (WORD *)ddsd.lpSurface;
	m_lpSurface->Unlock(NULL);
	CMyDib mydib(m_cPakFileName, m_dwBitmapFileStartLoc);
	m_lpSurface->GetDC(&hDC);
	mydib.PaintImage(hDC);
	m_lpSurface->ReleaseDC(hDC);
}

void CSprite::PutShadowSprite(int sX, int sY, int sFrame, DWORD dwTime)
{
	short sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - simplified shadow (no skew transform)
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_SHADOW, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	m_dwRefTime = dwTime;

	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}

	int iSangX, iSangY;
	pSrc = (WORD *)m_pSurfaceAddr + sx + sy*m_sPitch;
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr;// + dX + ((dY+szy-1)*m_pDDraw->m_sBackB4Pitch);

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		for( iy=0 ; iy<szy ; iy+= 3 )
		{
			for( ix=0 ; ix<szx ; ix++ )
			{
				iSangX = (sX+pvx)+ix+(iy-szy)/3;
				iSangY = (sY+pvy)+(iy+szy+szy)/3;
				if (pSrc[ix] != m_wColorKey)
				{
					if( iSangX >= 0 && iSangX < 640 && iSangY >= 0 && iSangY < 427 )
					{
						pDst[iSangY*m_pDDraw->m_sBackB4Pitch+iSangX] = ((pDst[iSangY*m_pDDraw->m_sBackB4Pitch+iSangX] & 0xE79C) >> 2);
					}
				}
			}
			pSrc += m_sPitch + m_sPitch + m_sPitch;
		}
		break;
	case 2:
		for( iy=0 ; iy<szy ; iy+= 3 )
		{
			for( ix=0 ; ix<szx ; ix++ )
			{
				iSangX = sX+pvx+ix+(iy-szy)/3;
				iSangY = sY+pvy+(iy+szy+szy)/3;
				if (pSrc[ix] != m_wColorKey)
				{
					if( iSangX >= 0 && iSangX < 640 && iSangY >= 0 && iSangY < 427 )
					{
						pDst[iSangY*m_pDDraw->m_sBackB4Pitch+iSangX] = ((pDst[iSangY*m_pDDraw->m_sBackB4Pitch+iSangX] & 0x739C) >> 2);
					}
				}
			}
			pSrc += m_sPitch + m_sPitch + m_sPitch;
		}
		break;
	}
	m_bOnCriticalSection = FALSE;
}


void CSprite::PutShadowSpriteClip(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - simplified shadow (no skew transform)
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_SHADOW, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;
	
	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy+szy-1)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY+szy-1)*m_pDDraw->m_sBackB4Pitch);

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		for (iy = 0; iy < szy; iy += 3) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey)
					if ( (dX - (iy/3) + ix)	> 0 )
						pDst[ix] = (pDst[ix] & 0xE79C) >> 2; 
			}
			pSrc -= m_sPitch + m_sPitch + m_sPitch;
			pDst -= m_pDDraw->m_sBackB4Pitch + 1;
		}
		break;

	case 2:
		for (iy = 0; iy < szy; iy += 3) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey)
					if ( (dX - (iy/3) + ix)	> 0 )
						pDst[ix] = (pDst[ix] & 0x739C) >> 2;
			}
			pSrc -= m_sPitch + m_sPitch + m_sPitch;
			pDst -= m_pDDraw->m_sBackB4Pitch + 1;
		}
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSprite(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	// GPU rendering path — DD uses G_lTransRB100: result = min(src + dst, max)
	// This is additive blending (dark pixels add nothing = invisible)
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) {
			LoadToGPU();
		}
		if (m_bIsGPUTexture) {
			m_pDDraw->m_pGPURenderer->QueueSprite(
				m_glTextureID, dX, dY, sx, sy, szx, szy,
				m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 1.0f, 0.0f, 0.0f, 0.0f);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	// DirectDraw fallback path
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB100[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG100[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutTransSprite_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			// alphaDepth=0 means "no transparency" (100% opaque) in the original DD code
			float alpha = (alphaDepth == 0) ? 1.0f : (float)alphaDepth / 100.0f;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, alpha, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB100[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG100[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutTransSprite70(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) {
			LoadToGPU();
		}
		if (m_bIsGPUTexture) {
			m_pDDraw->m_pGPURenderer->QueueSprite(
				m_glTextureID, dX, dY, sx, sy, szx, szy,
				m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.7f, 0.0f, 0.0f, 0.0f);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	// DirectDraw fallback path
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB70[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG70[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB70[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB70[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG70[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB70[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSprite70_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - 70% additive
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.7f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB70[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG70[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB70[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB70[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG70[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB70[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutTransSprite50(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) {
			LoadToGPU();
		}
		if (m_bIsGPUTexture) {
			m_pDDraw->m_pGPURenderer->QueueSprite(
				m_glTextureID, dX, dY, sx, sy, szx, szy,
				m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.5f, 0.0f, 0.0f, 0.0f);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	// DirectDraw fallback path
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB50[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG50[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB50[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB50[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG50[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB50[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSprite50_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - 50% additive
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.5f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB50[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG50[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB50[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB50[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG50[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB50[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutTransSprite25(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - 25% additive
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.25f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB25[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11] <<11) | (G_lTransG25[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5] <<5) | G_lTransRB25[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB25[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG25[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB25[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSprite25_NoColorKey(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - 25% additive
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 0.25f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB25[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11] <<11) | (G_lTransG25[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5] <<5) | G_lTransRB25[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_lTransRB25[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG25[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB25[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSprite2(int sX, int sY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - average blend
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_AVERAGE, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB2[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG2[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB2[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB2[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG2[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB2[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutShiftTransSprite2(int sX, int sY, int shX, int shY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - average blend with shift
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx + shX;
			short srcY = m_stBrush[sFrame].sy + shY;
			short srcW = 128;
			short srcH = 128;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_AVERAGE, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = 128;//m_stBrush[sFrame].szx;
	szy = 128;//m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	sx += shX;
	sy += shY;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB2[(pDst[ix]&0xF800)>>11][(pSrc[ix]&0xF800)>>11]<<11) | (G_lTransG2[(pDst[ix]&0x7E0)>>5][(pSrc[ix]&0x7E0)>>5]<<5) | G_lTransRB2[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_lTransRB2[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10]<<10) | (G_lTransG2[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5]<<5) | G_lTransRB2[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutFadeSprite(short sX, short sY, short sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;
 //int dX,dY,sx,sy,szx,szy,pvx,pvy,sTmp;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - fade/darken effect
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_FADE, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		for (iy = 0; iy < szy; iy++) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey) 
					pDst[ix] = ((pDst[ix] & 0xE79C) >> 2); 
				
			}
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
		}
		break;

	case 2:
		for (iy = 0; iy < szy; iy++) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey)	
					pDst[ix] = ((pDst[ix] & 0x739C) >> 2);
				
			}
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
		}
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutFadeSpriteDst(WORD * pDstAddr, short sPitch, short sX, short sY, short sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 int  ix, iy;
 WORD * pSrc, * pDst;
 //int           iRet, dX,dY,sx,sy,szx,szy,pvx,pvy,sTmp;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;	
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)pDstAddr + dX + ((dY)*sPitch);

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		for (iy = 0; iy < szy; iy++) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey) 
					pDst[ix] = ((pDst[ix] & 0xE79C) >> 2); 
				
			}
			pSrc += m_sPitch;
			pDst += sPitch;
		}
		break;

	case 2:
		for (iy = 0; iy < szy; iy++) {
			for (ix = 0; ix < szx; ix++) {
				if (pSrc[ix] != m_wColorKey)	
					pDst[ix] = ((pDst[ix] & 0x739C) >> 2);
				
			}
			pSrc += m_sPitch;
			pDst += sPitch;
		}
		break;
	}

	m_bOnCriticalSection = FALSE;
}


bool CSprite::_iOpenSprite()
{
  	if (m_lpSurface != NULL) return FALSE;
	m_lpSurface = _pMakeSpriteSurface(); 
	if (m_lpSurface == NULL) return FALSE;
	m_pDDraw->iSetColorKey(m_lpSurface, m_wColorKey);
	m_bIsSurfaceEmpty  = FALSE;
	DDSURFACEDESC2  ddsd;
	ddsd.dwSize = 124;
	if (m_lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) != DD_OK) return FALSE;	
	m_pSurfaceAddr = (WORD *)ddsd.lpSurface;
	m_sPitch = (short)ddsd.lPitch >> 1;	
	m_lpSurface->Unlock(NULL);
	_SetAlphaDegree();
//	m_dwRefTime = timeGetTime();
	return TRUE;
}

void CSprite::_iCloseSprite()
{
	if( m_stBrush == NULL ) return;
	if (m_lpSurface == NULL) return;
	if (m_lpSurface->IsLost() != DD_OK)	return;
	m_lpSurface->Release();
	m_lpSurface = NULL;
	m_bIsSurfaceEmpty = TRUE;
	m_cAlphaDegree = 1;
}

//////////////////////////////////////////////////////////////////////
// GPU Texture Methods
//////////////////////////////////////////////////////////////////////

bool CSprite::LoadToGPU()
{
	if (m_bIsGPUTexture && m_glTextureID != 0) return true;  // Already loaded
	if (m_stBrush == NULL) return false;
	if (m_pDDraw == NULL || m_pDDraw->m_pGPURenderer == NULL) return false;

	// Try stb_image first (handles PNG and 24/32-bit BMP)
	{
		char pathName[32];
		if (memcmp(m_cPakFileName, "lgn_", 4) == 0)
			wsprintf(pathName, "sprites\\%s.lpk", m_cPakFileName);
		else
			wsprintf(pathName, "sprites\\%s.pak", m_cPakFileName);
		HANDLE hFile = CreateFileA(pathName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		// Determine image data size (may not have been set in constructor for last sprite)
		DWORD bitmapSize = m_dwBitmapFileSize;
		if (bitmapSize == 0) {
			DWORD fileSize = GetFileSize(hFile, NULL);
			if (fileSize > m_dwBitmapFileStartLoc)
				bitmapSize = fileSize - m_dwBitmapFileStartLoc;
		}
		if (bitmapSize == 0) { CloseHandle(hFile); return false; }

		SetFilePointer(hFile, m_dwBitmapFileStartLoc, NULL, FILE_BEGIN);
		unsigned char* blob = (unsigned char*)malloc(bitmapSize);
		if (!blob) { CloseHandle(hFile); return false; }
		DWORD nRead = 0;
		ReadFile(hFile, blob, bitmapSize, &nRead, NULL);
		CloseHandle(hFile);
		if (nRead != bitmapSize) { free(blob); return false; }
		int w = 0, h = 0, ch = 0;
		stbi_set_flip_vertically_on_load(0);
		unsigned char* rgba = stbi_load_from_memory(blob, (int)bitmapSize, &w, &h, &ch, 4);
		free(blob);
		if (rgba && w > 0 && h > 0) {
			m_wBitmapSizeX = (WORD)w;
			m_wBitmapSizeY = (WORD)h;

			// If source had no alpha channel, apply color key transparency
			// Read top-left pixel as color key, set matching pixels to alpha=0
			if (ch < 4) {
				uint8_t ckR = rgba[0], ckG = rgba[1], ckB = rgba[2];
				for (int i = 0; i < w * h; i++) {
					if (rgba[i*4+0] == ckR && rgba[i*4+1] == ckG && rgba[i*4+2] == ckB) {
						rgba[i*4+3] = 0;
					}
				}
			}

			// Runtime edge defringe: neutralize color-key contamination near edges.
			// AI upscaling bleeds the color key (often bright green) into nearby
			// pixels. Build a distance map from transparent boundary, then desaturate
			// green-dominant pixels within DEFRINGE_LAYERS of transparency.
			{
				const int DEFRINGE_LAYERS = 5;
				// Distance map: 0 = transparent, 1..N = layers from boundary
				uint8_t* dist = (uint8_t*)calloc(w * h, 1);
				if (dist) {
					// Initialize: transparent=0, opaque=255 (unvisited)
					for (int i = 0; i < w * h; i++)
						dist[i] = (rgba[i*4+3] == 0) ? 0 : 255;
					// BFS-style expansion from transparent boundary
					for (int layer = 1; layer <= DEFRINGE_LAYERS; layer++) {
						for (int y = 0; y < h; y++) {
							for (int x = 0; x < w; x++) {
								int idx = y * w + x;
								if (dist[idx] != 255) continue; // already assigned
								uint8_t prev = (uint8_t)(layer - 1);
								bool nearBoundary = false;
								if (x > 0   && dist[idx-1] == prev) nearBoundary = true;
								if (x < w-1 && dist[idx+1] == prev) nearBoundary = true;
								if (y > 0   && dist[idx-w] == prev) nearBoundary = true;
								if (y < h-1 && dist[idx+w] == prev) nearBoundary = true;
								if (nearBoundary) dist[idx] = (uint8_t)layer;
							}
						}
					}
					// Fix green-dominant pixels near edges
					for (int i = 0; i < w * h; i++) {
						if (dist[i] == 0 || dist[i] > DEFRINGE_LAYERS) continue;
						int r = rgba[i*4+0], g = rgba[i*4+1], b = rgba[i*4+2];
						if (g > r + 25 && g > b + 25 && g > 60) {
							int avg = (r + b) / 2;
							// Stronger correction for pixels closer to edge
							int blend = (dist[i] <= 2) ? 4 : 3; // 80% or 67% toward avg
							rgba[i*4+1] = (uint8_t)((avg * blend + g) / (blend + 1));
						}
					}
					free(dist);
				}
			}

			// Detect sprite scale: compare texture width to max brush source extent
			// With 1x brush values and 2x textures, ratio will be ~2
			if (m_iTotalFrame > 0 && m_stBrush != NULL) {
				int maxSrcRight = 0;
				for (int i = 0; i < m_iTotalFrame; i++) {
					int r = m_stBrush[i].sx + m_stBrush[i].szx;
					if (r > maxSrcRight) maxSrcRight = r;
				}
				if (maxSrcRight > 0) {
					int ratio = (w + maxSrcRight / 2) / maxSrcRight;
					if (ratio >= 2) m_iSpriteScale = ratio;
				}
			}

			m_glTextureID = m_pDDraw->m_pGPURenderer->CreateTexture(rgba, m_wBitmapSizeX, m_wBitmapSizeY);
			stbi_image_free(rgba);
			if (m_glTextureID == 0) return false;
			m_bIsGPUTexture = true;
			return true;
		}
		// stbi failed (e.g. 16-bit BMP) — fall through to CMyDib path
	}

	// Fallback: CMyDib handles 16-bit BMP and other formats stbi can't decode
	CMyDib mydib(m_cPakFileName, m_dwBitmapFileStartLoc);
	if (mydib.m_lpDib == NULL) return false;

	m_wBitmapSizeX = mydib.m_wWidthX;
	m_wBitmapSizeY = mydib.m_wWidthY;

	LPBITMAPINFOHEADER bmpHeader = (LPBITMAPINFOHEADER)mydib.m_lpDib;
	WORD bitsPerPixel = (WORD)bmpHeader->biBitCount;

	// Get pointer to pixel data (after header and palette)
	LPSTR pPixelData = mydib.m_lpDib + bmpHeader->biSize + mydib.m_wColorNums * sizeof(RGBQUAD);

	// Allocate RGBA buffer
	int totalPixels = m_wBitmapSizeX * m_wBitmapSizeY;
	uint8_t* rgbaData = new uint8_t[totalPixels * 4];

	// Calculate row stride (BMP rows are padded to 4-byte boundaries)
	int rowStride = ((m_wBitmapSizeX * bitsPerPixel + 31) / 32) * 4;

	if (bitsPerPixel == 8) {
		// 8-bit palette-based image
		RGBQUAD* palette = (RGBQUAD*)(mydib.m_lpDib + bmpHeader->biSize);
		uint8_t* pSrc = (uint8_t*)pPixelData;

		// Get color key from top-left visual pixel (last row of bottom-up BMP data)
		uint8_t* topVisualRow = pSrc + (m_wBitmapSizeY - 1) * rowStride;
		uint8_t colorKeyIndex = topVisualRow[0];

		for (int y = 0; y < m_wBitmapSizeY; y++) {
			// BMP is bottom-up, flip to top-down for OpenGL
			int srcY = m_wBitmapSizeY - 1 - y;
			uint8_t* srcRow = pSrc + srcY * rowStride;

			for (int x = 0; x < m_wBitmapSizeX; x++) {
				uint8_t index = srcRow[x];
				int dstIdx = (y * m_wBitmapSizeX + x) * 4;

				// Look up in palette (RGBQUAD is BGRA order)
				rgbaData[dstIdx + 0] = palette[index].rgbRed;
				rgbaData[dstIdx + 1] = palette[index].rgbGreen;
				rgbaData[dstIdx + 2] = palette[index].rgbBlue;
				rgbaData[dstIdx + 3] = (index == colorKeyIndex) ? 0 : 255;
			}
		}
	} else if (bitsPerPixel == 24) {
		// 24-bit RGB image
		uint8_t* pSrc = (uint8_t*)pPixelData;

		// Get color key from top-left visual pixel (last row of bottom-up BMP data, BGR order)
		uint8_t* topVisualRow = pSrc + (m_wBitmapSizeY - 1) * rowStride;
		uint8_t ckB = topVisualRow[0], ckG = topVisualRow[1], ckR = topVisualRow[2];

		for (int y = 0; y < m_wBitmapSizeY; y++) {
			int srcY = m_wBitmapSizeY - 1 - y;
			uint8_t* srcRow = pSrc + srcY * rowStride;

			for (int x = 0; x < m_wBitmapSizeX; x++) {
				int srcIdx = x * 3;
				int dstIdx = (y * m_wBitmapSizeX + x) * 4;

				uint8_t b = srcRow[srcIdx + 0];
				uint8_t g = srcRow[srcIdx + 1];
				uint8_t r = srcRow[srcIdx + 2];

				rgbaData[dstIdx + 0] = r;
				rgbaData[dstIdx + 1] = g;
				rgbaData[dstIdx + 2] = b;
				rgbaData[dstIdx + 3] = (r == ckR && g == ckG && b == ckB) ? 0 : 255;
			}
		}
	} else if (bitsPerPixel == 4) {
		// 4-bit palette-based image (16 colors)
		RGBQUAD* palette = (RGBQUAD*)(mydib.m_lpDib + bmpHeader->biSize);
		uint8_t* pSrc = (uint8_t*)pPixelData;

		// Get color key from top-left visual pixel (last row of bottom-up BMP data, high nibble)
		uint8_t* topVisualRow = pSrc + (m_wBitmapSizeY - 1) * rowStride;
		uint8_t colorKeyIndex = (topVisualRow[0] >> 4) & 0x0F;

		for (int y = 0; y < m_wBitmapSizeY; y++) {
			int srcY = m_wBitmapSizeY - 1 - y;
			uint8_t* srcRow = pSrc + srcY * rowStride;

			for (int x = 0; x < m_wBitmapSizeX; x++) {
				// Each byte contains 2 pixels: high nibble first, then low nibble
				int byteIdx = x / 2;
				uint8_t index;
				if (x % 2 == 0) {
					index = (srcRow[byteIdx] >> 4) & 0x0F;  // High nibble
				} else {
					index = srcRow[byteIdx] & 0x0F;  // Low nibble
				}

				int dstIdx = (y * m_wBitmapSizeX + x) * 4;

				rgbaData[dstIdx + 0] = palette[index].rgbRed;
				rgbaData[dstIdx + 1] = palette[index].rgbGreen;
				rgbaData[dstIdx + 2] = palette[index].rgbBlue;
				rgbaData[dstIdx + 3] = (index == colorKeyIndex) ? 0 : 255;
			}
		}
	} else if (bitsPerPixel == 16) {
		// 16-bit RGB image — detect RGB555 vs RGB565 from BMP header
		uint16_t* pSrc = (uint16_t*)pPixelData;
		// Get color key from top-left visual pixel (last row of bottom-up BMP data)
		uint16_t* topVisualRow16 = (uint16_t*)((uint8_t*)pSrc + (m_wBitmapSizeY - 1) * rowStride);
		m_wColorKey = topVisualRow16[0];

		// Detect pixel format: BI_BITFIELDS has explicit masks, BI_RGB defaults to RGB555
		bool bIsRGB565 = false;
		if (bmpHeader->biCompression == BI_BITFIELDS) {
			// Bit masks follow the BITMAPINFOHEADER
			DWORD* pMasks = (DWORD*)(mydib.m_lpDib + bmpHeader->biSize);
			DWORD dwRedMask = pMasks[0];
			bIsRGB565 = (dwRedMask == 0xF800);  // RGB565: R=F800, G=07E0, B=001F
			// else RGB555: R=7C00, G=03E0, B=001F
		}
		// BI_RGB with 16-bit = RGB555 by Windows standard

		for (int y = 0; y < m_wBitmapSizeY; y++) {
			int srcY = m_wBitmapSizeY - 1 - y;
			uint16_t* srcRow = (uint16_t*)((uint8_t*)pSrc + srcY * rowStride);

			for (int x = 0; x < m_wBitmapSizeX; x++) {
				uint16_t pixel = srcRow[x];
				int dstIdx = (y * m_wBitmapSizeX + x) * 4;

				if (bIsRGB565) {
					// RGB565: RRRRRGGGGGGBBBBB (5-6-5)
					uint8_t r5 = (pixel >> 11) & 0x1F;
					uint8_t g6 = (pixel >> 5) & 0x3F;
					uint8_t b5 = pixel & 0x1F;
					rgbaData[dstIdx + 0] = (r5 << 3) | (r5 >> 2);
					rgbaData[dstIdx + 1] = (g6 << 2) | (g6 >> 4);
					rgbaData[dstIdx + 2] = (b5 << 3) | (b5 >> 2);
				} else {
					// RGB555: 0RRRRRGGGGGBBBBB (1-5-5-5)
					uint8_t r5 = (pixel >> 10) & 0x1F;
					uint8_t g5 = (pixel >> 5) & 0x1F;
					uint8_t b5 = pixel & 0x1F;
					rgbaData[dstIdx + 0] = (r5 << 3) | (r5 >> 2);
					rgbaData[dstIdx + 1] = (g5 << 3) | (g5 >> 2);
					rgbaData[dstIdx + 2] = (b5 << 3) | (b5 >> 2);
				}
				rgbaData[dstIdx + 3] = (pixel == m_wColorKey) ? 0 : 255;
			}
		}
	} else if (bitsPerPixel == 1) {
		// 1-bit monochrome image
		RGBQUAD* palette = (RGBQUAD*)(mydib.m_lpDib + bmpHeader->biSize);
		uint8_t* pSrc = (uint8_t*)pPixelData;

		// Color key is index 0 (usually black/background)
		uint8_t colorKeyIndex = 0;

		for (int y = 0; y < m_wBitmapSizeY; y++) {
			int srcY = m_wBitmapSizeY - 1 - y;
			uint8_t* srcRow = pSrc + srcY * rowStride;

			for (int x = 0; x < m_wBitmapSizeX; x++) {
				int byteIdx = x / 8;
				int bitIdx = 7 - (x % 8);  // MSB first
				uint8_t index = (srcRow[byteIdx] >> bitIdx) & 0x01;

				int dstIdx = (y * m_wBitmapSizeX + x) * 4;

				rgbaData[dstIdx + 0] = palette[index].rgbRed;
				rgbaData[dstIdx + 1] = palette[index].rgbGreen;
				rgbaData[dstIdx + 2] = palette[index].rgbBlue;
				rgbaData[dstIdx + 3] = (index == colorKeyIndex) ? 0 : 255;
			}
		}
	} else if (bitsPerPixel == 32) {
		// 32-bit BGRA image — use alpha channel directly (no color key)
		uint8_t* pSrc = (uint8_t*)pPixelData;
		for (int y = 0; y < m_wBitmapSizeY; y++) {
			int srcY = m_wBitmapSizeY - 1 - y;
			uint8_t* srcRow = pSrc + srcY * rowStride;
			for (int x = 0; x < m_wBitmapSizeX; x++) {
				int srcIdx = x * 4;
				int dstIdx = (y * m_wBitmapSizeX + x) * 4;
				rgbaData[dstIdx + 0] = srcRow[srcIdx + 2];  // R
				rgbaData[dstIdx + 1] = srcRow[srcIdx + 1];  // G
				rgbaData[dstIdx + 2] = srcRow[srcIdx + 0];  // B
				rgbaData[dstIdx + 3] = srcRow[srcIdx + 3];  // A
			}
		}
	} else {
		// Unsupported format - log and fail gracefully
		OutputDebugString("LoadToGPU: Unsupported BMP bit depth\n");
		delete[] rgbaData;
		return false;
	}

	// Detect sprite scale for CMyDib path (same logic as stbi path)
	if (m_iTotalFrame > 0 && m_stBrush != NULL) {
		int maxSrcRight = 0;
		for (int i = 0; i < m_iTotalFrame; i++) {
			int r = m_stBrush[i].sx + m_stBrush[i].szx;
			if (r > maxSrcRight) maxSrcRight = r;
		}
		if (maxSrcRight > 0) {
			int ratio = (m_wBitmapSizeX + maxSrcRight / 2) / maxSrcRight;
			if (ratio >= 2) m_iSpriteScale = ratio;
		}
	}

	// Create OpenGL texture
	m_glTextureID = m_pDDraw->m_pGPURenderer->CreateTexture(rgbaData, m_wBitmapSizeX, m_wBitmapSizeY);

	delete[] rgbaData;

	if (m_glTextureID == 0) {
		return false;
	}

	m_bIsGPUTexture = true;
	return true;
}

void CSprite::UnloadFromGPU()
{
	// Note: Individual glDeleteTextures calls are skipped here.
	// During shutdown, the GL context destruction (wglDeleteContext) automatically
	// frees all textures. Calling DeleteTexture after the renderer is destroyed
	// would access freed memory and cause heap corruption.
	// During gameplay, sprites are loaded once and kept; no mid-game recycling needed.
	m_glTextureID = 0;
	m_bIsGPUTexture = false;
}

void CSprite::PutSpriteRGB(int sX, int sY, int sFrame, int sRed, int sGreen, int sBlue, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy, iRedPlus255, iGreenPlus255, iBluePlus255;
	WORD * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;

	if ((szx == 0) || (szy == 0)) return;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	// GPU rendering path
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) {
			LoadToGPU();
		}
		if (m_bIsGPUTexture) {
			// Convert color values from DD pixel-format space to 0-1 range.
			// sRed/sBlue are in 5-bit space (0-31), sGreen in 6-bit for RGB565 or 5-bit for RGB555.
			float rMax = 31.0f;
			float gMax = (m_pDDraw->m_cPixelFormat == 1 || m_pDDraw->m_cPixelFormat == 3) ? 63.0f : 31.0f;
			float bMax = 31.0f;
			float colorR = sRed / rMax;
			float colorG = sGreen / gMax;
			float colorB = sBlue / bMax;
			// DEBUG: Log PutSpriteRGB values (once)
			{
				static int dbgCount = 0;
				if (dbgCount < 20) {
					FILE *fDbg = fopen("color_debug.txt", "a");
					if (fDbg) {
						fprintf(fDbg, "PutSpriteRGB GPU: sRGB=(%d,%d,%d) pixFmt=%d colorRGB=(%.3f,%.3f,%.3f)\n",
							sRed, sGreen, sBlue, m_pDDraw->m_cPixelFormat, colorR, colorG, colorB);
						fclose(fDbg);
					}
					dbgCount++;
				}
			}
			m_pDDraw->m_pGPURenderer->QueueSprite(
				m_glTextureID, dX, dY, sx, sy, szx, szy,
				m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_TINTED, 1.0f, colorR, colorG, colorB);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	// DirectDraw fallback path
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	iRedPlus255   = sRed +255;
	iGreenPlus255 = sGreen +255;
	iBluePlus255  = sBlue +255;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)(( G_iAddTable31[(pSrc[ix]&0xF800)>>11][iRedPlus255] <<11) | ( G_iAddTable63[(pSrc[ix]&0x7E0)>>5][iGreenPlus255] <<5) | G_iAddTable31[(pSrc[ix]&0x1F)][iBluePlus255]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_iAddTable31[(pSrc[ix]&0x7C00)>>10][iRedPlus255]<<10) | (G_iAddTable31[(pSrc[ix]&0x3E0)>>5][iGreenPlus255]<<5) | G_iAddTable31[(pSrc[ix]&0x1F)][iBluePlus255]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}


void CSprite::PutTransSpriteRGB(int sX, int sY, int sFrame, int sRed, int sGreen, int sBlue, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 short ix, iy, iRedPlus255, iGreenPlus255, iBluePlus255;
 WORD  * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - additive color blend (texture + tint added to destination)
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			// Normalize integer tint values to float range (original is 5-bit: -31 to +31)
			// Shader clamps tex+tint to [0,1], so negative tints darken naturally
			float normR = sRed / 32.0f;
			float normG = sGreen / 32.0f;
			float normB = sBlue / 32.0f;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 1.0f, normR, normG, normB);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	if ((szx == 0) || (szy == 0)) return;

	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	iRedPlus255   = sRed +255;
	iGreenPlus255 = sGreen +255;
	iBluePlus255  = sBlue +255;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0xF800)>>11][((pSrc[ix]&0xF800)>>11)] + iRedPlus255][(pDst[ix]&0xF800)>>11]<<11) | (G_iAddTransTable63[G_lTransG100[(pDst[ix]&0x7E0)>>5][((pSrc[ix]&0x7E0)>>5)] + iGreenPlus255][(pDst[ix]&0x7E0)>>5]<<5) | G_iAddTransTable31[m_pDDraw->m_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)] + iBluePlus255][(pDst[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					pDst[ix] = (WORD)((G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10] +iRedPlus255][(pDst[ix]&0x7C00)>>10]<<10) | (G_iAddTransTable31[G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5] +iGreenPlus255][(pDst[ix]&0x3E0)>>5]<<5) | G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)] +iBluePlus255][(pDst[ix]&0x1F)]);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutTransSpriteRGB_NoColorKey(int sX, int sY, int sFrame, int sRed, int sGreen, int sBlue, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 short ix, iy, iRedPlus255, iGreenPlus255, iBluePlus255;
 WORD  * pSrc, * pDst;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - additive color blend (texture + tint added to destination)
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			// Normalize integer tint values to float range (original is 5-bit: -31 to +31)
			// Shader clamps tex+tint to [0,1], so negative tints darken naturally
			float normR = sRed / 32.0f;
			float normG = sGreen / 32.0f;
			float normB = sBlue / 32.0f;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_ADDITIVE, 1.0f, normR, normG, normB);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	m_dwRefTime = dwTime;
	
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);

	if ((szx == 0) || (szy == 0)) return;

	iRedPlus255   = sRed +255;
	iGreenPlus255 = sGreen +255;
	iBluePlus255  = sBlue +255;

	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0xF800)>>11][((pSrc[ix]&0xF800)>>11)] + iRedPlus255][(pDst[ix]&0xF800)>>11]<<11) | (G_iAddTransTable63[G_lTransG100[(pDst[ix]&0x7E0)>>5][((pSrc[ix]&0x7E0)>>5)] + iGreenPlus255][(pDst[ix]&0x7E0)>>5]<<5) | G_iAddTransTable31[m_pDDraw->m_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)] + iBluePlus255][(pDst[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;

	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				pDst[ix] = (WORD)((G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0x7C00)>>10][(pSrc[ix]&0x7C00)>>10] +iRedPlus255][(pDst[ix]&0x7C00)>>10]<<10) | (G_iAddTransTable31[G_lTransG100[(pDst[ix]&0x3E0)>>5][(pSrc[ix]&0x3E0)>>5] +iGreenPlus255][(pDst[ix]&0x3E0)>>5]<<5) | G_iAddTransTable31[G_lTransRB100[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F)] +iBluePlus255][(pDst[ix]&0x1F)]);
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}

	m_bOnCriticalSection = FALSE;
}

void CSprite::_GetSpriteRect(int sX, int sY, int sFrame)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;	

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	
	m_rcBound.top    = -1;
	m_rcBound.bottom = -1;
	m_rcBound.left   = -1;
	m_rcBound.right  = -1;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	m_sPivotX = pvx;
	m_sPivotY = pvy;
}

void CSprite::_SetAlphaDegree()
{
 WORD * pSrc, wR, wG, wB, wTemp, ix, iy;
 int iR, iG, iB, sRed, sGreen, sBlue;
 
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_bOnCriticalSection = TRUE;
	if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
		
		m_cAlphaDegree = G_cSpriteAlphaDegree;
		switch (m_cAlphaDegree) {
		case 1:
			sRed = sGreen = sBlue = 0;
			break;

		case 2:
			sRed   = -3;
			sGreen = -3;
			sBlue  =  2;
			break;
		}

		pSrc = (WORD *)m_pSurfaceAddr;
		
		switch (m_pDDraw->m_cPixelFormat) {
		case 1:
			for (iy = 0; iy < m_wBitmapSizeY; iy++) 
			{	for (ix = 0; ix < m_wBitmapSizeX; ix++) 
				{	if (pSrc == NULL) return; 
					if (pSrc[ix] != m_wColorKey) 
					{	wR = (WORD)(pSrc[ix]&0xF800)>>11;
						wG = (WORD)(pSrc[ix]&0x7E0)>>5;
						wB = (WORD)(pSrc[ix]&0x1F);
						iR = (int)wR + sRed;
						iG = (int)wG + sGreen;
						iB = (int)wB + sBlue;
						
						if (iR < 0) iR = 0;
						else if (iR > 31) iR = 31;
						if (iG < 0) iG = 0;
						else if (iG > 63) iG = 63;
						if (iB < 0) iB = 0;
						else if (iB > 31) iB = 31;
						
						wTemp = (WORD)((iR<<11) | (iG<<5) | iB);
						if (wTemp != m_wColorKey)
							 pSrc[ix] = wTemp;
						else pSrc[ix] = (WORD)((iR<<11) | (iG<<5) | (iB+1));
				}	}
				pSrc += m_sPitch;
			}
			break;
			
		case 2:
			for (iy = 0; iy < m_wBitmapSizeY; iy++) 
			{	for (ix = 0; ix < m_wBitmapSizeX; ix++) 
				{	if (pSrc == NULL) return; 
					if (pSrc[ix] != m_wColorKey)	
					{	wR = (WORD)(pSrc[ix]&0x7C00)>>10;
						wG = (WORD)(pSrc[ix]&0x3E0)>>5;
						wB = (WORD)(pSrc[ix]&0x1F);
						iR = (int)wR + sRed;
						iG = (int)wG + sGreen;
						iB = (int)wB + sBlue;						
						if (iR < 0) iR = 0;
						else if (iR > 31) iR = 31;
						if (iG < 0) iG = 0;
						else if (iG > 31) iG = 31;
						if (iB < 0) iB = 0;
						else if (iB > 31) iB = 31;						
						wTemp = (WORD)((iR<<10) | (iG<<5) | iB);
						if (wTemp != m_wColorKey)
							 pSrc[ix] = wTemp;
						else pSrc[ix] = (WORD)((iR<<10) | (iG<<5) | (iB+1));				
				}	}
				pSrc += m_sPitch;
			}
			break;	
	}	}

	m_bOnCriticalSection = FALSE;
}

BOOL CSprite::_bCheckCollison(int sX, int sY, short sFrame, int msX, int msY)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
	int  ix, iy;
	WORD * pSrc;
	int  tX, tY;
	
	if( this == NULL ) return FALSE;
	if( m_stBrush == NULL ) return FALSE;
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return FALSE;
	if( m_bIsSurfaceEmpty == TRUE && !(m_pDDraw->m_bUseGPU) ) return FALSE;
	if( msX < m_pDDraw->m_rcClipArea.left+3 ) return FALSE;
	if( msX > m_pDDraw->m_rcClipArea.right-3 ) return FALSE;
	if( msY < m_pDDraw->m_rcClipArea.top+3 ) return FALSE;
	if( msY > m_pDDraw->m_rcClipArea.bottom-3 ) return FALSE;

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

  	dX = sX + pvx;
	dY = sY + pvy;

	if( msX < dX ) return FALSE;
	if( msX > dX+szx ) return FALSE;
	if( msY < dY ) return FALSE;
	if( msY > dY+szy ) return FALSE;

//	if (dX < m_pDDraw->m_rcClipArea.left+3) return FALSE;
//	if (dX+szx > m_pDDraw->m_rcClipArea.right-3) return FALSE;
//	if (dY < m_pDDraw->m_rcClipArea.top+3) return FALSE;
//	if (dY+szy > m_pDDraw->m_rcClipArea.bottom-3) return FALSE;

	if (dX < m_pDDraw->m_rcClipArea.left+3)
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left+3 - dX);
		szx = szx - (m_pDDraw->m_rcClipArea.left+3 - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return FALSE;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left+3;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right-3)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right-3);
		if (szx < 0) {
			m_rcBound.top = -1;
			return FALSE;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top+3)
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top+3 - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top+3 - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return FALSE;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top+3;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom-3)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom-3);
		if (szy < 0) {
			m_rcBound.top = -1;
			return FALSE;
		}
	}

	SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);

	// GPU mode: bounding box passed, no pixel data available for sub-pixel test
	if( m_bIsSurfaceEmpty == TRUE ) return TRUE;

	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	tX = dX;
	tY = dY;


//	pSrc += m_sPitch * ( msY - tY );
//	if( pSrc[msX-tX] != m_wColorKey ) return TRUE;
//	else return FALSE;

	if( msY < tY + 3 ) return FALSE;
	if( msX < tX + 3 ) return FALSE;
	pSrc += m_sPitch * ( msY - tY - 3 );
	for( iy=0 ; iy<=6 ; iy++ )
	{
		for( ix=msX-tX-3 ; ix<=msX-tX+3 ; ix++ )
		{
			if( pSrc[ix] != m_wColorKey ) return TRUE;
		}
		pSrc += m_sPitch;
	}
	return FALSE;
}

void CSprite::PutShiftSpriteFast(int sX, int sY, int shX, int shY, int sFrame, DWORD dwTime)
{
	short dX,dY,sx,sy,szx,szy,pvx,pvy;
 RECT rcRect;
	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - shifted sprite
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx + shX;
			short srcY = m_stBrush[sFrame].sy + shY;
			short srcW = 128;
			short srcH = 128;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_OPAQUE, 1.0f, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = 128;//m_stBrush[sFrame].szx;
	szy = 128;//m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;

	sx += shX;
	sy += shY;

  	dX = sX + pvx;
	dY = sY + pvy;

	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}

	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy <= 0) {
			m_rcBound.top = -1;
			return;
		}
	}
	
	m_dwRefTime = dwTime;

	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}

	//SetRect(&rcRect,  sx, sy, sx + szx, sy + szy); // our fictitious sprite bitmap is 
	//SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
	rcRect.left = sx;
	rcRect.top  = sy;
	rcRect.right  = sx + szx;
	rcRect.bottom = sy + szy;

	m_rcBound.left = dX;
	m_rcBound.top  = dY;
	m_rcBound.right  = dX + szx;
	m_rcBound.bottom = dY + szy;

	m_pDDraw->m_lpBackB4->BltFast( dX, dY, m_lpSurface, &rcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT );

	m_bOnCriticalSection = FALSE;
}

void CSprite::PutRevTransSprite(int sX, int sY, int sFrame, DWORD dwTime, int alphaDepth)
{
	int  ix, iy;
	int  iR, iG, iB;
	WORD * pSrc, * pDst;
	int dX,dY,sx,sy,szx,szy,pvx,pvy;//,sTmp;

	if( this == NULL ) return;
	if( m_stBrush == NULL ) return;
	m_rcBound.top = -1; // Fix by Snoopy.... (Reco at mine)
	if ((m_iTotalFrame-1 < sFrame) || (sFrame < 0)) return;
	m_bOnCriticalSection = TRUE;

	// GPU rendering path - reverse alpha blend
	if (m_pDDraw->m_bUseGPU && m_pDDraw->m_pGPURenderer != NULL) {
		if (!m_bIsGPUTexture) LoadToGPU();
		if (m_bIsGPUTexture) {
			m_dwRefTime = dwTime;
			short srcX = m_stBrush[sFrame].sx;
			short srcY = m_stBrush[sFrame].sy;
			short srcW = m_stBrush[sFrame].szx;
			short srcH = m_stBrush[sFrame].szy;
			short pivotX = m_stBrush[sFrame].pvx;
			short pivotY = m_stBrush[sFrame].pvy;
			int destX = sX + pivotX;
			int destY = sY + pivotY;
			m_rcBound.left = destX;
			m_rcBound.top = destY;
			m_rcBound.right = destX + srcW;
			m_rcBound.bottom = destY + srcH;
			// Subtractive blend: result = dst - src per channel (DD _CalcMinValue)
			// alphaDepth increases subtraction strength (0 = normal)
			float alpha = 1.0f + ((float)alphaDepth / 32.0f);
			m_pDDraw->m_pGPURenderer->QueueSprite(m_glTextureID, destX, destY,
				srcX, srcY, srcW, srcH, m_wBitmapSizeX, m_wBitmapSizeY, m_iSpriteScale,
				BLEND_SUBTRACTIVE, alpha, 0, 0, 0);
		}
		m_bOnCriticalSection = FALSE;
		return;
	}

	sx  = m_stBrush[sFrame].sx;
	sy  = m_stBrush[sFrame].sy;
	szx = m_stBrush[sFrame].szx;
	szy = m_stBrush[sFrame].szy;
	pvx = m_stBrush[sFrame].pvx;
	pvy = m_stBrush[sFrame].pvy;
	
	dX = sX + pvx;
	dY = sY + pvy;
		
	if (dX < m_pDDraw->m_rcClipArea.left) 								  
	{
		sx = sx	+ (m_pDDraw->m_rcClipArea.left - dX);							
		szx = szx - (m_pDDraw->m_rcClipArea.left - dX);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
		dX = (short)m_pDDraw->m_rcClipArea.left;
	}
	else if (dX+szx > m_pDDraw->m_rcClipArea.right)
	{
		szx = szx - ((dX+szx) - (short)m_pDDraw->m_rcClipArea.right);
		if (szx < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
		
	if (dY < m_pDDraw->m_rcClipArea.top) 								  
	{
		sy = sy	+ (m_pDDraw->m_rcClipArea.top - dY);
		szy = szy - (m_pDDraw->m_rcClipArea.top - dY);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
		dY = (short)m_pDDraw->m_rcClipArea.top;
	}
	else if (dY+szy > m_pDDraw->m_rcClipArea.bottom)
	{
		szy = szy - ((dY+szy) - (short)m_pDDraw->m_rcClipArea.bottom);
		if (szy < 0) {
			m_rcBound.top = -1;
			return;
		}
	}
		
	m_dwRefTime = dwTime;
		
	if (m_bIsSurfaceEmpty == TRUE) {
		if( _iOpenSprite() == FALSE ) return;
	}
	else {
		if (m_bAlphaEffect && (m_cAlphaDegree != G_cSpriteAlphaDegree)) {
			if (G_cSpriteAlphaDegree == 2) {
				_SetAlphaDegree();
			}
			else {
				_iCloseSprite();
				if( _iOpenSprite() == FALSE ) return;
			}
		}
	}
	
	SetRect(&m_rcBound, dX, dY, dX + szx, dY + szy);
		
	pSrc = (WORD *)m_pSurfaceAddr + sx + ((sy)*m_sPitch);
	pDst = (WORD *)m_pDDraw->m_pBackB4Addr + dX + ((dY)*m_pDDraw->m_sBackB4Pitch);
	
	if ((szx == 0) || (szy == 0)) return;
		
	switch (m_pDDraw->m_cPixelFormat) {
	case 1:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					iR = (int)m_pDDraw->m_lFadeRB[((pDst[ix]&0xF800)>>11)][((pSrc[ix]&0xF800)>>11) +alphaDepth];
					iG = (int)m_pDDraw->m_lFadeG[(pDst[ix]&0x7E0)>>5][((pSrc[ix]&0x7E0)>>5) +alphaDepth +alphaDepth];
					iB = (int)m_pDDraw->m_lFadeRB[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F) +alphaDepth];
					pDst[ix] = (WORD)((iR<<11) | (iG<<5) | iB);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
		
	case 2:
		iy =0;
		do {
			ix = 0;
			do {
				if (pSrc[ix] != m_wColorKey) {
					iR = (int)m_pDDraw->m_lFadeRB[(pDst[ix]&0x7C00)>>10][((pSrc[ix]&0x7C00)>>10) +alphaDepth];
					iG = (int)m_pDDraw->m_lFadeG[(pDst[ix]&0x3E0)>>5][((pSrc[ix]&0x3E0)>>5) +alphaDepth];
					iB = (int)m_pDDraw->m_lFadeRB[(pDst[ix]&0x1F)][(pSrc[ix]&0x1F) +alphaDepth];
					pDst[ix] = (WORD)((iR<<10) | (iG<<5) | iB);
				}
				
				ix++;
			} while (ix < szx);
			pSrc += m_sPitch;
			pDst += m_pDDraw->m_sBackB4Pitch;
			iy++;
		} while (iy < szy);
		break;
	}
	m_bOnCriticalSection = FALSE;
}

void ReadFramePositions(HANDLE hPakFile, std::vector<int> & framePositions, int frames)
{
	DWORD * dwp, count;
	char * fileHeader = new char[frames*8 +8];
	SetFilePointer(hPakFile, 24, NULL, FILE_BEGIN);
	ReadFile(hPakFile, fileHeader,  frames *8, &count, NULL);
	dwp = (DWORD *) fileHeader;
	for(int i = 0; i < frames; i++, dwp+=2)
	{
		framePositions.push_back(*dwp);
	}
	delete [] fileHeader;
}

