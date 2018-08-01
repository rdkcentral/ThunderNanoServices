#include "DSResolution.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DSResolution, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DSResolution::Config>> jsonBodyDataFactory(1);

    /* virtual */ const string DSResolution::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_controller.IsValid() == false);

        string result;

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _service = service;

        _controller = DSResolutionHAL::Create();
        if ((_controller.IsValid() == true) && (!(_controller->IsOperational()))) {
            _controller.Release();
            result = _T("Not Feasible to change the Device Settings"); 
        }
        
        if((service != nullptr) && ( _controller->IsOperational())){
            TRACE(Trace::Information, (_T("Successfully instantiated DSResolution Plug-In")));
            result = _T("") ;
        }
        else {
            if(_controller->IsOperational()) {
                TRACE(Trace::Error, (_T("Failed to initialize DSResolution Plug-In")));          
                result =  _T("No Service.");
             }
        }
        return result;
    }

    /* virtual */ void DSResolution::Deinitialize(PluginHost::IShell* service)
    {
         ASSERT(_service == service);
        _service = nullptr;
    }

    /* virtual */ string DSResolution::Information() const
    {
        //No additional info to report
        return (string());
    }
   
    /* virtual */ void DSResolution::Inbound(Web::Request& request)
    {
         if (request.Verb == Web::Request::HTTP_POST)
             request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> DSResolution::Process(const Web::Request& request)
    {
        TRACE(Trace::Information, (string(_T("Process"))));
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();
        if (request.Verb == Web::Request::HTTP_GET)
            result = GetMethod(index);
        else if (request.Verb == Web::Request::HTTP_POST)
            result = PostMethod(index, request);

        return result;
    }
   
    Core::ProxyType<Web::Response> DSResolution::GetMethod(Core::TextSegmentIterator& index)
    {
        TRACE(Trace::Information, (string(_T("GetMethod"))));
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<DSResolution::Config>> response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("GetDSResolution")) {
                    TRACE(Trace::Information, (string(_T("GetDSResolution"))));
                    uint16_t resolution = _controller->GetDSResolution();
                    response->DSResolution = resolution;
                    TRACE(Trace::Information, (_T("DSResolution : %d"), resolution));
                    if(resolution <= dsVIDEO_PIXELRES_MAX) {
                        result->ContentType = Web::MIMETypes::MIME_JSON;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Device Setting Resolution");
                        TRACE(Trace::Information, (string(_T("Successfully get the Display Resolution"))));
                    } else
                        TRACE(Trace::Error, (string(_T("Failed to get the Display Resolution"))));
                }
                else
                    TRACE(Trace::Information, (string(_T("Not a valid GET method"))));
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> DSResolution::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        TRACE(Trace::Information, (string(_T("PostMethod"))));
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<DSResolution::Config>> response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("SetDSResolution") && (request.HasBody())) {
                    uint16_t DSResolution = request.Body<const Config>()->DSResolution.Value();
                    bool status;
                    DSResolution--;
                    TRACE(Trace::Information, (_T("DSResolution : %d"), DSResolution));
                    status = _controller->SetDSResolution(DSResolution);
                    if(status) {
                        result->ContentType = Web::MIMETypes::MIME_JSON;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Set Device Setting Resolution");
                        TRACE(Trace::Information, (string(_T("Successfully set the Display Resolution"))));
                    } else 
                        TRACE(Trace::Error, (string(_T("Failed to set the Display Resolution"))));
                }
            }
        }
        return result;
    }       
} //Plugin
} //WPEFramework
