#pragma once

#include "DisplayFilter.h"

#include <windows.h>

class GammaRampFilter final : public DisplayFilter {
public:
    GammaRampFilter();
    ~GammaRampFilter() override;

    bool initialize() override;
    bool enable() override;
    bool disable() override;
    bool isActive() const override;
    void shutdown() override;
    const char* name() const override;

private:
    void SetError(const char* action, DWORD error);
    bool SetGammaRamp(WORD ramp[3][256], const char* action);
    void BuildRedRamp();

    HDC hdc_;
    bool initialized_;
    bool active_;
    WORD originalGammaRamp_[3][256];
    WORD redGammaRamp_[3][256];
    char lastError_[256];
};
