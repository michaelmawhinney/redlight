#pragma once

class DisplayFilter {
public:
    virtual ~DisplayFilter() = default;

    virtual bool initialize() = 0;
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual bool isActive() const = 0;
    virtual void shutdown() = 0;
    virtual const char* name() const = 0;
};
