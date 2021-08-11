#pragma once
#include "Module.h"

class Math : public WPEFramework::Exchange::IMath {
public:
    Math(const Math&) = delete;
    Math& operator=(const Math&) = delete;

    Math()
    {
    }
    ~Math() override
    {
    }

public:
    // Inherited via IMath
    uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A + B;
        return (WPEFramework::Core::ERROR_NONE);
    }
    uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A - B;
        return (WPEFramework::Core::ERROR_NONE);
    }

    BEGIN_INTERFACE_MAP(Math)
    INTERFACE_ENTRY(WPEFramework::Exchange::IMath)
    END_INTERFACE_MAP
};