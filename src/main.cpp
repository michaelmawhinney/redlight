#include <windows.h>
#include <shellapi.h>
#include <cstdio>

#include "Diagnostics.h"
#include "GammaRampFilter.h"
#include "MagnificationFilter.h"

#define WM_TRAYICON (WM_USER + 1)
#define WM_REDLIGHT_RESTORE (WM_APP + 1)
#define IDI_REDLIGHT_ICON 100
#define ID_TRAY_TOGGLE 200
#define ID_TRAY_ABOUT 201
#define ID_TRAY_EXIT 202

namespace {

MagnificationFilter g_magnificationFilter;
GammaRampFilter g_gammaRampFilter;
DisplayFilter* g_displayFilter = &g_gammaRampFilter;
NOTIFYICONDATA nid = {};

}

bool ShouldRunResetMode();
void LogSystemInfo();
bool ResetDisplay();
bool RequestRunningInstanceRestore(bool* handled);
void ToggleRedlight();
void UpdateTrayIconTip(const char* tip);
void InitializeTrayIcon(HINSTANCE hInstance);
void ShowAboutDialog(HWND parent);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    Diagnostics::Initialize();
    Diagnostics::Log("RedLight starting.");
    LogSystemInfo();

    if (ShouldRunResetMode()) {
        const bool resetSucceeded = ResetDisplay();
        Diagnostics::LogFormat("RedLight reset mode completed with %s.", resetSucceeded ? "success" : "failure");
        Diagnostics::Log("RedLight exiting.");
        Diagnostics::Shutdown();
        return resetSucceeded ? 0 : 1;
    }

    // silently exit multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\RedLightApp");
    if (!hMutex) {
        Diagnostics::LogFormat("CreateMutex failed (GetLastError=%lu).", static_cast<unsigned long>(GetLastError()));
        Diagnostics::Log("RedLight exiting.");
        Diagnostics::Shutdown();
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Diagnostics::Log("Another RedLight instance is already running.");
        CloseHandle(hMutex);
        Diagnostics::Log("RedLight exiting.");
        Diagnostics::Shutdown();
        return 1;
    }

    if (g_magnificationFilter.initialize()) {
        g_displayFilter = &g_magnificationFilter;
        Diagnostics::LogFormat("Selected backend: %s", g_displayFilter->name());
    } else {
        Diagnostics::Log("MagnificationFilter initialization failed; falling back to GammaRampFilter.");
        if (!g_gammaRampFilter.initialize()) {
            Diagnostics::Log("GammaRampFilter initialization failed.");
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            Diagnostics::Log("RedLight exiting.");
            Diagnostics::Shutdown();
            return 1;
        }
        g_displayFilter = &g_gammaRampFilter;
        Diagnostics::LogFormat("Selected backend: %s", g_displayFilter->name());
    }

    Diagnostics::LogFormat("Monitor count: %d", GetSystemMetrics(SM_CMONITORS));
    InitializeTrayIcon(hInstance);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_displayFilter->shutdown();
    Diagnostics::Log("RedLight exiting.");

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    Diagnostics::Shutdown();
    return 0;
}

bool ShouldRunResetMode() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return false;
    }

    bool resetMode = false;
    for (int i = 1; i < argc; ++i) {
        if (lstrcmpiW(argv[i], L"--restore") == 0 || lstrcmpiW(argv[i], L"--reset-display") == 0) {
            resetMode = true;
            break;
        }
    }

    LocalFree(argv);
    return resetMode;
}

void LogSystemInfo() {
    HMODULE ntdll = LoadLibraryA("ntdll.dll");
    if (!ntdll) {
        Diagnostics::Log("Windows version: unavailable.");
        return;
    }

    struct RTL_OSVERSIONINFOW_LOCAL {
        ULONG dwOSVersionInfoSize;
        ULONG dwMajorVersion;
        ULONG dwMinorVersion;
        ULONG dwBuildNumber;
        ULONG dwPlatformId;
        WCHAR szCSDVersion[128];
    } version = {};

    using RtlGetVersionFn = LONG (WINAPI*)(RTL_OSVERSIONINFOW_LOCAL*);
    auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtlGetVersion) {
        FreeLibrary(ntdll);
        Diagnostics::Log("Windows version: unavailable.");
        return;
    }

    version.dwOSVersionInfoSize = sizeof(version);
    if (rtlGetVersion(&version) == 0) {
        Diagnostics::LogFormat("Windows version: %lu.%lu build %lu",
            static_cast<unsigned long>(version.dwMajorVersion),
            static_cast<unsigned long>(version.dwMinorVersion),
            static_cast<unsigned long>(version.dwBuildNumber));
    } else {
        Diagnostics::Log("Windows version: unavailable.");
    }

    FreeLibrary(ntdll);
}

