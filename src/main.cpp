#include <windows.h>
#include <shellapi.h>
#include <magnification.h>
#include <cstdio>

#define WM_TRAYICON (WM_USER + 1)
#define IDI_REDLIGHT_ICON 100
#define ID_TRAY_TOGGLE 200
#define ID_TRAY_ABOUT 201
#define ID_TRAY_EXIT 202
#define ID_REFRESH_TIMER 1

bool isRedlightActive = false;
NOTIFYICONDATA nid = {};
HWND hwndTray = NULL;
HWND hwndHost = NULL;
HWND hwndMag = NULL;
HINSTANCE g_hInstance = NULL;

MAGCOLOREFFECT redEffect = {{
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f
}};

void ToggleRedlight();
void UpdateTrayIconTip(const char* tip);
void InitializeTrayIcon(HINSTANCE hInstance);
void ShowAboutDialog(HWND parent);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateMagnifierSource() {
    if (!hwndMag) return;
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    RECT src = { x, y, x + w, y + h };
    MagSetWindowSource(hwndMag, src);
}

bool CreateMagnifierOverlay() {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    hwndHost = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        "RedLightHost", NULL,
        WS_POPUP,
        x, y, w, h,
        NULL, NULL, g_hInstance, NULL
    );
    if (!hwndHost) return false;

    SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);

    hwndMag = CreateWindow(
        WC_MAGNIFIER, NULL,
        WS_CHILD | MS_SHOWMAGNIFIEDCURSOR | WS_VISIBLE,
        0, 0, w, h,
        hwndHost, NULL, g_hInstance, NULL
    );
    if (!hwndMag) {
        DestroyWindow(hwndHost);
        hwndHost = NULL;
        return false;
    }

    MAGTRANSFORM matrix = {};
    matrix.v[0][0] = 1.0f;
    matrix.v[1][1] = 1.0f;
    matrix.v[2][2] = 1.0f;
    MagSetWindowTransform(hwndMag, &matrix);
    MagSetColorEffect(hwndMag, &redEffect);
    UpdateMagnifierSource();
    ShowWindow(hwndHost, SW_SHOWNOACTIVATE);
    return true;
}

void DestroyMagnifierOverlay() {
    if (hwndHost) {
        DestroyWindow(hwndHost);
        hwndHost = NULL;
        hwndMag = NULL;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    // silently exit multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\RedLightApp");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 1;
    }

    g_hInstance = hInstance;

    if (!MagInitialize()) {
        MessageBox(NULL, "Failed to initialize Magnification API.", "RedLight Error", MB_ICONERROR);
        return 1;
    }

    InitializeTrayIcon(hInstance);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (isRedlightActive) DestroyMagnifierOverlay();
    MagUninitialize();

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}

void UpdateTrayIconTip(const char* tip) {
    strcpy_s(nid.szTip, sizeof(nid.szTip), tip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void ToggleRedlight() {
    if (isRedlightActive) {
        KillTimer(hwndTray, ID_REFRESH_TIMER);
        DestroyMagnifierOverlay();
    } else {
        if (!CreateMagnifierOverlay()) {
            MessageBox(NULL, "Failed to create color overlay.", "RedLight Error", MB_ICONERROR);
            return;
        }
        SetTimer(hwndTray, ID_REFRESH_TIMER, 16, NULL);
    }
    isRedlightActive = !isRedlightActive;
    UpdateTrayIconTip(isRedlightActive ? "RedLight ON" : "RedLight off");
}

void InitializeTrayIcon(HINSTANCE hInstance) {
    // Register host window class for magnifier overlay
    WNDCLASS hostClass = {};
    hostClass.lpfnWndProc = DefWindowProc;
    hostClass.hInstance = hInstance;
    hostClass.lpszClassName = "RedLightHost";
    RegisterClass(&hostClass);

    // Register tray window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TrayOnlyClass";
    RegisterClass(&wc);

    hwndTray = CreateWindowEx(0, "TrayOnlyClass", "RedLight", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwndTray;
    nid.uID = IDI_REDLIGHT_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REDLIGHT_ICON));
    strcpy_s(nid.szTip, "RedLight off");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowAboutDialog(HWND parent) {
    char aboutText[512];
    sprintf_s(aboutText, sizeof(aboutText), "RedLight v0.4.0-beta\n\ngithub.com/michaelmawhinney/redlight");
    MessageBox(parent, aboutText, "About", MB_OK | MB_ICONINFORMATION);
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

    case WM_TIMER:
        if (wParam == ID_REFRESH_TIMER) {
            UpdateMagnifierSource();
        }
        break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
