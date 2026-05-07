#pragma once

#include "DisplayFilter.h"

#include <windows.h>
#include <magnification.h>

class MagnificationFilter final : public DisplayFilter {
public:
    MagnificationFilter();
    ~MagnificationFilter() override;

    bool initialize() override;
    bool enable() override;
    bool disable() override;
    bool isActive() const override;
    void shutdown() override;
    const char* name() const override;

private:
    void LogError(const char* action, DWORD error);
    bool SetColorEffect(const MAGCOLOREFFECT& effect, const char* action);

    bool initialized_;
    bool active_;
    MAGCOLOREFFECT restoreEffect_;
    char lastError_[256];
};