bool ResetDisplay() {
    bool restoreHandledByRunningInstance = false;
    if (RequestRunningInstanceRestore(&restoreHandledByRunningInstance)) {
        Diagnostics::Log("Reset mode: running instance restore succeeded.");
        return true;
    }

    if (restoreHandledByRunningInstance) {
        Diagnostics::Log("Reset mode: running instance restore failed.");
        return false;
    }

    constexpr MAGCOLOREFFECT kIdentityEffect = {{
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    }};

    Diagnostics::Log("Reset mode: attempting Magnification API restore.");

    if (!MagInitialize()) {
        Diagnostics::LogFormat("Reset mode: MagInitialize failed (GetLastError=%lu).", static_cast<unsigned long>(GetLastError()));
        Diagnostics::Log("Reset mode: gamma-ramp reset not available without a saved ramp.");
        return false;
    }

    Diagnostics::Log("Reset mode: MagInitialize succeeded.");

    MAGCOLOREFFECT identity = kIdentityEffect;
    const BOOL colorEffectReset = MagSetFullscreenColorEffect(&identity);
    if (colorEffectReset) {
        Diagnostics::Log("Reset mode: MagSetFullscreenColorEffect(identity) succeeded.");
    } else {
        Diagnostics::LogFormat("Reset mode: MagSetFullscreenColorEffect(identity) failed (GetLastError=%lu).",
            static_cast<unsigned long>(GetLastError()));
    }

    if (!MagUninitialize()) {
        Diagnostics::LogFormat("Reset mode: MagUninitialize failed (GetLastError=%lu).", static_cast<unsigned long>(GetLastError()));
    } else {
        Diagnostics::Log("Reset mode: MagUninitialize succeeded.");
    }

    Diagnostics::Log("Reset mode: gamma-ramp reset not available without a saved ramp.");
    return colorEffectReset != FALSE;
}

bool RequestRunningInstanceRestore(bool* handled) {
    if (handled) {
        *handled = false;
    }

    HWND trayWindow = FindWindowA("TrayOnlyClass", "RedLight");
    if (!trayWindow) {
        Diagnostics::Log("Reset mode: running tray window not found.");
        return false;
    }

    if (handled) {
        *handled = true;
    }

    Diagnostics::LogFormat("Reset mode: found running tray window (hwnd=%p).", static_cast<void*>(trayWindow));

    DWORD_PTR restoreResult = FALSE;
    const LRESULT messageResult = SendMessageTimeoutA(
        trayWindow,
        WM_REDLIGHT_RESTORE,
        0,
        0,
        SMTO_ABORTIFHUNG,
        3000,
        &restoreResult);

    if (messageResult == 0) {
        Diagnostics::LogFormat(
            "Reset mode: WM_REDLIGHT_RESTORE send failed or timed out (GetLastError=%lu).",
            static_cast<unsigned long>(GetLastError()));
        return false;
    }

    Diagnostics::LogFormat(
        "Reset mode: WM_REDLIGHT_RESTORE succeeded with result=%s.",
        restoreResult ? "TRUE" : "FALSE");
    return restoreResult != FALSE;
}

void UpdateTrayIconTip(const char* tip) {
    strcpy_s(nid.szTip, sizeof(nid.szTip), tip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void ToggleRedlight() {
    Diagnostics::LogFormat("Toggle request: %s", g_displayFilter->isActive() ? "disable" : "enable");
    if (g_displayFilter->isActive()) {
        if (!g_displayFilter->disable()) {
            Diagnostics::Log("Toggle result: disable failed.");
        } else {
            Diagnostics::Log("Toggle result: disable succeeded.");
        }
    } else {
        if (!g_displayFilter->enable()) {
            Diagnostics::Log("Toggle result: enable failed.");
        } else {
            Diagnostics::Log("Toggle result: enable succeeded.");
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
    if (!hwnd) {
        Diagnostics::LogFormat("CreateWindowEx failed (GetLastError=%lu).", static_cast<unsigned long>(GetLastError()));
    }

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = IDI_REDLIGHT_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REDLIGHT_ICON));
    strcpy_s(nid.szTip, g_displayFilter->isActive() ? "RedLight ON" : "RedLight off");
    if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
        Diagnostics::LogFormat("Shell_NotifyIcon(NIM_ADD) failed (GetLastError=%lu).", static_cast<unsigned long>(GetLastError()));
    }
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

    case WM_REDLIGHT_RESTORE:
        Diagnostics::Log("External restore request received.");
        if (!g_displayFilter->isActive()) {
            Diagnostics::Log("External restore request: filter already inactive.");
            return TRUE;
        }

        if (!g_displayFilter->disable()) {
            Diagnostics::Log("External restore request: disable failed.");
            return FALSE;
        }

        UpdateTrayIconTip("RedLight off");
        Diagnostics::Log("External restore request: disable succeeded.");
        return TRUE;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
