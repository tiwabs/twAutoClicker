#pragma once
#include "autoclicker.h"

#include <string>

#include <d3d9.h>

namespace gui
{
	// constant window size
	constexpr int WIDTH = 500;
	constexpr int HEIGHT = 100;

	inline bool exit = true;

	// autoclick vars
	inline const char* clickTypes[]{ "Left", "Right", "Middle" };
	inline int clickType = 0;
	inline int startStopKey = VK_F6;
	inline bool doubleClick = false;
	inline bool isClicking = false;
	inline bool keyWasDown = false;
	inline bool waitingForKey = false;
	inline int repeatFor = 0;
	inline int clickCount = 0;
	inline int hours = 0;
	inline int minutes = 0;
	inline int seconds = 0;
	inline int milliseconds = 1;
	inline int interval = 0;

	// winapi window vars
	inline HWND window = nullptr;
	inline WNDCLASSEXA windowClass = { };

	// points for window movement
	inline POINTS position = { };

	// directX state vars
	inline PDIRECT3D9 d3d = nullptr;
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	// handle window creation & destruction
	void CreateHWindow(const char* windowName, const char* className) noexcept;
	void DestroyHWindow() noexcept;

	// handle device creation & destruction
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	// handle ImGui creation & destruction
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	// Render
	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;

	// Settings
	void SaveSettings(const std::string& filename);
	void LoadSettings(const std::string& filename);
}
