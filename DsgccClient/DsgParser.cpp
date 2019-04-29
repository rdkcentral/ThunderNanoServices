#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

#include "DsgParser.h"

#define SI_NIT_TABLE_ID  0xc2
#define SI_NTT_TABLE_ID  0xc3
#define SI_SVCT_TABLE_ID 0xc4

#define ELAPSED_TIME()  ((float_t)(Core::Time::Now().Ticks() - startTime) / 1000000)

namespace WPEFramework {
namespace Plugin {
//namespace Dsg {

static const char* scte_modfmt_table[] = {
  "UNKNOWN",
  "QPSK",
  "BPSK",
  "OQPSK",
  "8VSB",
  "16VSB",
  "QAM_16",
  "QAM_32",
  "QAM_64",
  "QAM_80",
  "QAM_96",
  "QAM_112",
  "QAM_128",
  "QAM_160",
  "QAM_192",
  "QAM_224",
  "QAM_256",
  "QAM_320",
  "QAM_384",
  "QAM_448",
  "QAM_512",
  "QAM_640",
  "QAM_768",
  "QAM_896",
  "QAM_1024",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED",
  "RESERVED"
};

void DsgParser::parse(unsigned char *pBuf, ssize_t len)
{
    int section_len = ((pBuf[1] & 0xf) << 8) | pBuf[2];
    if (section_len == (len-3)) {

        switch (pBuf[0]) {
        case SI_NIT_TABLE_ID:
            if (!nitDone) {
                if (section_len < 8) {
                  TRACE_L2("short NIT, got %d bytes (expected >7)", section_len);
                } else if (1 == (pBuf[6] & 0xf)) {
                    if (!cdsDone) {
                        TRACE_L1("[%3.3f] NIT : CDS", ELAPSED_TIME()); //1 CDS  Carrier Definition Subtable
                        HexDump("NIT-CDS:", std::string((char *)pBuf, len));
                        cdsDone = parse_cds(pBuf, section_len, &cds);
                    }
                } else if (2 == (pBuf[6] & 0xf)) {
                    if (!mmsDone) {
                        TRACE_L1("[%3.3f] NIT : MMS", ELAPSED_TIME()); //2 MMS  Modulation Mode Subtable
                        HexDump("NIT-MMS:", std::string((char *)pBuf, len));
                        mmsDone = parse_mms(pBuf, section_len, &mms);
                    }
                } else {
                    TRACE_L1("NIT tbl_subtype=%s", (pBuf[6] & 0xf) ? "RSVD" : "INVLD");  //3-15 Reserved, 0 invalid
                }
            }
            break;
        case SI_NTT_TABLE_ID:
            if(!nttDone) {
                HexDump("NTT:", std::string((char *)pBuf, len));

                nttDone = parse_ntt(pBuf, section_len, &ntt);
            }
            break;
        case SI_SVCT_TABLE_ID:
            if(!svctDone) {
                int vct_lookup_index = -1;
                HexDump("SVCT:", std::string((char *)pBuf, len));

                svctDone = parse_svct(pBuf, section_len, &vcm_list, _vctId, vct_lookup_index);
            }
            break;
        default:
            TRACE_L4("Unknown section 0x%x len=%d", pBuf[0], len);
            break;
        }
        if(cdsDone & mmsDone) nitDone = true;
        if(nitDone & nttDone & svctDone) allDone = true;

        TRACE_L2("cdsDone=%d mmsDone=%d nitDone=%d nttDone=%d svctDone=%d", cdsDone, mmsDone, nitDone, nttDone, svctDone);
        if (allDone) {
            // XXX: Notify
            TRACE_L1("[%3.3f]  All Done, Generating channel map", ELAPSED_TIME());
            channels = output_txt(&cds, &mms, &ntt, vcm_list);

            // dealloc memory
            while (ntt.sns_list) {
                struct sns_record *sns_next = ntt.sns_list->next;
                free (ntt.sns_list);
                ntt.sns_list=sns_next;
            }
            while (vcm_list) {
                struct vcm *vcm_next = vcm_list->next;
                while (vcm_list->vc_list) {
                    struct vc_record *vc_next = vcm_list->vc_list->next;
                    free(vcm_list->vc_list);
                    vcm_list->vc_list = vc_next;
                }
                free(vcm_list);
                vcm_list=vcm_next;
            }
        }
    } else {
        TRACE_L2("bad section length (expect %d, got %d)", len-3, section_len);
    }
}

bool DsgParser::parse_cds(unsigned char *buf, int len, struct cds_table *cds)
{
    unsigned char first_index = buf[4];
    unsigned char recs = buf[5];
    int index = first_index;
    int i, n;

    TRACE_L3("first_index = %d, %d recs", first_index, recs);
    n=7;

    for (i=0; i< recs; i++) {
        int j;
        int num_carriers = buf[n];
        int spacing_unit = (buf[n+1] & 0x80) ? 125000 : 10000;
        int freq_spacing = (((buf[n+1] & 0x3f) << 8) | buf[n+2]) * spacing_unit;
        int freq_unit = (buf[n+3] & 0x80) ? 125000 : 10000;
        int freq = (((buf[n+3] & 0x3f) << 8) | buf[n+4]) * freq_unit;

        n +=5;
        for (j=0; j< num_carriers; j++) {
            if (index > 255) TRACE_L2("CDS too big; noncompliant datastream?");
            cds->cd[index] = freq;
            cds->written[index] = 1;
            TRACE_L3("RF channel %d = %dhz", index, freq);
            index++;
            freq += freq_spacing;
        }
        // skip CD descriptors (should only be stuffing here anyway)
        n+=(buf[n]+1);
    }

    // parse NIT descriptors
    while (n < (len - 4 + 3)) { // 4 bytes CRC, 3 bytes header
        unsigned char d_tag = buf[n++];
        unsigned char d_len = buf[n++];

        if (d_tag == 0x93 && d_len >= 3) {
            unsigned char thissec = buf[n+1];
            unsigned char lastsec = buf[n+2];
            int i;

            cds->revdesc.valid = 1;
            cds->revdesc.version = buf[n] & 0x1f;
            TRACE_L3("CDS table version %d", cds->revdesc.version);
            TRACE_L3("CDS table section %d/%d", thissec, lastsec);
            lastsec++;
            if (NULL == cds->revdesc.parts)
            cds->revdesc.parts = (char *)calloc(lastsec,sizeof(*cds->revdesc.parts));
            cds->revdesc.parts[thissec]=1;
            for (i=0; i<lastsec && cds->revdesc.parts[i]; i++) {;} // check for unseen parts
            if (i == lastsec)
                return true;
            else
                return false;
        } else if (d_tag == 0x80) {
            TRACE_L2("ignoring CDS stuffing descriptor");
        } else {
            TRACE_L2("Unknown CDS descriptor/len 0x%x %d", d_tag, d_len);
        }
        n+=d_len;
    }
    // if no revision descriptor, then we must be done
    return true;
}

bool DsgParser::parse_mms(unsigned char *buf, int len, struct mms_table *mms)
{
    int first_index = buf[4];
    int recs = buf[5];
    int i, n;

    TRACE_L3("first_index = %d, %d recs", first_index, recs);
    n=7;

    for (i=0; i< recs; i++) {
        int p = first_index + i;

        if (p > 255) TRACE_L2("MMS too big; noncompliant datastream?");
        mms->mm[p].modulation_fmt = scte_modfmt_table[buf[n+1] & 0x1f];
        mms->written[p] = 1;
        TRACE_L3("MMS index %d = %s", p, mms->mm[p].modulation_fmt);
        n +=6;
        // skip over any descriptors
        // should only be stuffing and revision here anyway
        n+=(buf[n]+1);
    }

    // parse NIT descriptors
    while (n < (len - 4 + 3)) { // 4 bytes CRC, 3 bytes header
        unsigned char d_tag = buf[n++];
        unsigned char d_len = buf[n++];

        if (d_tag == 0x93 && d_len >= 3) {
            unsigned char thissec = buf[n+1];
            unsigned char lastsec = buf[n+2];
            int i;

            mms->revdesc.valid=1;
            mms->revdesc.version=buf[n] & 0x1f;
            TRACE_L3("MMS table version %d", mms->revdesc.version);
            TRACE_L3("MMS table section %d/%d", thissec, lastsec);
            lastsec++;
            if (NULL == mms->revdesc.parts)
            mms->revdesc.parts = (char *) calloc(lastsec,sizeof(*mms->revdesc.parts));
            mms->revdesc.parts[thissec]=1;
            for (i=0; i<lastsec && mms->revdesc.parts[i]; i++) {;} // check for unseen parts
            if (i == lastsec)
                return true;
            else
                return false;
        } else if (d_tag == 0x80) {
            TRACE_L2("ignoring MMS stuffing descriptor");
        } else {
            TRACE_L2("Unknown MMS descriptor/len 0x%x %d", d_tag, d_len);
        }
        n+=d_len;
    }
    // if no revision descriptor, then we must be done
    return true;
}


bool DsgParser::parse_ntt(unsigned char *buf, int len, struct ntt_table *ntt)
{
    char table_subtype = buf[7] & 0xf;
    char recs;
    char tmpstr[257];
    int i, n;

    for (i=0; i<3; i++)
    tmpstr[i] = buf[i+4];
    tmpstr[i] = '\0';
    TRACE_L3("ISO_639_language_code = %s", tmpstr);

    if (table_subtype != 6) {
        TRACE_L2("Invalid NTT table_subtype: got %d, expect 6", table_subtype);
        return false;
    }

    TRACE_L1("[%3.3f] NTT", ELAPSED_TIME());

    // begin SNS
    n=8;
    recs = buf[n++];
    for (i=0; i< recs; i++) {
        struct sns_record *sn = (sns_record *)calloc(1, sizeof(*sn));

        sn->app_type = buf[n++] & 0x80;
        sn->id = (buf[n] << 8) | buf[n+1];
        n+=2;
        sn->namelen = buf[n++];
        sn->mode = buf[n++];
        sn->length = buf[n++];

        for (int i=0; i<sn->length;i++)
                sn->segment[i]= buf[n+i];

        sn->segment[sn->length]= '\0';
            std::string sourceName(sn->segment, strlen(sn->segment));
            TRACE_L4("new %s_ID: 0x%04x=%s", sn->app_type ? "App" : "Src", sn->id, sourceName.c_str());

        n+=sn->length;
        sn->descriptor_count = buf[n++];
        memcpy(sn->descriptors, &buf[n], sn->descriptor_count);
        n+=sn->descriptor_count;
        //push to head of list
        sn->next = ntt->sns_list;
        ntt->sns_list = sn;
    }

    // parse NTT descriptors
    while (n < (len - 4 + 3)) { // 4 bytes CRC, 3 bytes header
        unsigned char d_tag = buf[n++];
        unsigned char d_len = buf[n++];

        if (d_tag == 0x93 && d_len >= 3) {
            unsigned char thissec = buf[n+1];
            unsigned char lastsec = buf[n+2];
            int i;

            ntt->revdesc.valid = 1;
            ntt->revdesc.version = buf[n] & 0x1f;
            TRACE_L3("NTT table version %d", ntt->revdesc.version);
            TRACE_L3("NTT table section %d/%d", thissec, lastsec);
            lastsec++;
            if (NULL == ntt->revdesc.parts)
            ntt->revdesc.parts = (char *)calloc(lastsec,sizeof(*ntt->revdesc.parts));
            ntt->revdesc.parts[thissec]=1;
            for (i=0; i<lastsec && ntt->revdesc.parts[i]; i++) {;} // check for unseen parts
            if (i == lastsec)
                return true;
            else
                return false;
        } else if (d_tag == 0x80) {
            TRACE_L2("gnoring NTT stuffing descriptor");
        } else {
            TRACE_L2("Unknown NTT descriptor/len 0x%x %d", d_tag, d_len);
        }
        n+=d_len;
    }
    // if no revision descriptor, then we must be done
    return true;
}


bool DsgParser::parse_svct(unsigned char *buf, int len, struct vcm **vcmlist, int vctidfilter, int &vct_lookup_index)
{
    unsigned char descriptors_included, splice, vc_recs;
    unsigned int activation_time, vctid;
    struct vcm *vcm = *vcmlist;
    int i, n=4;

    enum svct_table_subtype {
        SCTE_SVCT_VCM = 0x00,
        SCTE_SVCT_DCM = 0x01,
        SCTE_SVCT_ICM = 0x02
    } table_subtype;

    table_subtype = (svct_table_subtype) (buf[n++] & 0xf);
    vctid = (buf[n] << 8) | buf[n+1];
    n+=2;

    TRACE(Trace::Information, ("[%3.3f] VCT_ID Received from Stream : %d from Application : %d (table_subtype=%d)",
        ELAPSED_TIME(), vctid, vctidfilter, table_subtype));

    if (table_subtype > 2)
        TRACE_L2("Invalid SVCT subtype %d", table_subtype);

    TRACE_L3("*** vct_lookup_index value : %d", vct_lookup_index);
    if(table_subtype == SCTE_SVCT_VCM && vct_lookup_index == -1)
    {
        //store the first vctid onto vct_lookup_index to monitor the vctid sequence [Eg: Vctid Seq : 3 5 1005 3 5 1005 3 5 1005 ...]
        vct_lookup_index = vctid;
        TRACE_L3("*** vct_lookup_index assigned with vctid value : %d",vct_lookup_index);
    }
    else if (table_subtype == SCTE_SVCT_VCM && vct_lookup_index == vctid)
    {
        //it does indicate that vctidfilter value not found in available vctid sequence before next sequence repeats [Vctid Seq : 3 5 1005 3 ]
        vctidfilter = vct_lookup_index;
        TRACE_L2("*** vctid not found in svct, take the first vctid from the list : %d", vctidfilter);
    }

    // filter on VCMs and, if requested, matching VCT_IDs
    if (table_subtype != SCTE_SVCT_VCM || (vctidfilter != -1 && vctidfilter!=vctid))
    {
        TRACE_L2("table_subtype is not VCM : %d", table_subtype);
        return false;
    }

    TRACE_L1("[%3.3f] SVCT", ELAPSED_TIME());
    // walk the list for matching vctid
    while (vcm && vcm->vctid != vctid) {
        vcm = vcm->next;
    }
    // add new vct/vcm to head if not found
    if (NULL == vcm) {

        TRACE_L2("add new vct/vcm to head if not found ");
        struct vcm *new_vcm = (struct vcm *)calloc(1, sizeof(*new_vcm));

        new_vcm->next = *vcmlist;
        new_vcm->vctid = vctid;
        *vcmlist = vcm = new_vcm;
        TRACE_L2("adding to list new VCT_ID=0x%04x", vctid);
    }

    // now process the rest of the VCT we got from the demod (VCM starts here)
    descriptors_included = buf[n++] & 0x20;
    splice = buf[n++] & 0x80;
    activation_time = (buf[n]<<24) | (buf[n+1]<<16) | (buf[n+2]<<8) | buf[n+3];
    n +=4;
    vc_recs = buf[n++];
    TRACE_L4("VCT_ID =0x%04x, descriptors_incl=%c, splice=%c, activation_time=%d, recs=%d",
        vctid, descriptors_included ? 'Y' : 'N', splice ? 'Y' : 'N', activation_time, vc_recs);
    for (i=0; i< vc_recs; i++) {
        struct vc_record demodvc_rec, *vc_rec;

        n+=read_vc(&buf[n], &demodvc_rec, descriptors_included);

        // done reading record, now search for vc in vcm's vc list
        vc_rec = vcm->vc_list;
        while (vc_rec && demodvc_rec.vc != vc_rec->vc)
        vc_rec = vc_rec->next;

        // if not found, add it to head; otherwise, do nothing
        if (NULL == vc_rec) {
            struct vc_record *newvc_rec = (vc_record *)calloc(1, sizeof(*newvc_rec));

            memcpy(newvc_rec, &demodvc_rec, sizeof(demodvc_rec));
            newvc_rec->next = vcm->vc_list;
            vcm->vc_list = newvc_rec;
            TRACE_L2("adding new rec");
        }
    }

    // parse S-VCT descriptors
    while (n < (len - 4 + 3)) { // 4 bytes CRC, 3 bytes header
        unsigned char d_tag = buf[n++];
        unsigned char d_len = buf[n++];

        if (d_tag == 0x93 && d_len >= 3) {
            unsigned char thissec = buf[n+1];
            unsigned char lastsec = buf[n+2];
            int i;

            vcm->revdesc.valid = 1;
            vcm->revdesc.version = buf[n] & 0x1f;
            TRACE_L3("VCM VCTID 0x%04x table version %d, section %d/%d", vctid, vcm->revdesc.version, thissec, lastsec);
            lastsec++;
            if (NULL == vcm->revdesc.parts)
            vcm->revdesc.parts = (char *)calloc(lastsec,sizeof(*vcm->revdesc.parts));
            vcm->revdesc.parts[thissec]=1;
            for (i=0; i<lastsec && vcm->revdesc.parts[i]; i++) {;} // check for unseen parts
            if (i == lastsec)
            {
                TRACE_L3("*** parse_svct ::  lastsec is true ->  i value : %d, lastsec value  :%d ", i, lastsec);
                return true;
            }
            else
            {
                vct_lookup_index = vctid;
                TRACE_L3("*** parse_svct :: lastsec is false ->  i value : %d, lastsec value  :%d, vct_lookup_index : %d ", i, lastsec, vct_lookup_index);
                return false;
            }
        } else if (d_tag == 0x80) {
            TRACE_L2("ignoring S-VCT stuffing descriptor");
        } else {
            TRACE_L2("Unknown S-VCT descriptor/len 0x%x %d", d_tag, d_len);
        }
        n+=d_len;
    }

    return true;
}

string DsgParser::output_txt(struct cds_table *cds, struct mms_table *mms, struct ntt_table *ntt, struct vcm *vcm_list)
{
    string strChannelMap;
    Core::JSON::ArrayType<Channel> channelMap;

    struct vcm *vcm = vcm_list;
    for (vcm = vcm_list; vcm != NULL; vcm=vcm->next) {
        struct vc_record *vc_rec;
        vcm->vc_list = reverse_vc_list(vcm->vc_list);

        int callNumber = 1;
        for (vc_rec = vcm->vc_list; vc_rec != NULL; vc_rec=vc_rec->next) {
            Channel channel;
            channel.ChannelNumber = (uint16_t) vc_rec->vc;
            channel.CallNumber = (uint16_t) callNumber;

            int cdsValue = 0;
            if (cds->written[vc_rec->cds_ref])
                cdsValue = cds->cd[vc_rec->cds_ref]/1000000;
            channel.Freq = (uint16_t) cdsValue;

            channel.ProgramNumber = (uint16_t) vc_rec->prognum;

            stringstream chusId;
            //for (int i=0; i<vc_rec->desc_cnt; i++)
            //      chusId << std::hex << vc_rec->chusId[i];
            channel.ChuId = chusId.str();

            const char* mm = "";
            if (mms->written[vc_rec->mms_ref])
                mm = mms->mm[vc_rec->mms_ref].modulation_fmt;

            string modulationMode = std::string(mm);
            if(modulationMode.compare("QAM_64") == 0)
                 channel.Modulation = 8;
            else if(modulationMode.compare("QAM_256") == 0)
                channel.Modulation = 16;

            channel.SourceId = (uint32_t) vc_rec->id;

            // Search NTT for Source ID match and Retrieve Source Name
            std::string sourceName = "Test Channel";
            struct sns_record *sn_rec = ntt->sns_list;
            while (sn_rec && sn_rec->id != vc_rec->id && !vc_rec->application)
              sn_rec=sn_rec->next;
            if (sn_rec)
                sourceName = string(sn_rec->segment);

            channel.Description = sourceName;

            //  0: vcn 0000;  source_id 0x0002;  name ;  descriptors 0; freq 111000000Hz;  mod 16;  pn 128;
            //TRACE_L1("%d: vcn %d;  source_id %04x;  name %s;  freq %d;  mod %d; pn %d;", callNumber, vc_rec->vc, vc_rec->id, sourceName.c_str(), cdsValue, 16, vc_rec->prognum);

            channelMap.Add(channel);

            callNumber++;
        } // for
    }

    TRACE_L1("channelMap.Length=%d", channelMap.Length());
    channelMap.ToString(strChannelMap);
    TRACE_L1("strChannelMap.size=%d", strChannelMap.size());
    return strChannelMap;
}

// vc list formed backwards, so reverse order
struct vc_record* DsgParser::reverse_vc_list(struct vc_record *vc_rec) {
    struct vc_record *vc_new=NULL;

