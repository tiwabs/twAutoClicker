#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../nlohmann/json.hpp"

#include <string>
#include <fstream>

using json = nlohmann::json;

extern ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);

long __stdcall WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
		case WM_SIZE: {
			if (gui::device && wideParameter != SIZE_MINIMIZED)
			{
				gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
				gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
				gui::ResetDevice();
			}
		} return 0;

		case WM_SYSCOMMAND: {
			if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
		} break;

		case WM_DESTROY: {
			PostQuitMessage(0);
		} return 0;

		case WM_LBUTTONDOWN: {
			gui::position = MAKEPOINTS(longParameter);
		} return 0;

		case WM_MOUSEMOVE: {
			if (wideParameter == MK_LBUTTON)
			{
				const auto points = MAKEPOINTS(longParameter);
				auto rect = ::RECT{ };

				GetWindowRect(gui::window, &rect);

				rect.left += points.x - gui::position.x;
				rect.top += points.y - gui::position.y;

				if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 && gui::position.y <= 19)
					SetWindowPos(gui::window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
			}
		} return 0;
	}

	return DefWindowProcA(window, message, wideParameter, longParameter);
}

// handle window creation & destruction
void gui::CreateHWindow(const char* windowName, const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);
	window = CreateWindowA(className, windowName, WS_POPUP, 100, 100, WIDTH, HEIGHT, 0, 0, windowClass.hInstance, 0);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

// handle device creation & destruction
bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D9b_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

// handle ImGui creation & destruction
void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	// make imgui not creating a ini file when app start
	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// Render
void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 Device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

static void handleShortCut()
{
	if (!gui::waitingForKey) {
		if (GetAsyncKeyState(gui::startStopKey) & 0x8000)
		{
			if (!gui::keyWasDown)
			{
				if (gui::isClicking)
				{
					autoClicker::Stop();
					gui::isClicking = false;
				}
				else
				{
					autoClicker::Start(gui::interval);
					gui::isClicking = true;
				}
				gui::keyWasDown = true;
			}
		}
		else
		{
			gui::keyWasDown = false;
		}
	}
}

static void hanleEditKeyBind()
{
	for (int key = 0; key < 256; ++key)
	{
		if (GetAsyncKeyState(key) & 0x8000)
		{
			while (GetAsyncKeyState(key) & 0x8000) {
				Sleep(10);
			}
			gui::startStopKey = key;
			gui::waitingForKey = false;
			break;
		}
	}
}

void gui::Render() noexcept
{
	handleShortCut();

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin("Tiwabs Auto Clicker", &exit, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
	
	ImGui::PushItemWidth(100);
	ImGui::InputInt("Hours", &hours, 1, 100, ImGuiInputTextFlags_NoStepButtons);
	ImGui::PopItemWidth();
	
	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	ImGui::InputInt("Minutes", &minutes, 1, 100, ImGuiInputTextFlags_NoStepButtons);
	ImGui::PopItemWidth();
	
	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	ImGui::InputInt("Seconds", &seconds, 1, 100, ImGuiInputTextFlags_NoStepButtons);
	ImGui::PopItemWidth();
	
	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	ImGui::InputInt("Milliseconds", &milliseconds, 1, 100, ImGuiInputTextFlags_NoStepButtons);
	ImGui::PopItemWidth();

	interval = (hours * 3600000) + (minutes * 60000) + (seconds * 1000) + milliseconds;
	if (interval < 1) interval = 1;

	//ImGui::Text(("Interval: " + std::to_string(interval)).c_str());
	
	ImGui::PushItemWidth(100);
	ImGui::Combo("Click Type", &clickType, clickTypes, IM_ARRAYSIZE(clickTypes));
	
	ImGui::SameLine();
	
	ImGui::Checkbox("Double Click", &doubleClick);
	
	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	ImGui::InputInt("Repeat times", &repeatFor, 1, 0, ImGuiInputTextFlags_NoStepButtons);

	char text[64];
	ImGuiKey imKey = ImGui_ImplWin32_KeyEventToImGuiKey(startStopKey, 0);
	std::snprintf(text, sizeof(text), "Press [%s] %s", waitingForKey ? "Waiting for key" : ImGui::GetKeyName(imKey), isClicking ? "to stop" : "to start");

	if (ImGui::Button(text))
	{
		if (isClicking)
		{
			autoClicker::Stop();
			isClicking = false;
		}
		else
		{
			autoClicker::Start(interval);
			isClicking = true;
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Edit keybind"))
	{
		if (waitingForKey) return;
		waitingForKey = true;
	}

	if (waitingForKey)
		hanleEditKeyBind();


	ImGui::SameLine();

	if (ImGui::Button("Load Settings"))
	{
		OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = window;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn))
        {
            gui::LoadSettings(ofn.lpstrFile);
        }
	}

	ImGui::SameLine();

	if (ImGui::Button("Save Settings"))
	{
		OPENFILENAMEA ofn;
        char szFile[260] = "settings.json";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = window;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "json";

        if (GetSaveFileNameA(&ofn))
        {
            gui::SaveSettings(ofn.lpstrFile);
        }
	}

	ImGui::End();
}

void gui::SaveSettings(const std::string& filename)
{
	json settings;

	settings["hours"] = gui::hours;
	settings["minutes"] = gui::minutes;
	settings["seconds"] = gui::seconds;
	settings["milliseconds"] = gui::milliseconds;
	settings["interval"] = gui::interval;
	settings["startStopKey"] = gui::startStopKey;
	settings["clickType"] = gui::clickType;
	settings["doubleClick"] = gui::doubleClick;
	settings["repeatFor"] = gui::repeatFor;

	std::ofstream file(filename);
	if (file.is_open())
	{
		file << settings.dump(4);
		file.close();
	}
}

void gui::LoadSettings(const std::string& filename)
{
	std::ifstream file(filename);
	if (file.is_open())
	{
		json settings;
		file >> settings;
		file.close();

		if (settings.contains("hours")) hours = settings["hours"];
		if (settings.contains("minutes")) minutes = settings["minutes"];
		if (settings.contains("seconds")) seconds = settings["seconds"];
		if (settings.contains("milliseconds")) milliseconds = settings["milliseconds"];
		if (settings.contains("interval")) interval = settings["interval"];
		if (settings.contains("startStopKey")) startStopKey = settings["startStopKey"];
		if (settings.contains("clickType")) clickType = settings["clickType"];
		if (settings.contains("doubleClick")) doubleClick = settings["doubleClick"];
		if (settings.contains("repeatFor")) repeatFor = settings["repeatFor"];
	}
}

