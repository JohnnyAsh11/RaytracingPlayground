
#include <Windows.h>
#include <crtdbg.h>

#include "Window.h"
#include "Graphics.h"
#include "Input.h"
#include "Application.h"
namespace
{
	Application* pApp = 0;

	void ResizeWindow()
	{
		if (pApp)
		{
			pApp->OnResize();
		}
	}
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	// Enable memory leak detection:
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Window::CreateConsoleWindow(500, 120, 32, 120);
#endif

	// Set up app init details.
	unsigned int windowWidth = 1280;
	unsigned int windowHeight = 720;
	const wchar_t* windowTitle = L"Raytracer Playground";
	bool vsync = false;
	
	// Create the window and verify
	HRESULT windowResult = Window::CreateGraphicsWindow(
		hInstance,
		windowWidth,
		windowHeight,
		windowTitle,
		ResizeWindow);
	if (FAILED(windowResult))
		return windowResult;

	// Initialize the graphics API and verify
	HRESULT graphicsResult = Graphics::Initialize(
		Window::GetWidth(),
		Window::GetHeight(),
		Window::GetHandle(),
		vsync);
	if (FAILED(graphicsResult))
		return graphicsResult;

	// Initalize the input system, which requires the window handle
	Input::Initialize(Window::GetHandle());

	// Now the main application object itself can be initialzied
	pApp = new Application();

	// Time tracking
	LARGE_INTEGER perfFreq{};
	double perfSeconds = 0;
	__int64 startTime = 0;
	__int64 currentTime = 0;
	__int64 previousTime = 0;

	// Query for accurate timing information
	QueryPerformanceFrequency(&perfFreq);
	perfSeconds = 1.0 / (double)perfFreq.QuadPart;

	// Performance Counter gives high-resolution time stamps
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	currentTime = startTime;
	previousTime = startTime;

	// Windows message loop (and our game loop)
	MSG msg = {};	while (msg.message != WM_QUIT)
	{
		// Determine if there is a message from the operating system
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate and dispatch the message
			// to our custom WindowProc function
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Calculate up-to-date timing info
			QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
			float deltaTime = max((float)((currentTime - previousTime) * perfSeconds), 0.0f);
			float totalTime = (float)((currentTime - startTime) * perfSeconds);
			previousTime = currentTime;

			// Calculate basic fps
			Window::Update(totalTime);

			// Input updating
			Input::Update();

			// Update and draw
			pApp->Update(deltaTime, totalTime);
			pApp->Draw(deltaTime, totalTime);

			// Notify Input system about end of frame
			Input::EndOfFrame();

#if defined(DEBUG) || defined(_DEBUG)
			// Print any graphics debug messages that occurred this frame
			Graphics::PrintDebugMessages();
#endif
		}
	}

	// Clean up
	delete pApp;
	Input::ShutDown();
	Graphics::ShutDown();
	return (HRESULT)msg.wParam;
}