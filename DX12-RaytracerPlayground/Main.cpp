
#include <Windows.h>
#include <crtdbg.h>

#include "Window.h"

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
}