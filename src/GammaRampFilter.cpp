#include "GammaRampFilter.h"

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
        return false;
    }

    if (!GetDeviceGammaRamp(hdc_, originalGammaRamp_)) {
        SetError("GetDeviceGammaRamp", GetLastError());
        ReleaseDC(NULL, hdc_);
        hdc_ = NULL;
        return false;
    }

    BuildRedRamp();
    initialized_ = true;
    active_ = false;
    lastError_[0] = '\0';
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
    return true;
}

bool GammaRampFilter::isActive() const {
    return active_;
}

void GammaRampFilter::shutdown() {
    if (!initialized_) {
        return;
    }

    if (hdc_) {
        if (!SetGammaRamp(originalGammaRamp_, "shutdown restore")) {
            OutputDebugStringA("GammaRampFilter: shutdown restore failed; display state may not have been restored.\n");
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
    OutputDebugStringA(lastError_);
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
