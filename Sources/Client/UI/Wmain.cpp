// -------------------------------------------------------------- 
//                      Helbreath Client 						  
//
//                      1998.10 by Soph
//
// --------------------------------------------------------------


#define STB_IMAGE_IMPLEMENTATION
#include "..\stb_image.h"
#define SHADERS_H
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h> 
#include <winbase.h>
#include <mmsystem.h>
#include <process.h>
#include "..\res\resource.h"
#include "..\net\XSocket.h"
#include "winmain.h"
#include "..\Game.h"
#include "..\GlobalDef.h"
#include "..\include\GL\glew.h"
// Note: GLFW no longer used for windowing - WGL creates OpenGL context on Win32 window
#include "..\include\GL\wglew.h"
extern "C" __declspec( dllimport) int __FindHackingDll__(char *);

// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500
#define WM_USER_CALCSOCKETEVENT WM_USER + 600
#define GLEW_STATIC // Only define this if you're linking against the static version of GLEW

int				G_iAddTable31[64][510], G_iAddTable63[64][510]; 
int				G_iAddTransTable31[510][64], G_iAddTransTable63[510][64]; 

long    G_lTransG100[64][64], G_lTransRB100[64][64];
long    G_lTransG70[64][64], G_lTransRB70[64][64];
long    G_lTransG50[64][64], G_lTransRB50[64][64];
long    G_lTransG25[64][64], G_lTransRB25[64][64];
long    G_lTransG2[64][64], G_lTransRB2[64][64];

char			szAppClass[32];
HWND			G_hWnd = NULL;
HWND			G_hEditWnd = NULL;
HINSTANCE       G_hInstance = NULL;
MMRESULT		G_mmTimer;
char   G_cSpriteAlphaDegree;
class CGame * G_pGame;
class XSocket * G_pCalcSocket = NULL;
BOOL  G_bIsCalcSocketConnected = TRUE;
DWORD G_dwCalcSocketTime = NULL, G_dwCalcSocketSendTime = NULL;

// Note: GLFW no longer used - OpenGL context created via WGL on Win32 window

char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120], G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];

