#include "Window.h"
#include "Graphics.h"
#include "Input.h"

#include <sstream>

namespace Window
{
	// Anonymous namespace to hold internal values.
	namespace 
	{
		// Init?
		bool bWindowCreated = false;
		bool bConsoleCreated = false;

		// Window details:
		std::wstring sWindowTitle;
		unsigned int uWindowWidth = 0;
		unsigned int uWindowHeight = 0;
		HWND windowHandle = 0;
		bool bHasFocus = false;
		bool bIsMinimized = false;
		void (*pOnResize)() = 0;

		// FPS tracking:
		float fFpsTimeElapsed = 0.0f;
		__int64 nFpsFrameCounter = 0;
	}
}

HRESULT Window::CreateGraphicsWindow(HINSTANCE a_AppInstance, unsigned int a_uWidth, unsigned int a_uHeight, std::wstring a_sTitle, void(*a_pResizeCallback)())
{
	if (bWindowCreated)
	{
		return E_FAIL;
	}

	// Saving the window details.
	uWindowWidth = a_uWidth;
	uWindowHeight = a_uHeight;
	sWindowTitle = a_sTitle;
	pOnResize = a_pResizeCallback;

	// Create the default window description struct.
	WNDCLASS windowDesc = {};
	windowDesc.style = CS_HREDRAW | CS_VREDRAW;
	windowDesc.lpfnWndProc = ProcessMessage;
	windowDesc.cbClsExtra = 0;
	windowDesc.cbWndExtra = 0;
	windowDesc.hInstance = a_AppInstance;
	windowDesc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowDesc.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowDesc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowDesc.lpszMenuName = NULL;
	windowDesc.lpszClassName = L"GraphicsWindowClass";

	// Attempting to regist the window description.
	if (!RegisterClass(&windowDesc))
	{
		// grab the most recent error.
		DWORD error = GetLastError();

		// If the class exists, that's actually fine.  Otherwise,
		// we can't proceed with the next step.
		if (error != ERROR_CLASS_ALREADY_EXISTS)
		{
			return HRESULT_FROM_WIN32(error);
		}
	}

	// Ensuring the client sizes match our desired window size.
	RECT clientRect;
	SetRect(&clientRect, 0, 0, uWindowWidth, uWindowHeight);
	AdjustWindowRect(
		&clientRect,
		WS_OVERLAPPEDWINDOW,
		false);

	// Center the window on the screen
	RECT desktopRect;
	GetClientRect(GetDesktopWindow(), &desktopRect);
	int centeredX = (desktopRect.right / 2) - (clientRect.right / 2);
	int centeredY = (desktopRect.bottom / 2) - (clientRect.bottom / 2);

	// Actually querying the OS to create the window.
	windowHandle = CreateWindow(
		windowDesc.lpszClassName,
		sWindowTitle.c_str(),
		WS_OVERLAPPEDWINDOW,
		centeredX,
		centeredY,
		clientRect.right - clientRect.left,	
		clientRect.bottom - clientRect.top,
		0,
		0,
		a_AppInstance,
		0);			

	// Ensuring the window was created properly.
	if (windowHandle == NULL)
	{
		DWORD error = GetLastError();
		return HRESULT_FROM_WIN32(error);
	}

	// Show the window.
	bWindowCreated = true;
	ShowWindow(windowHandle, SW_SHOW);
	return S_OK;
}

void Window::Update(float a_fTotalTime)
{
	// Track frame count
	nFpsFrameCounter++;
	float elapsed = a_fTotalTime - fFpsTimeElapsed;

	// Only update once per second, otherwise it will be too volatile to read.
	if (elapsed < 1.0f) return;

	// Approximately how long each frame took.  fpsFrameCounter == FPS.
	float mspf = 1000.0f / (float)nFpsFrameCounter;
	nFpsFrameCounter = 0;
	fFpsTimeElapsed += elapsed;
}

void Window::Shutdown()
{
	// Tell OS to close the app.
	PostMessage(windowHandle, WM_CLOSE, 0, 0);
}

void Window::CreateConsoleWindow(int a_nBufferLines, int a_nBufferColumns, int a_nWindowLines, int a_nWindowColumns)
{
	// Ensure there is not already a console.
	if (bConsoleCreated)
		return;

	// Similar to the graphics window creation struct.
	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	// Getting the console ingo and setting buffer sizes.
	AllocConsole();
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = a_nBufferLines;
	coninfo.dwSize.X = a_nBufferColumns;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	SMALL_RECT rect = {};
	rect.Left = 0;
	rect.Top = 0;
	rect.Right = a_nWindowColumns;
	rect.Bottom = a_nWindowLines;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

	FILE* stream;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);

	HWND consoleHandle = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu(consoleHandle, FALSE);
	EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);

	DWORD currentMode = 0;
	GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &currentMode);
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
		currentMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	bConsoleCreated = true;
}

LRESULT Window::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Checking the incoming message and handle accordingly.
	switch (uMsg)
	{
		// Window closing message.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// Stops beeping when using "alt-enter".
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

		// Minimum window size.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

		// Message for window resizing.
	case WM_SIZE:
		bIsMinimized = wParam == SIZE_MINIMIZED;
		if (bIsMinimized)
			return 0;

		// Saving the new dimension.
		uWindowWidth = LOWORD(lParam);
		uWindowHeight = HIWORD(lParam);

		// Let other systems know
		Graphics::ResizeBuffers(uWindowWidth, uWindowHeight);
		if (pOnResize)
			pOnResize();

		return 0;

		// Mouse wheel scrolling.
	case WM_MOUSEWHEEL:
		Input::SetWheelDelta(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
		return 0;

		// Focus state changes.
	case WM_SETFOCUS:	bHasFocus = true;	return 0;
	case WM_KILLFOCUS:	bHasFocus = false;	return 0;
	case WM_ACTIVATE:	bHasFocus = (LOWORD(wParam) != WA_INACTIVE); return 0;
	}

	// Resort any other messages to windows default message handler.
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

unsigned int Window::GetWidth() { return uWindowWidth; }
unsigned int Window::GetHeight() { return uWindowHeight; }
float Window::GetAspectRatio() { return (float)uWindowWidth / uWindowHeight; }
HWND Window::GetHandle() { return windowHandle; }
bool Window::HasFocus() { return bHasFocus; }
bool Window::IsMinimized() { return bIsMinimized; }
