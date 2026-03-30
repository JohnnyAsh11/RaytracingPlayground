#ifndef __WINDOW_H_
#define __WINDOW_H_

#include <Windows.h>
#include <string>

namespace Window
{
	/// <summary>
	/// Creates a graphical window context for rendering.
	/// </summary>
	HRESULT CreateGraphicsWindow(
		HINSTANCE a_AppInstance,
		unsigned int a_uWidth,
		unsigned int a_uHeight,
		std::wstring a_sTitle,
		void (*a_pResizeCallback)());

	/// <summary>
	/// Updates helper stats of the application.
	/// </summary>
	void Update(float a_fTotalTime);

	/// <summary>
	/// Cleans up the window and its associated resources.
	/// </summary>
	void Shutdown();

	/// <summary>
	/// Creates a Console window for debug output.
	/// </summary>
	void CreateConsoleWindow(int a_nBufferLines, int a_nBufferColumns, int a_nWindowLines, int a_nWindowColumns);

	/// <summary>
	/// OS message bus handling.
	/// </summary>
	LRESULT ProcessMessage(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam);

	// Accessors:
	unsigned int GetWidth();
	unsigned int GetHeight();
	float GetAspectRatio();
	HWND GetHandle();
	bool HasFocus();
	bool IsMinimized();
}

#endif //__WINDOW_H_