// --------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam, LPARAM lParam)
{ 
	if(G_pGame->GetText( hWnd, message, wParam, lParam)) return 0;

	switch (message) {
	case WM_USER_CALCSOCKETEVENT:
		G_pGame->_CalcSocketClosed();
		break;
	
	case WM_CLOSE:
		if ( (G_pGame->m_cGameMode == GAMEMODE_ONMAINGAME) && ( G_pGame->m_bForceDisconn == FALSE ) )
		{

			if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > (char)G_pGame->m_iLogOutTimer) {
				G_pGame->m_cLogOutCount = (char)G_pGame->m_iLogOutTimer;
				G_pGame->m_dwLogOutCountTime = timeGetTime();
				G_pGame->m_sLogOutStartX = G_pGame->m_sPlayerX;
				G_pGame->m_sLogOutStartY = G_pGame->m_sPlayerY;
			}

		}
			else if (G_pGame->m_cGameMode == GAMEMODE_ONLOADING) return (DefWindowProc(hWnd, message, wParam, lParam));
			else if (G_pGame->m_cGameMode == GAMEMODE_ONMAINMENU) G_pGame->ChangeGameMode(GAMEMODE_ONQUIT);
		break;
	
	case WM_SYSCOMMAND:
		if((wParam&0xFFF0)==SC_SCREENSAVE || (wParam&0xFFF0)==SC_MONITORPOWER) 
			return 0; 
		return DefWindowProc(hWnd, message, wParam, lParam);
			
	case WM_USER_TIMERSIGNAL:
		G_pGame->OnTimer();
		break;

	case WM_KEYDOWN:
		G_pGame->OnKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		
	case WM_KEYUP:
		G_pGame->OnKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_SYSKEYDOWN:
		G_pGame->OnSysKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_SYSKEYUP:
		G_pGame->OnSysKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_ACTIVATEAPP:
		if( wParam == 0 ) 
		{	G_pGame->m_bIsProgramActive = FALSE;
			G_pGame->m_DInput.SetAcquire(FALSE);
		}else 
		{	G_pGame->m_bIsProgramActive = TRUE;
			G_pGame->m_DInput.SetAcquire(TRUE);
			G_pGame->m_bCtrlPressed = FALSE;
			
			if (G_pGame->bCheckImportantFile() == FALSE) 
			{	MessageBox(G_pGame->m_hWnd, "File checksum error! Get Update again please!", "ERROR1", MB_ICONEXCLAMATION | MB_OK);
				PostQuitMessage(0);
				return 0;
			}			
			if (__FindHackingDll__("CRCCHECK") != 1) 
			{	G_pGame->ChangeGameMode(GAMEMODE_ONQUIT);
				return NULL;
		}	}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_SETCURSOR:
		SetCursor(NULL);
		return TRUE;

	case WM_DESTROY:
		OnDestroy();
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_USER_GAMESOCKETEVENT:
		G_pGame->OnGameSocketEvent(wParam, lParam);
		break;

	case WM_USER_LOGSOCKETEVENT:
		G_pGame->OnLogSocketEvent(wParam, lParam);
		break;
		
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		ValidateRect(hWnd, NULL);
		return 0;

	case WM_SIZE: {
		// When window is resized, update GPU renderer viewport/projection so scaling stays correct.
		if (G_pGame != NULL && G_pGame->m_DDraw.m_bUseGPU && G_pGame->m_DDraw.m_pGPURenderer != NULL) {
			int w = (int)(short)LOWORD(lParam);
			int h = (int)(short)HIWORD(lParam);
			if (w > 0 && h > 0) {
				G_pGame->m_DDraw.m_pGPURenderer->SetNativeResolution(w, h);
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	default:
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}	
	return NULL;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpCmdLine, int nCmdShow )
{HINSTANCE hDll;
 char cSearchDll[] = "rd`qbg-ckk";
 char cRealName[12];

	srand((unsigned)time(NULL));
	char *pJammer = new char[(rand() % 100) +1];
	G_pGame = new class CGame;
	ZeroMemory(cRealName, sizeof(cRealName));
	strcpy(cRealName, cSearchDll);
	for (WORD i = 0; i < strlen(cRealName); i++)
	if (cRealName[i] != NULL) cRealName[i]++;

	hDll = LoadLibrary(cRealName);
	if( hDll == NULL ) 
	{	MessageBox(NULL, "don't find search.dll", "ERROR!", MB_OK);
		return 0;
	}

#ifdef USING_WIN_IME
	HINSTANCE hRichDll = LoadLibrary( "Riched20.dll" );
#endif

	typedef int (MYPROC)(char *) ;
	MYPROC *pFindHook; 
	pFindHook = (MYPROC *) GetProcAddress(hDll, "__FindHackingDll__") ;

	if (pFindHook== NULL) 
	{	MessageBox(NULL, "can't find search.dll", "ERROR!", MB_OK);
		return 0 ;
	}else if ((*pFindHook)("CRCCHECK") != 1) 
	{	return 0 ;
	}
	FreeLibrary(hDll);

#ifndef _DEBUG
	if (OpenMutex(MUTEX_ALL_ACCESS, FALSE, "0543kjg3j31%") != NULL) {
		MessageBox(NULL, "Only one Helbreath client program allowed!", "ERROR!", MB_OK);
		return 0;
	}
	HANDLE hMutex = CreateMutex(NULL, FALSE, "0543kjg3j31%");
#endif

	// Note: GLFW window creation removed - OpenGL context now created on Win32 window via WGL
	// GPU renderer initialization happens in DXC_ddraw::bInit() using WGL

	sprintf( szAppClass, "Client-I%d", hInstance);
	if (!InitApplication( hInstance))		return (FALSE);
    if (!InitInstance(hInstance, nCmdShow)) return (FALSE);

	Initialize((char *)lpCmdLine);

	EventLoop();


#ifndef _DEBUG
	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
#endif

	delete[] pJammer;
	delete G_pGame;

#ifdef USING_WIN_IME
	FreeLibrary(hRichDll);
#endif

	return 0;
}


GLuint loadTexture(const char* filepath) {
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		std::cerr << "Failed to load texture" << std::endl;
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	return textureID;
}



BOOL InitApplication( HINSTANCE hInstance)
{WNDCLASS  wc;
	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadCursor(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szAppClass;        
	return (RegisterClass(&wc));
}

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{	int cx = GetSystemMetrics(SM_CXFULLSCREEN)/2;
	int cy = GetSystemMetrics(SM_CYFULLSCREEN)/2;
	// Create window at 2x scale (1280x960) for GPU rendering
	G_hWnd = CreateWindowEx(NULL, szAppClass, "Helbreath Legion", WS_POPUP, cx-640, cy-480,
							1280, 960, NULL, NULL, hInstance, NULL);
    if (!G_hWnd) return FALSE;
    G_hInstance	= hInstance;
	ShowWindow(G_hWnd, SW_SHOWDEFAULT);
	UpdateWindow(G_hWnd);
	return TRUE;
}

// Shader source code
const GLchar* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 position;
    void main() {
        gl_Position = vec4(position, 1.0);
    }
)glsl";

const GLchar* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 color;
    void main() {
        color = vec4(1.0, 0.5, 0.2, 1.0);
    }
)glsl";





GLuint LoadTexture(const char* filepath) {
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // Tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (data) {
		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
		return textureID;
	}
	else {
		MessageBox(NULL, "Failed to load texture", "ERROR!", MB_OK);
		return 0;
	}
}

