#include "GammaRampFilter.h"
#include "Diagnostics.h"

#include <cstdio>

namespace {

constexpr const char* kFilterName = "GammaRampFilter";

}

GammaRampFilter::GammaRampFilter()
    : hdc_(NULL),
      initialized_(false),
      active_(false),
      originalGammaRamp_{},
      redGammaRamp_{},
      lastError_{} {}

GammaRampFilter::~GammaRampFilter() {
    shutdown();
}

bool GammaRampFilter::initialize() {
    if (initialized_) {
        return true;
    }

    hdc_ = GetDC(NULL);
    if (!hdc_) {
        SetError("GetDC", GetLastError());
        Diagnostics::LogFormat("%s initialization failed.", kFilterName);
        return false;
    }

    if (!GetDeviceGammaRamp(hdc_, originalGammaRamp_)) {
        SetError("GetDeviceGammaRamp", GetLastError());
        ReleaseDC(NULL, hdc_);
        hdc_ = NULL;
        Diagnostics::LogFormat("%s initialization failed.", kFilterName);
        return false;
    }

    BuildRedRamp();
    initialized_ = true;
    active_ = false;
    lastError_[0] = '\0';
    Diagnostics::LogFormat("%s initialization succeeded.", kFilterName);
    return true;
}

bool GammaRampFilter::enable() {
    if (!initialized_) {
        SetError("enable before initialize", ERROR_INVALID_HANDLE);
        return false;
    }

    if (active_) {
        return true;
    }

    if (!SetGammaRamp(redGammaRamp_, "SetDeviceGammaRamp(red)")) {
        return false;
    }

    active_ = true;
    Diagnostics::LogFormat("%s enable succeeded.", kFilterName);
    return true;
}

bool GammaRampFilter::disable() {
    if (!initialized_) {
        SetError("disable before initialize", ERROR_INVALID_HANDLE);
        return false;
    }

    if (!active_) {
        return true;
    }

    if (!SetGammaRamp(originalGammaRamp_, "SetDeviceGammaRamp(restore)")) {
        return false;
    }

    active_ = false;
    Diagnostics::LogFormat("%s disable succeeded.", kFilterName);
    return true;
}

bool GammaRampFilter::isActive() const {
    return active_;
}

bool GammaRampFilter::setRedMode(RedMode mode) {
    if (mode == RedMode::StrictRed) {
        return true;
    }

    Diagnostics::Log("GammaRampFilter: Luma Red requested but gamma ramps do not support cross-channel mixing; staying in Strict Red.");
    return false;
}

RedMode GammaRampFilter::redMode() const {
    return RedMode::StrictRed;
}

bool GammaRampFilter::supportsRedMode(RedMode mode) const {
    return mode == RedMode::StrictRed;
}

void GammaRampFilter::shutdown() {
    if (!initialized_) {
        return;
    }

    if (hdc_) {
        Diagnostics::LogFormat("%s shutdown restore attempt.", kFilterName);
        if (!SetGammaRamp(originalGammaRamp_, "shutdown restore")) {
            Diagnostics::Log("GammaRampFilter: shutdown restore failed; display state may not have been restored.");
        } else {
            active_ = false;
        }

        if (!ReleaseDC(NULL, hdc_)) {
            SetError("ReleaseDC", GetLastError());
        }
        hdc_ = NULL;
    }

    initialized_ = false;
}

const char* GammaRampFilter::name() const {
    return kFilterName;
}

void GammaRampFilter::SetError(const char* action, DWORD error) {
    std::snprintf(lastError_, sizeof(lastError_), "%s: %s failed (GetLastError=%lu)\n",
        kFilterName,
        action,
        static_cast<unsigned long>(error));
    Diagnostics::LogFormat("%s: %s failed (GetLastError=%lu)", kFilterName, action, static_cast<unsigned long>(error));
}

bool GammaRampFilter::SetGammaRamp(WORD ramp[3][256], const char* action) {
    if (!hdc_) {
        SetError(action, ERROR_INVALID_HANDLE);
        return false;
    }

    if (!SetDeviceGammaRamp(hdc_, ramp)) {
        SetError(action, GetLastError());
        return false;
    }

    lastError_[0] = '\0';
    return true;
}

void GammaRampFilter::BuildRedRamp() {
    for (int i = 0; i < 256; ++i) {
        redGammaRamp_[0][i] = static_cast<WORD>(i << 8);
        redGammaRamp_[1][i] = 0;
        redGammaRamp_[2][i] = 0;
    }
}
