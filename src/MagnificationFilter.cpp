#include "MagnificationFilter.h"
#include "Diagnostics.h"

#include <cstdio>

namespace {

constexpr const char* kFilterName = "MagnificationFilter";

constexpr MAGCOLOREFFECT kIdentityEffect = {{
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
}};

constexpr MAGCOLOREFFECT kRedOnlyEffect = {{
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
}};

}  // namespace

MagnificationFilter::MagnificationFilter()
    : initialized_(false),
      active_(false),
      restoreEffect_(kIdentityEffect),
      lastError_{} {}

MagnificationFilter::~MagnificationFilter() {
    shutdown();
}

bool MagnificationFilter::initialize() {
    if (initialized_) {
        return true;
    }

    if (!MagInitialize()) {
        LogError("MagInitialize", GetLastError());
        Diagnostics::LogFormat("%s initialization failed.", kFilterName);
        return false;
    }

    MAGCOLOREFFECT capturedEffect = {};
    if (MagGetFullscreenColorEffect(&capturedEffect)) {
        restoreEffect_ = capturedEffect;
    } else {
        LogError("MagGetFullscreenColorEffect", GetLastError());
        restoreEffect_ = kIdentityEffect;
        Diagnostics::Log("MagnificationFilter: using identity restore matrix because the previous full-screen color effect could not be captured.");
    }

    initialized_ = true;
    active_ = false;
    lastError_[0] = '\0';
    Diagnostics::LogFormat("%s initialization succeeded.", kFilterName);
    return true;
}

bool MagnificationFilter::enable() {
    if (!initialized_) {
        LogError("enable before initialize", ERROR_INVALID_HANDLE);
        return false;
    }

    if (active_) {
        return true;
    }

    if (!SetColorEffect(kRedOnlyEffect, "MagSetFullscreenColorEffect(red-only)")) {
        return false;
    }

    active_ = true;
    Diagnostics::LogFormat("%s enable succeeded.", kFilterName);
    return true;
}

bool MagnificationFilter::disable() {
    if (!initialized_) {
        LogError("disable before initialize", ERROR_INVALID_HANDLE);
        return false;
    }

    if (!active_) {
        return true;
    }

    if (!SetColorEffect(restoreEffect_, "MagSetFullscreenColorEffect(restore)")) {
        return false;
    }

    active_ = false;
    Diagnostics::LogFormat("%s disable succeeded.", kFilterName);
    return true;
}

bool MagnificationFilter::isActive() const {
    return active_;
}

void MagnificationFilter::shutdown() {
    if (!initialized_) {
        return;
    }

    if (active_) {
        Diagnostics::LogFormat("%s shutdown restore attempt.", kFilterName);
        if (!SetColorEffect(restoreEffect_, "shutdown restore")) {
            Diagnostics::Log("MagnificationFilter: shutdown restore failed; display state may not have been restored.");
        } else {
            active_ = false;
        }
    }

    if (!MagUninitialize()) {
        LogError("MagUninitialize", GetLastError());
    } else {
        Diagnostics::LogFormat("%s shutdown completed.", kFilterName);
    }

    initialized_ = false;
}

const char* MagnificationFilter::name() const {
    return kFilterName;
}

void MagnificationFilter::LogError(const char* action, DWORD error) {
    std::snprintf(lastError_, sizeof(lastError_), "%s: %s failed (GetLastError=%lu)\n",
        kFilterName,
        action,
        static_cast<unsigned long>(error));
    Diagnostics::LogFormat("%s: %s failed (GetLastError=%lu)", kFilterName, action, static_cast<unsigned long>(error));
}

bool MagnificationFilter::SetColorEffect(const MAGCOLOREFFECT& effect, const char* action) {
    if (!initialized_) {
        LogError(action, ERROR_INVALID_HANDLE);
        return false;
    }

    MAGCOLOREFFECT mutableEffect = effect;
    if (!MagSetFullscreenColorEffect(&mutableEffect)) {
        LogError(action, GetLastError());
        return false;
    }

    lastError_[0] = '\0';
    return true;
}