void DrawSprite(GLuint textureID, float x, float y, float width, float height) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
	glEnd();
}

GLuint CompileShaders() {
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Check for shader compile errors...

	// Fragment shader for advanced effects
	const GLchar* fragmentShaderSource = R"glsl(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 ourColor; // We set this variable from the OpenGL code.
        void main() {
            FragColor = ourColor;
        }
    )glsl";

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Check for shader compile errors...

	// Link shaders
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Check for linking errors...

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}


void SetupOpenGLFor2D() {
	glDisable(GL_DEPTH_TEST); // Disable depth testing for 2D
	glEnable(GL_BLEND); // Enable blending, useful for textures with transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set blending function
}

void RenderWithOpenGL() {
	// Ensure the shader program and texture are loaded once, preferably in the initialization part of your application
	static GLuint shaderProgram; 
	static GLuint textureID = LoadTexture("textures/login_background.png"); // Same as above

	// Activate the shader program
	glUseProgram(shaderProgram);

	// Bind the texture for use in rendering
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set up vertex data (and buffer(s)) and configure vertex attributes
	// Note: This is a simplified example. For a complete implementation, you should define VBOs (Vertex Buffer Objects) and VAOs (Vertex Array Objects)
	GLfloat vertices[] = {
		// Positions         // Texture Coords
		-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom Left
		 0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom Right
		 0.0f,  0.5f, 0.0f,  0.5f, 1.0f  // Top 
	};

	// You would typically upload these to the GPU and use them in your draw call
	// For now, this is more conceptual as implementing VBOs/VAOs is beyond this snippet's scope

	// Draw the triangle
	// Note: Adjust your drawing code to use VBOs and attribute pointers
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < 9; i += 3) {
		glVertex3f(vertices[i], vertices[i + 1], vertices[i + 2]);
	}
	glEnd();

	// Note: GLFW no longer used - this function is deprecated
	// Buffer swapping now handled by GPURenderer::SwapBuffers() via WGL
}



void SetupOrtho(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, 0, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void EventLoop() {
    MSG msg;
    // Standard Win32 message loop - no GLFW dependency
    // GPU rendering uses WGL on the Win32 window directly
    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (!GetMessage(&msg, NULL, 0, 0)) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Game logic and rendering update
            G_pGame->UpdateScreen();

            // GPU rendering is handled through DXC_ddraw and CSprite classes
            // Buffer swapping happens in DXC_ddraw::EndGPUFrame()
        }
    }
}