    while (vc_rec) {
        struct vc_record *vc_tmp = vc_rec;
        vc_rec = vc_rec->next; // pop top from old
        vc_tmp->next = vc_new;// push it on to new
        vc_new=vc_tmp;
    }
    return vc_new;
}

// fills vc_rec from buf, returns number of bytes processed
int DsgParser::read_vc(unsigned char *buf, struct vc_record *vc_rec, unsigned char desc_inc) {
    int i,n;
    vc_rec->vc = ((buf[0] & 0xf) << 8) | buf[1];
    vc_rec->application = buf[2] & 0x80;
    vc_rec->path_select = buf[2] & 0x20;
    vc_rec->transport_type = buf[2] & 0x10;
    vc_rec->channel_type = buf[2] & 0xf;
    vc_rec->id = (buf[3] << 8) | buf[4];

    TRACE_L3("vc %4d, %s_ID: 0x%04x, %s", vc_rec->vc, vc_rec->application ? "app" : "src", vc_rec->id, vc_rec->channel_type ? "hidden/reserved" : "normal");
    n=5;
    if (vc_rec->transport_type == 0) {
        vc_rec->cds_ref = buf[n++];
        vc_rec->prognum = (buf[n] << 8) | buf[n+1];
        n+=2;
        vc_rec->mms_ref = buf[n++];
        TRACE_L3("CDS_ref=%d, program=%d, MMS_ref=%d", vc_rec->cds_ref, vc_rec->prognum, vc_rec->mms_ref);
    } else {
        vc_rec->cds_ref = buf[n++];
        vc_rec->scrambled = buf[n] & 0x80;
        vc_rec->video_std = buf[n++] & 0xf;
        n+=2; // 2 bytes of zero
        TRACE_L3("CDS_ref=%d, scrambled=%c, vid_std=%d", vc_rec->cds_ref, vc_rec->scrambled ? 'Y' : 'N', vc_rec->video_std);
    }

    if (desc_inc) {
        vc_rec->desc_cnt = buf[n++];
        for (i=0; i<vc_rec->desc_cnt; i++) {
            vc_rec->descTag[i] = buf[n++];
            vc_rec->descLength = buf[n++];
            vc_rec->chusId[i] = (buf[n] << 24)| (buf[n+1] << 16)| (buf[n+2] << 8) | buf[n+3]; //add logic dynamically retrieve the descriptor values
            TRACE_L3("descriptor %d: descTag=0x%02x, descLength=%d, chusId=0x%04x", i, vc_rec->descTag[i], vc_rec->descLength, vc_rec->chusId[i]);
            n += vc_rec->descLength;
        }
    }

    return n;
}


void DsgParser::HexDump(const char* label, const std::string& msg, uint16_t charsPerLine)
{
    #if _TRACE_LEVEL >= 4
    std::stringstream ssHex, ss;
    for (uint16_t i = 0; i < msg.length(); i++) {
        int byte = (uint8_t)msg.at(i);
        ssHex << std::setfill('0') << std::setw(2) << std::hex <<  byte << " ";
        ss << char((byte < ' ' || byte > 127) ? '.' : byte);

        if (!((i+1) % charsPerLine)) {
            TRACE_L4("%s: %s %s", label, ssHex.str().c_str(), ss.str().c_str());
            ss.str(std::string());
            ssHex.str(std::string());
        }
    }
    TRACE_L4("%s: %s %s", label, ssHex.str().c_str(), ss.str().c_str());
    #endif
}

//} // DSG
} // namespace Plugin
} // namespace WPEFramework
