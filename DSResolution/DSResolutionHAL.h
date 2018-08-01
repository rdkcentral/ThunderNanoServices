#ifndef DSRESOLUTION_HAL_H
#define DSRESOLUTION_HAL_H

#include "Module.h"
#include "dsVideoPort.h"

namespace WPEFramework {
class DSResolutionHAL {
private:
    DSResolutionHAL(const DSResolutionHAL&) = delete;
    DSResolutionHAL& operator=(const DSResolutionHAL&) = delete;

public:
    static Core::ProxyType<DSResolutionHAL> Create () {
        return (Core::ProxyType<DSResolutionHAL>::Create());
    }
    DSResolutionHAL();
    virtual ~DSResolutionHAL();
    uint16_t GetDSResolution(); 
    bool SetDSResolution(uint16_t resolution);
    inline bool IsOperational () const {
        return (_isOperational);
    }
private:
    int _vopHandle;
    int  _vopType;
    bool _enabled;
    bool _connected;
    bool _isOperational;
    dsError_t _dsRet;
};

} /* namespace WPEFramework */
#endif /* DSRESOLUTION_HAL_H */
