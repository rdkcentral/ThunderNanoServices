#include "Module.h"
#include "DSResolutionHAL.h"


namespace WPEFramework {

DSResolutionHAL::DSResolutionHAL()
    :_vopHandle(0)
    ,_vopType(dsVIDEOPORT_TYPE_HDMI)
    ,_enabled(false)
    ,_connected(false)
    ,_isOperational(false)
    ,_dsRet(dsERR_UNKNOWN)
{
    if (dsHostInit() == dsERR_NONE){
        if (dsVideoPortInit() == dsERR_NONE){
            if (dsGetVideoPort((dsVideoPortType_t)_vopType,0,&_vopHandle) == dsERR_NONE) {
                if (dsIsVideoPortEnabled(_vopHandle,&_enabled) == dsERR_NONE) {
                    if (dsIsDisplayConnected(_vopHandle,&_connected) == dsERR_NONE) 
                        _isOperational = true;
                }
            }
         }
    }
}

DSResolutionHAL::~DSResolutionHAL()
{
    _isOperational = false;
}

uint16_t DSResolutionHAL::GetDSResolution() 
{
    TRACE(Trace::Information, (_T("GetDSResolution")));
    int _vopHandle = 0;
    dsVideoPortResolution_t resolution;
    if (dsGetResolution(_vopHandle,&resolution) == dsERR_NONE) 
        return (uint16_t)resolution.pixelResolution;
    return dsVIDEO_PIXELRES_MAX+1;
}

bool  DSResolutionHAL::SetDSResolution(uint16_t pixelResolution) 
{
    TRACE(Trace::Information, (_T("SetDSResolution")));
    int _vopHandle = 0;
    dsVideoPortResolution_t resolution;
    resolution.pixelResolution = (dsVideoResolution_t)pixelResolution;
    if (dsSetResolution(_vopHandle,&resolution) == dsERR_NONE) 
        return true;
    return false;
}

} /* namespace WPEFramework */
