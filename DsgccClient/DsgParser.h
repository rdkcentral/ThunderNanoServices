#ifndef _DSGPARSER_H
#define _DSGPARSER_H

#include "Module.h"

namespace WPEFramework {
namespace Plugin {
//namespace Dsg {

struct revdesc {
  unsigned char valid;
  int version;
  char *parts;
};

struct cds_table {
  int           cd[256];
  unsigned char written[256];
  struct revdesc revdesc;
};

struct mms_record {
  const char *modulation_fmt;
};

struct mms_table {
  struct mms_record mm[256];
  unsigned char written[256];
  struct revdesc revdesc;
};

struct sns_record {
  struct sns_record *next;
  unsigned char app_type;
  int id; //source id
  int namelen; // source name length
  char mode; //Multilingual Text String (MTS) Format <mode><length><segment> [ <mode><length><segment> ]
  char length;
  char segment[32];
  int descriptor_count;
  char descriptors[256];
};

struct ntt_table {
  struct sns_record *sns_list;
  struct revdesc revdesc;
};

struct vc_record {
  struct vc_record *next;
  int vc;
  char application;
  char path_select;
  char transport_type;
  char channel_type;
  unsigned int id;
  int cds_ref;
  int prognum;
  int mms_ref;
  unsigned char scrambled;
  unsigned char video_std;
  unsigned char desc_cnt;
  unsigned char descTag[256];  //unsigned char descTag[desc_cnt];
  unsigned int descLength;
  //unsigned char descValue[5]; //unsigned char descValue[desc_cnt][descLength];
  unsigned int  chusId[256];
};

struct vcm {
  struct vcm *next;
  struct vc_record *vc_list;
  unsigned int vctid;
  struct revdesc revdesc;
};


class Channel : public Core::JSON::Container {
public:
    Channel& operator=(const Channel&) = delete;

public:
    Channel()
        : Core::JSON::Container()
    {
        Add(_T("channelNumber"), &ChannelNumber);
        Add(_T("callNumber"), &CallNumber);
        Add(_T("freq"), &Freq);
        Add(_T("programNumber"), &ProgramNumber);
        Add(_T("modulation"), &Modulation);
        Add(_T("chuId"), &ChuId);
        Add(_T("sourceId"), &SourceId);
        Add(_T("description"), &Description);
    }
    ~Channel()
    {
    }
    Channel(const Channel& rh)
        : Core::JSON::Container()
        , ChannelNumber(rh.ChannelNumber)
        , CallNumber(rh.CallNumber)
        , Freq(rh.Freq)
        , ProgramNumber(rh.ProgramNumber)
        , Modulation(rh.Modulation)
        , ChuId(rh.ChuId)
        , SourceId(rh.SourceId)
        , Description(rh.Description)
    {
        Add(_T("channelNumber"), &ChannelNumber);
        Add(_T("callNumber"), &CallNumber);
        Add(_T("freq"), &Freq);
        Add(_T("programNumber"), &ProgramNumber);
        Add(_T("modulation"), &Modulation);
        Add(_T("chuId"), &ChuId);
        Add(_T("sourceId"), &SourceId);
        Add(_T("description"), &Description);
    }

public:
    Core::JSON::DecUInt16 ChannelNumber;
    Core::JSON::DecUInt16 CallNumber;
    Core::JSON::DecUInt16 Freq;
    Core::JSON::DecUInt16 ProgramNumber;
    Core::JSON::DecUInt16 Modulation;
    Core::JSON::String ChuId;
    Core::JSON::DecUInt32 SourceId;
    Core::JSON::String Description;
};


class DsgParser {
public:
    DsgParser(int vctId)
        : _vctId(vctId)
        , startTime (Core::Time::Now().Ticks())
    {
        TRACE_L1("VctId=%d", _vctId);
    }

    bool isDone() {
        return allDone;
    }

    string getChannels() {
        return channels;
    }

    void parse(unsigned char *pBuf, ssize_t len);
    bool parse_cds(unsigned char *buf, int len, struct cds_table *cds);
    bool parse_mms(unsigned char *buf, int len, struct mms_table *mms);
    bool parse_ntt(unsigned char *buf, int len, struct ntt_table *ntt);
    bool parse_svct(unsigned char *buf, int len, struct vcm **vcmlist, int vctidfilter, int &vct_lookup_index);
    struct vc_record* reverse_vc_list(struct vc_record *vc_rec);
    int read_vc(unsigned char *buf, struct vc_record *vc_rec, unsigned char desc_inc);

    string output_txt(struct cds_table *cds, struct mms_table *mms, struct ntt_table *ntt, struct vcm *vcm_list);
    void HexDump(const char* label, const std::string& msg, uint16_t charsPerLine = 32);

private:
    int _vctId;
    string channels;
    uint64_t startTime;

    struct cds_table cds;
    // 8bit S-VCT MMS_reference means...
    struct mms_table mms;
    // infinite sized SNS and VCM, so use linked lists
    struct ntt_table ntt;
    struct vcm *vcm_list= NULL;
    time_t start_time;

    bool cdsDone = false;
    bool mmsDone = false;
    bool nitDone = false;
    bool nttDone = false;
    bool svctDone = false;
    bool allDone  = false;
};

//} // Dsg
} // namespace Plugin
} // namespace WPEFramework

#endif // _DSGPARSER_H
