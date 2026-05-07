#include <windows.h>
#include <magnification.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace {

MAGCOLOREFFECT kIdentityEffect = {{
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
}};

MAGCOLOREFFECT kRedOnlyEffect = {{
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
}};

MAGCOLOREFFECT g_restoreEffect = kIdentityEffect;
bool g_hasRestoreEffect = false;
bool g_magInitialized = false;
std::atomic<bool> g_cleanupStarted{false};

template <typename Fn>
auto LogApiCall(const char* name, Fn&& fn) -> decltype(fn()) {
    SetLastError(0);
    const auto result = fn();
    const DWORD error = GetLastError();
    std::printf("%s => %d, GetLastError=%lu\n", name, static_cast<int>(result), static_cast<unsigned long>(error));
    return result;
}

void PrintPrompt() {
    std::puts("Press Enter to restore the previous color matrix and exit.");
}

void RestoreDisplayState() {
    bool expected = false;
    if (!g_cleanupStarted.compare_exchange_strong(expected, true)) {
        return;
    }

    if (g_magInitialized) {
        MAGCOLOREFFECT* effect = g_hasRestoreEffect ? &g_restoreEffect : &kIdentityEffect;
        const BOOL restored = LogApiCall("MagSetFullscreenColorEffect(restore)", [effect]() {
            return MagSetFullscreenColorEffect(effect);
        });
        if (!restored) {
            std::puts("Failed to restore the prior color matrix.");
        }
    }

    if (g_magInitialized) {
        const BOOL uninitialized = LogApiCall("MagUninitialize", []() {
            return MagUninitialize();
        });
        if (!uninitialized) {
            std::puts("MagUninitialize reported failure.");
        }
        g_magInitialized = false;
    }
}

BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        std::puts("Console shutdown requested; attempting cleanup.");
        RestoreDisplayState();
        return TRUE;
    default:
        return FALSE;
    }
}

}  // namespace

int main() {
    std::puts("MagRedProbe starting.");

    (void)LogApiCall("SetConsoleCtrlHandler", []() {
        return SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    });

    const BOOL initialized = LogApiCall("MagInitialize", []() {
        return MagInitialize();
    });
    if (!initialized) {
        std::puts("MagInitialize failed; exiting without applying a color effect.");
        return EXIT_FAILURE;
    }
    g_magInitialized = true;

    const BOOL previousCaptured = LogApiCall("MagGetFullscreenColorEffect", []() {
        return MagGetFullscreenColorEffect(&g_restoreEffect);
    });
    g_hasRestoreEffect = previousCaptured != FALSE;
    if (!g_hasRestoreEffect) {
        g_restoreEffect = kIdentityEffect;
        std::puts("MagGetFullscreenColorEffect failed; identity will be used as the fallback restore matrix.");
    }

    const BOOL redApplied = LogApiCall("MagSetFullscreenColorEffect(red-only)", []() {
        return MagSetFullscreenColorEffect(&kRedOnlyEffect);
    });
    if (!redApplied) {
        std::puts("Failed to apply the red-only matrix; attempting cleanup.");
    } else {
        PrintPrompt();
        (void)std::getchar();
    }

    RestoreDisplayState();

    std::puts("MagRedProbe finished.");
    return redApplied ? EXIT_SUCCESS : EXIT_FAILURE;
}
