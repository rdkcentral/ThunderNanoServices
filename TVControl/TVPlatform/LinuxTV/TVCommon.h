#ifndef LINUXCOMMON_H
#define LINUXCOMMON_H

#include <condition_variable>
#include <fcntl.h>
#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <libdvbapi/dvbfe.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <map>
#include <memory>
#include <mutex>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define DVB_ADAPTER_SCAN 6

enum TunerChangedOperation {
    Added,
    Removed
};

enum SourceType {
    DvbT,
    DvbT2,
    DvbC,
    DvbC2,
    DvbS,
    DvbS2,
    DvbH,
    DvbSh,
    Atsc,
    AtscMH,
    IsdbT,
    IsdbTb,
    IsdbS,
    IsdbC,
    _1seg,
    Dtmb,
    Cmmb,
    TDmb,
    SDmb,
    Undifined
};

struct dvbfe_handle {
    int32_t fd;
    enum dvbfe_type type;
    char* name;
};

struct TunerData {
    TunerData() = default;
    ~TunerData() = default;
    std::string tunerId;
    uint32_t modulation;
    std::vector<uint32_t> frequency;
};

enum ChannelList {
    AtscVsb = 1,
    AtscQam = 2,
    DvbTAu = 3,
    DvbTDe = 4,
    DvbTFr = 5,
    DvbTGb = 6,
    DvbCQam = 7,
    DvbCFi = 8,
    DvbCFr = 9,
    DvbCBr = 10,
    IsdbT6MHz = 11,
    UserList = 999
};

/******************************************************************************
 * PIDs as defined for accessing tables.
 *
 *****************************************************************************/
enum PidType {
    PidPat = 0x0000,
    PidCat = 0x0001,
    PidTsdt = 0x0002,
    PidNitSt = 0x0010,
    PidSdtBatSt = 0x0011,
    PidEitStCit = 0x0012,
    PidRstSt = 0x0013,
    PidTdtTotSt = 0x0014,
    PidRnt = 0x0016,
    PidDit = 0x001E,
    PidSit = 0x001F,
    PidVct = 0x1FFB,
};

/******************************************************************************
 * table ids as defined by standards.
 *
 *****************************************************************************/

enum TableId {
    TablePat = 0x00, // program_association_section.
    TableCat = 0x01, // conditional_access_section.
    TablePmt = 0x02, // program_map_section.
    TableTsdt = 0x03, // transport_stream_description_section.
    TableNitAct = 0x40, // network_information_section - actual_network.
    TableNitOth = 0x41, // network_information_section - other_network.
    TableSdtAct = 0x42, // service_description_section - actual_transport_stream.
    TableSdtOth = 0x46, // service_description_section - other_transport_stream.
    TableBat = 0x4A, // bouquet_association_section.
    TableEitAct = 0x4E, // event_information_section - actual_transport_stream, present/following.
    TableEitOth = 0x4F, // event_information_section - other_transport_stream, present/following.
    TableEitScheduleAct50 = 0x50, // 0x50 to 0x5F event_information_section - actual_transport_stream, schedule.
    TableEitScheduleAct5F = 0x5F,
    TableEitScheduleOth60 = 0x60, // 0x60 to 0x6F event_information_section - other_transport_stream, schedule.
    TableEitScheduleOth6F = 0x6F,
    TableTdt = 0x70, // time_date_section.
    TableRst = 0x71, // running_status_section.
    TableStuffing = 0x72, // stuffing_section.
    TableTot = 0x73, // time_offset_section.
    TableAit = 0x74, // application information section (TS 102 812 [17]).
    TableCst = 0x75, // container section (TS 102 323 [15]).
    TableRct = 0x76, // related content section (TS 102 323 [15]).
    TableCit = 0x77, // content identifier section (TS 102 323 [15]).
    TableMpeFec = 0x78,
    TableRns = 0x79, // resolution notification section (TS 102 323 [15]).
    TableDit = 0x7E, // discontinuity_information_section.
    TableSit = 0x7F, // selection_information_section.
    TablePremiere_Cit = 0xA0, // premiere content information section.
    TableVctTerr = 0xC8, // ATSC VCT VSB (terr).
    TableVctCable = 0xC9, // ATSC VCT QAM (cable).
};

inline struct dvbfe_handle* OpenFE(std::string& tunerId)
{
    // Change id to int.
    int32_t adapter = stoi(tunerId.substr(0, tunerId.find(":")));
    int32_t frontend = stoi(tunerId.substr(tunerId.find(":") + 1));
    // Open dvbFe.
    return dvbfe_open(adapter, frontend, 0);
}

struct SrcTypesVector {
    SourceType* types;
    uint64_t length;
};
#endif
