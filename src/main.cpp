#include <windows.h>
#include <shellapi.h>
#include <cstdio>

#define WM_TRAYICON (WM_USER + 1)
#define IDI_REDLIGHT_ICON 100
#define ID_TRAY_TOGGLE 200
#define ID_TRAY_ABOUT 201
#define ID_TRAY_EXIT 202

HDC hDC;
WORD originalGammaRamp[3][256];
WORD redGammaRamp[3][256];
bool isRedlightActive = false;
NOTIFYICONDATA nid = {};

void ToggleRedlight();
void UpdateTrayIconTip(const char* tip);
void InitializeTrayIcon(HINSTANCE hInstance);
void ShowAboutDialog(HWND parent);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    // silently exit multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\RedLightApp");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 1;
    }

    hDC = GetDC(NULL);
    if (!hDC) {
        MessageBox(NULL, "Failed to get device context.", "Error", MB_ICONERROR);
        return 1;
    }

    GetDeviceGammaRamp(hDC, originalGammaRamp); // store the original gamma ramp

    for (int i = 0; i < 256; i++) {
        redGammaRamp[0][i] = (WORD)(i << 8);
        redGammaRamp[1][i] = redGammaRamp[2][i] = 0;
    }

    InitializeTrayIcon(hInstance);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    SetDeviceGammaRamp(hDC, originalGammaRamp); // restore the original gamma ramp
    ReleaseDC(NULL, hDC);

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
        SetDeviceGammaRamp(hDC, originalGammaRamp);
    } else {
        SetDeviceGammaRamp(hDC, redGammaRamp);
    }
    isRedlightActive = !isRedlightActive;
    UpdateTrayIconTip(isRedlightActive ? "RedLight ON" : "RedLight off");
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
    strcpy_s(nid.szTip, "RedLight off");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowAboutDialog(HWND parent) {
    const HINSTANCE hInstance = GetModuleHandle(NULL);
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
