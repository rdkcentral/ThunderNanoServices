#ifndef __COMMON_H__
#define __COMMON_H__

#include <core/core.h>

namespace WPEFramework {

class Channel : public Core::JSON::Container {
private:
    Channel& operator=(const Channel&);

public:
    Channel()
        : Core::JSON::Container()
    {
        Add(_T("number"), &number);
        Add(_T("networkId"), &networkId);
        Add(_T("transportSId"), &transportSId);
        Add(_T("serviceId"), &serviceId);
        Add(_T("name"), &name);
        Add(_T("frequency"), &frequency);
        Add(_T("programNo"), &programNo);
        Add(_T("type"), &type);
    }
    Channel(const Channel& copy)
        : Core::JSON::Container()
        , number(copy.number)
        , networkId(copy.networkId)
        , transportSId(copy.transportSId)
        , serviceId(copy.serviceId)
        , name(copy.name)
        , frequency(copy.frequency)
        , programNo(copy.programNo)
        , type(copy.type)
    {
        Add(_T("number"), &number);
        Add(_T("networkId"), &networkId);
        Add(_T("transportSId"), &transportSId);
        Add(_T("serviceId"), &serviceId);
        Add(_T("name"), &name);
        Add(_T("frequency"), &frequency);
        Add(_T("programNo"), &programNo);
        Add(_T("type"), &type);
    }
    ~Channel()
    {
    }

public:
    Core::JSON::String number;
    Core::JSON::DecUInt16 networkId;
    Core::JSON::DecUInt16 transportSId;
    Core::JSON::DecUInt16 serviceId;
    Core::JSON::String name;
    Core::JSON::DecUInt32 frequency;
    Core::JSON::DecUInt16 programNo;
    Core::JSON::String type;
};

class Program : public Core::JSON::Container {
private:
    Program& operator=(const Program&);

public:
    Program()
        : Core::JSON::Container()
    {
        Add(_T("eventId"), &eventId);
        Add(_T("title"), &title);
        Add(_T("startTime"), &startTime);
        Add(_T("duration"), &duration);
        Add(_T("shortDescription"), &shortDescription);
        Add(_T("longDescription"), &longDescription);
        Add(_T("rating"), &rating);
        Add(_T("serviceId"), &serviceId);
        Add(_T("audio"), &audio);
        Add(_T("subtitle"), &subtitle);
        Add(_T("genre"), &genre);
    }
    Program(const Program& copy)
        : Core::JSON::Container()
        , eventId(copy.eventId)
        , title(copy.title)
        , startTime(copy.startTime)
        , duration(copy.duration)
        , shortDescription(copy.shortDescription)
        , longDescription(copy.longDescription)
        , rating(copy.rating)
        , serviceId(copy.serviceId)
        , audio(copy.audio)
        , subtitle(copy.subtitle)
        , genre(copy.genre)
    {
        Add(_T("eventId"), &eventId);
        Add(_T("title"), &title);
        Add(_T("startTime"), &startTime);
        Add(_T("duration"), &duration);
        Add(_T("shortDescription"), &shortDescription);
        Add(_T("longDescription"), &longDescription);
        Add(_T("rating"), &rating);
        Add(_T("serviceId"), &serviceId);
        Add(_T("audio"), &audio);
        Add(_T("subtitle"), &subtitle);
        Add(_T("genre"), &genre);
    }
    ~Program()
    {
    }

public:
    Core::JSON::DecUInt16 eventId;
    Core::JSON::String title;
    Core::JSON::DecUInt32 startTime;
    Core::JSON::DecUInt32 duration;
    Core::JSON::String shortDescription;
    Core::JSON::String longDescription;
    Core::JSON::String rating;
    Core::JSON::DecUInt16 serviceId;
    Core::JSON::String audio;
    Core::JSON::String subtitle;
    Core::JSON::String genre;
};
}
#endif
