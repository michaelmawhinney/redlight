#include <windows.h>
#include <shellapi.h>
#include <cstdio>

#define WM_TRAYICON       (WM_USER + 1)
#define IDI_REDLIGHT_ICON 100
#define ID_TRAY_TOGGLE    200
#define ID_TRAY_ABOUT     201
#define ID_TRAY_EXIT      202
#define ID_TRAY_SCHEDULE  203

#define IDD_SCHEDULE      300
#define IDC_SCHED_ENABLE  301
#define IDC_START_HOUR    302
#define IDC_START_MIN     303
#define IDC_END_HOUR      304
#define IDC_END_MIN       305

#define ID_SCHEDULE_TIMER 2

HDC hDC;
WORD originalGammaRamp[3][256];
WORD redGammaRamp[3][256];
bool isRedlightActive = false;
NOTIFYICONDATA nid = {};
HWND hwndTray = NULL;

// Schedule state (loaded from / saved to registry)
bool schedEnabled  = false;
int  schedOnHour   = 20, schedOnMin  = 0;
int  schedOffHour  = 8,  schedOffMin = 0;

void ToggleRedlight();
void UpdateTrayIconTip(const char* tip);
void InitializeTrayIcon(HINSTANCE hInstance);
void ShowAboutDialog(HWND parent);
void ShowScheduleDialog(HWND parent);
void LoadSchedule();
void SaveSchedule();
void CheckSchedule();
bool IsInScheduleWindow();
LRESULT CALLBACK  WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK  ScheduleDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ── Registry helpers ──────────────────────────────────────────────────────────

void LoadSchedule() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\RedLight", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    DWORD val, size = sizeof(DWORD);
    auto qv = [&](const char* name, int& dst) {
        if (RegQueryValueExA(hKey, name, NULL, NULL, (BYTE*)&val, &size) == ERROR_SUCCESS)
            dst = (int)val;
    };
    auto qb = [&](const char* name, bool& dst) {
        if (RegQueryValueExA(hKey, name, NULL, NULL, (BYTE*)&val, &size) == ERROR_SUCCESS)
            dst = (val != 0);
    };

    qb("ScheduleEnabled", schedEnabled);
    qv("OnHour",  schedOnHour);
    qv("OnMin",   schedOnMin);
    qv("OffHour", schedOffHour);
    qv("OffMin",  schedOffMin);

    RegCloseKey(hKey);
}

void SaveSchedule() {
    HKEY hKey;
    RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\RedLight", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

    auto sv = [&](const char* name, DWORD val) {
        RegSetValueExA(hKey, name, 0, REG_DWORD, (BYTE*)&val, sizeof(val));
    };

    sv("ScheduleEnabled", schedEnabled ? 1 : 0);
    sv("OnHour",  (DWORD)schedOnHour);
    sv("OnMin",   (DWORD)schedOnMin);
    sv("OffHour", (DWORD)schedOffHour);
    sv("OffMin",  (DWORD)schedOffMin);

    RegCloseKey(hKey);
}

// ── Schedule logic ────────────────────────────────────────────────────────────

bool IsInScheduleWindow() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    int now   = st.wHour * 60 + st.wMinute;
    int start = schedOnHour  * 60 + schedOnMin;
    int end   = schedOffHour * 60 + schedOffMin;

    if (start == end) return false;

    if (start < end) {
        return now >= start && now < end;       // e.g. 08:00 – 20:00
    } else {
        return now >= start || now < end;       // overnight, e.g. 22:00 – 06:00
    }
}

void CheckSchedule() {
    if (!schedEnabled) return;
    bool shouldBeOn = IsInScheduleWindow();
    if (shouldBeOn != isRedlightActive)
        ToggleRedlight();
}

// ── WinMain ───────────────────────────────────────────────────────────────────

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

    LoadSchedule();
    SetTimer(hwndTray, ID_SCHEDULE_TIMER, 30000, NULL); // check every 30 seconds
    CheckSchedule(); // apply schedule state on startup

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

// ── Tray helpers ──────────────────────────────────────────────────────────────

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
    const HINSTANCE hInstance = GetModuleHandle(NULL);
    char aboutText[512];
    sprintf_s(aboutText, sizeof(aboutText), "RedLight v0.4.0-beta\n\ngithub.com/michaelmawhinney/redlight");
    const char* aboutTitle = "About";

    MessageBox(parent, aboutText, aboutTitle, MB_OK | MB_ICONINFORMATION);
}

// ── Schedule dialog ───────────────────────────────────────────────────────────

INT_PTR CALLBACK ScheduleDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_SCHED_ENABLE, schedEnabled ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(hwnd, IDC_START_HOUR, schedOnHour,  FALSE);
        SetDlgItemInt(hwnd, IDC_START_MIN,  schedOnMin,   FALSE);
        SetDlgItemInt(hwnd, IDC_END_HOUR,   schedOffHour, FALSE);
        SetDlgItemInt(hwnd, IDC_END_MIN,    schedOffMin,  FALSE);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            schedEnabled = (IsDlgButtonChecked(hwnd, IDC_SCHED_ENABLE) == BST_CHECKED);

            int h, m;
            h = (int)GetDlgItemInt(hwnd, IDC_START_HOUR, NULL, FALSE);
            m = (int)GetDlgItemInt(hwnd, IDC_START_MIN,  NULL, FALSE);
            schedOnHour = h < 0 ? 0 : h > 23 ? 23 : h;
            schedOnMin  = m < 0 ? 0 : m > 59 ? 59 : m;

            h = (int)GetDlgItemInt(hwnd, IDC_END_HOUR, NULL, FALSE);
            m = (int)GetDlgItemInt(hwnd, IDC_END_MIN,  NULL, FALSE);
            schedOffHour = h < 0 ? 0 : h > 23 ? 23 : h;
            schedOffMin  = m < 0 ? 0 : m > 59 ? 59 : m;

            SaveSchedule();
            CheckSchedule();
            EndDialog(hwnd, IDOK);
            return TRUE;
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowScheduleDialog(HWND parent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SCHEDULE), parent, ScheduleDlgProc);
}

// ── Window proc ───────────────────────────────────────────────────────────────

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                AppendMenu(hMenu, MF_STRING, ID_TRAY_TOGGLE,   TEXT("Toggle ON/off"));
                AppendMenu(hMenu, MF_STRING, ID_TRAY_SCHEDULE, TEXT("Schedule..."));
                AppendMenu(hMenu, MF_STRING, ID_TRAY_ABOUT,    TEXT("About"));
                AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT,     TEXT("Exit"));
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
        } else if (LOWORD(wParam) == ID_TRAY_SCHEDULE) {
            ShowScheduleDialog(hwnd);
        } else if (LOWORD(wParam) == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == ID_TRAY_ABOUT) {
            ShowAboutDialog(hwnd);
        }
        break;

    case WM_TIMER:
        if (wParam == ID_SCHEDULE_TIMER)
            CheckSchedule();
        break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
