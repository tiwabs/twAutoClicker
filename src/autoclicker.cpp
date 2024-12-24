#include "autoclicker.h"
#include "gui.h"

#include <thread>
#include <atomic>
#include <windows.h>

static std::atomic<bool> running(false);

static bool IsAppActive(HWND appWindow) {
	HWND foregroundWindow = GetForegroundWindow();
	return foregroundWindow == appWindow;
}

static void SendMouseClick(int clickType, bool isDoubleClick) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;

    DWORD downFlag = 0, upFlag = 0;
    switch (clickType) {
    case 0:
        downFlag = MOUSEEVENTF_LEFTDOWN;
        upFlag = MOUSEEVENTF_LEFTUP;
        break;
    case 1:
        downFlag = MOUSEEVENTF_RIGHTDOWN;
        upFlag = MOUSEEVENTF_RIGHTUP;
        break;
    case 2:
        downFlag = MOUSEEVENTF_MIDDLEDOWN;
        upFlag = MOUSEEVENTF_MIDDLEUP;
        break;
    }

    input.mi.dwFlags = downFlag;
    SendInput(1, &input, sizeof(INPUT));
    input.mi.dwFlags = upFlag;
    SendInput(1, &input, sizeof(INPUT));

    if (isDoubleClick) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        input.mi.dwFlags = downFlag;
        SendInput(1, &input, sizeof(INPUT));
        input.mi.dwFlags = upFlag;
        SendInput(1, &input, sizeof(INPUT));
    }
}

void autoClicker::Start(int interval) {
	if (running) return;
	
	running = true;

	std::thread([interval]() {
		while (running && !IsAppActive(gui::window))
		{

            if (gui::repeatFor > 0 && gui::clickCount == gui::repeatFor)
            {
                Stop();
            }

            SendMouseClick(gui::clickType, gui::doubleClick);

			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            
            gui::clickCount += 1;
		}
	}).detach();
}

void autoClicker::Stop() {
	running = false;
    gui::isClicking = false;
}