void OnDestroy()
{	G_pGame->m_bIsProgramActive = FALSE;		
	_StopTimer(G_mmTimer);
	G_pGame->Quit();
	WSACleanup();
	PostQuitMessage(0);
}

void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{	PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, NULL);
}


MMRESULT _StartTimer(DWORD dwTime)
{TIMECAPS caps;
	timeGetDevCaps(&caps, sizeof(caps));
	timeBeginPeriod(caps.wPeriodMin);
	return timeSetEvent(dwTime,0,_TimerFunc,0, (UINT)TIME_PERIODIC);
}


void _StopTimer(MMRESULT timerid)
{TIMECAPS caps;
	if (timerid != 0) 
	{	timeKillEvent(timerid);
		timerid = 0;
		timeGetDevCaps(&caps, sizeof(caps));
		timeEndPeriod(caps.wPeriodMin);
	}
}



void Initialize(char * pCmdLine)
{int iX, iY, iSum;
 int     iErrCode;
 WORD	 wVersionRequested;
 WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 ); 
	iErrCode = WSAStartup( wVersionRequested, &wsaData );
	if ( iErrCode ) 
	{	MessageBox(G_hWnd, "Winsock-V1.1 not found! Cannot execute program.","ERROR",MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}
	if (G_pGame->bInit(G_hWnd, G_hInstance, pCmdLine) == FALSE) 
	{	PostQuitMessage(0);
		return;
	}	
	G_mmTimer = _StartTimer(1000);
	for (iX = 0; iX < 64; iX++)
	for (iY = 0; iY < 510; iY++) 
	{	iSum = iX + (iY - 255);
		if (iSum <= 0)  iSum = 1;
		if (iSum >= 31) iSum = 31;
		G_iAddTable31[iX][iY] = iSum; 
		iSum = iX + (iY - 255);
		if (iSum <= 0)  iSum = 1;
		if (iSum >= 63) iSum = 63;
		G_iAddTable63[iX][iY] = iSum; 
		if ((iY - 255) < iX) G_iAddTransTable31[iY][iX] = iX;
		else if ((iY - 255) > 31) G_iAddTransTable31[iY][iX] = 31;
		else G_iAddTransTable31[iY][iX] = iY-255;
		if ((iY - 255) < iX) G_iAddTransTable63[iY][iX] = iX;
		else if ((iY - 255) > 63) G_iAddTransTable63[iY][iX] = 63;
		else G_iAddTransTable63[iY][iX] = iY-255;
	}
}

LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{   HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);
    if (retval == ERROR_SUCCESS) 
	{	long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, NULL, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }
    return retval;
}


void GoHomepage() 
{	
	LPCTSTR	url = MSG_HOMEPAGE;

	int		showcmd = SW_SHOW;
	char	key[MAX_PATH + MAX_PATH];	
    // First try ShellExecute()
    HINSTANCE result = ShellExecute(NULL, "open", url, NULL,NULL, showcmd);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) 
	{  if (GetRegKey(HKEY_CLASSES_ROOT, ".htm", key) == ERROR_SUCCESS) 
		{  lstrcat(key, "\\shell\\open\\command");
            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS)
			{   char *pos;
                pos = strstr(key, "\"%1\"");
                if (pos == NULL) {                     // No quotes found
                    pos = strstr(key, "%1");           // Check for %1, without quotes 
                    if (pos == NULL)                   // No parameter at all...
                          pos = key+lstrlen(key)-1;
                    else *pos = '\0';                   // Remove the parameter
                }else    *pos = '\0';                   // Remove the parameter
                lstrcat(pos, " ");
                lstrcat(pos, url);
                result = (HINSTANCE) WinExec(key,showcmd);
    }	}	}
}
////////////////////////////////////////////////////////////////////////
