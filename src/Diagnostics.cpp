#include "Diagnostics.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

HANDLE g_logFile = INVALID_HANDLE_VALUE;
bool g_initialized = false;
SRWLOCK g_lock = SRWLOCK_INIT;

constexpr ULONGLONG kMaxLogSizeBytes = 1ull << 20;

std::string BuildLogDirectory() {
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

    return directory;
}

std::string BuildLogPath(const char* fileName) {
    std::string directory = BuildLogDirectory();
    if (directory.empty()) {
        return {};
    }

    directory += "\\";
    directory += fileName ? fileName : "";
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

void WriteDebugFailure(const char* action, const std::string& path, DWORD error) {
    char buffer[512] = {};
    std::snprintf(buffer, sizeof(buffer), "RedLight: diagnostics %s failed for %s (error %lu)\n",
        action ? action : "operation",
        path.c_str(),
        static_cast<unsigned long>(error));
    OutputDebugStringA(buffer);
}

bool IsLogTooLarge(const std::string& path) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }

    const ULONGLONG size = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) |
        static_cast<ULONGLONG>(data.nFileSizeLow);
    return size > kMaxLogSizeBytes;
}

void BestEffortDeleteFile(const std::string& path) {
    if (!DeleteFileA(path.c_str())) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND) {
            WriteDebugFailure("delete", path, error);
        }
    }
}

void BestEffortMoveFile(const std::string& from, const std::string& to) {
    if (!MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return;
        }

        std::string path = from;
        path += " -> ";
        path += to;
        WriteDebugFailure("rename", path, error);
    }
}

void RotateLogsIfNeeded() {
    const std::string logPath = BuildLogPath("redlight.log");
    if (logPath.empty() || !IsLogTooLarge(logPath)) {
        return;
    }

    const std::string log1Path = BuildLogPath("redlight.1.log");
    const std::string log2Path = BuildLogPath("redlight.2.log");
    const std::string log3Path = BuildLogPath("redlight.3.log");
    if (log1Path.empty() || log2Path.empty() || log3Path.empty()) {
        return;
    }

    BestEffortDeleteFile(log3Path);
    BestEffortMoveFile(log2Path, log3Path);
    BestEffortMoveFile(log1Path, log2Path);
    BestEffortMoveFile(logPath, log1Path);
}

void OpenLogFile() {
    const std::string path = BuildLogPath("redlight.log");
    if (path.empty()) {
        return;
    }

    RotateLogsIfNeeded();

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

    OpenLogFile();
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
