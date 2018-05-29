#ifndef IITABLEDATAHANDLER_H
#define IITABLEDATAHANDLER_H
#include <inttypes.h>
#include <stdio.h>
#include <string>
class ITableDataHandler {
public:
    virtual void EITBroadcast() = 0;
    virtual void EmergencyAlert() = 0;
    virtual void ParentalControlChanged() = 0;
    virtual void ParentalLockChanged(const std::string&) = 0;
};
#endif
