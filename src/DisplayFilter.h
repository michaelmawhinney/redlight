#pragma once

enum class RedMode {
    StrictRed,
    LumaRed,
};

inline const char* RedModeDisplayName(RedMode mode) {
    switch (mode) {
    case RedMode::StrictRed:
        return "Strict Red";
    case RedMode::LumaRed:
        return "Luma Red";
    }

    return "Unknown";
}

class DisplayFilter {
public:
    virtual ~DisplayFilter() = default;

    virtual bool initialize() = 0;
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual bool isActive() const = 0;
    virtual bool setRedMode(RedMode mode) = 0;
    virtual RedMode redMode() const = 0;
    virtual bool supportsRedMode(RedMode mode) const = 0;
    virtual void shutdown() = 0;
    virtual const char* name() const = 0;
};
