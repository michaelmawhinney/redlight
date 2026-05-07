#include <windows.h>
#include <shellapi.h>
#include <cstdio>

#include "GammaRampFilter.h"
#include "MagnificationFilter.h"

#define WM_TRAYICON (WM_USER + 1)
#define IDI_REDLIGHT_ICON 100
#define ID_TRAY_TOGGLE 200
#define ID_TRAY_ABOUT 201
#define ID_TRAY_EXIT 202

namespace {

MagnificationFilter g_magnificationFilter;
GammaRampFilter g_gammaRampFilter;
DisplayFilter* g_displayFilter = &g_gammaRampFilter;
NOTIFYICONDATA nid = {};

void LogBackendMessage(const char* message, const char* backendName) {
    char buffer[256];
    sprintf_s(buffer, sizeof(buffer), "RedLight: %s (%s)\n", message, backendName);
    OutputDebugStringA(buffer);
}

}

void ToggleRedlight();
void UpdateTrayIconTip(const char* tip);
void InitializeTrayIcon(HINSTANCE hInstance);
void ShowAboutDialog(HWND parent);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    // silently exit multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\RedLightApp");
    if (!hMutex) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 1;
    }

    if (g_magnificationFilter.initialize()) {
        g_displayFilter = &g_magnificationFilter;
        LogBackendMessage("using primary display backend", g_displayFilter->name());
    } else {
        OutputDebugStringA("RedLight: MagnificationFilter initialization failed; falling back to GammaRampFilter.\n");
        if (!g_gammaRampFilter.initialize()) {
            CloseHandle(hMutex);
            return 1;
        }
        g_displayFilter = &g_gammaRampFilter;
        LogBackendMessage("using fallback display backend", g_displayFilter->name());
    }

    InitializeTrayIcon(hInstance);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_displayFilter->shutdown();

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}

void UpdateTrayIconTip(const char* tip) {
    strcpy_s(nid.szTip, sizeof(nid.szTip), tip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void ToggleRedlight() {
    if (g_displayFilter->isActive()) {
        if (!g_displayFilter->disable()) {
            OutputDebugStringA("RedLight: failed to disable red filter.\n");
        }
    } else {
        if (!g_displayFilter->enable()) {
            OutputDebugStringA("RedLight: failed to enable red filter.\n");
        }
    }

    UpdateTrayIconTip(g_displayFilter->isActive() ? "RedLight ON" : "RedLight off");
}

void InitializeTrayIcon(HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TrayOnlyClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "TrayOnlyClass", "RedLight", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = IDI_REDLIGHT_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REDLIGHT_ICON));
    strcpy_s(nid.szTip, g_displayFilter->isActive() ? "RedLight ON" : "RedLight off");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowAboutDialog(HWND parent) {
    char aboutText[512];
    sprintf_s(aboutText, sizeof(aboutText), "RedLight v0.4.0-beta\n\ngithub.com/michaelmawhinney/redlight");
    const char* aboutTitle = "About";

    MessageBox(parent, aboutText, aboutTitle, MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                AppendMenu(hMenu, MF_STRING, ID_TRAY_TOGGLE, TEXT("Toggle ON/off"));
                AppendMenu(hMenu, MF_STRING, ID_TRAY_ABOUT, TEXT("About"));
                AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
        } else if (lParam == WM_LBUTTONDOWN) {
            ToggleRedlight();
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_TOGGLE) {
            ToggleRedlight();
        } else if (LOWORD(wParam) == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == ID_TRAY_ABOUT) {
            ShowAboutDialog(hwnd);
        }
        break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
