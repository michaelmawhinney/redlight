#include "Diagnostics.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

HANDLE g_logFile = INVALID_HANDLE_VALUE;
bool g_initialized = false;
SRWLOCK g_lock = SRWLOCK_INIT;

std::string BuildLogPath() {
    char localAppData[32768] = {};
    const DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", localAppData, static_cast<DWORD>(sizeof(localAppData)));
    if (length == 0 || length >= sizeof(localAppData)) {
        return {};
    }

    std::string directory = localAppData;
    directory += "\\RedLight";

    if (!CreateDirectoryA(directory.c_str(), NULL)) {
        const DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            return {};
        }
    }

    directory += "\\redlight.log";
    return directory;
}

std::string BuildTimestamp() {
    SYSTEMTIME st = {};
    GetLocalTime(&st);

    char timestamp[64] = {};
    std::snprintf(timestamp, sizeof(timestamp), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        static_cast<unsigned>(st.wYear),
        static_cast<unsigned>(st.wMonth),
        static_cast<unsigned>(st.wDay),
        static_cast<unsigned>(st.wHour),
        static_cast<unsigned>(st.wMinute),
        static_cast<unsigned>(st.wSecond),
        static_cast<unsigned>(st.wMilliseconds));
    return timestamp;
}

void WriteLine(const char* message) {
    std::string line = BuildTimestamp();
    line += " ";
    line += message ? message : "";
    line += "\r\n";

    OutputDebugStringA(line.c_str());

    if (g_logFile == INVALID_HANDLE_VALUE) {
        return;
    }

    AcquireSRWLockExclusive(&g_lock);
    DWORD written = 0;
    (void)WriteFile(g_logFile, line.c_str(), static_cast<DWORD>(line.size()), &written, NULL);
    ReleaseSRWLockExclusive(&g_lock);
}

}  // namespace

namespace Diagnostics {

void Initialize() {
    if (g_initialized) {
        return;
    }

    const std::string path = BuildLogPath();
    if (!path.empty()) {
        g_logFile = CreateFileA(
            path.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (g_logFile == INVALID_HANDLE_VALUE) {
            OutputDebugStringA("RedLight: diagnostics log file unavailable; continuing with OutputDebugStringA only.\n");
        }
    }

    g_initialized = true;
}

void Log(const char* message) {
    if (!message) {
        return;
    }

    if (!g_initialized) {
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
        return;
    }

    WriteLine(message);
}

void LogFormat(const char* format, ...) {
    if (!format) {
        return;
    }

    char buffer[1024] = {};
    va_list args;
    va_start(args, format);
    (void)vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);

    Log(buffer);
}

void Shutdown() {
    if (!g_initialized) {
        return;
    }

    if (g_logFile != INVALID_HANDLE_VALUE) {
        AcquireSRWLockExclusive(&g_lock);
        (void)FlushFileBuffers(g_logFile);
        (void)CloseHandle(g_logFile);
        g_logFile = INVALID_HANDLE_VALUE;
        ReleaseSRWLockExclusive(&g_lock);
    }

    g_initialized = false;
}

}  // namespace Diagnostics
