#include "RtspSessionInfo.h"

namespace WPEFramework {
namespace Plugin {

RtspSessionInfo::RtspSessionInfo()
    : bSrmIsRtspProxy(true)
    , sessionTimeout(-1)
    , ctrlSessionTimeout(-1)
    , duration(0)
    , bookmark(0)
    , npt(0)
    , scale(0)
{
    defaultSessionTimeout = 0;
    defaultCtrlSessionTimeout = 0;
}

void RtspSessionInfo::reset()
{
    npt = 0;
    duration = 0;
    bookmark = 0;
    scale = 0;

}

}} // WPEFramework::Plugin

