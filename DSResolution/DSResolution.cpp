/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "DSResolution.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DSResolution, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<DSResolution::Config>> jsonBodyDataFactory(1);

    /* virtual */ const string DSResolution::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string result;
        DSResolution::Config config;
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        if (_controller.IsOperational() == false) {
            result = _T("Not Feasible to change the Device Settings");
        } else if ((config.Resolution.IsSet() == true) && (!_controller.Resolution(config.Resolution.Value()))) {
            TRACE(Trace::Information, (string(_T("Failed to set the configured Display Resolution"))));
        }

        return result;
    }

    /* virtual */ void DSResolution::Deinitialize(PluginHost::IShell* /* service */)
    {
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
        if (request.Verb == Web::Request::HTTP_GET) {
            TRACE(Trace::Information, (string(_T("GET"))));
            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_POST) {
            TRACE(Trace::Information, (string(_T("POST"))));
            result = PostMethod(index, request);
        }
        return result;
    }

    Core::ProxyType<Web::Response> DSResolution::GetMethod(Core::TextSegmentIterator& index)
    {
        TRACE(Trace::Information, (string(_T("GetMethod"))));
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<DSResolution::Config>> response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("Resolution")) {
                    TRACE(Trace::Information, (string(_T("Get Resolution"))));
                    response->Resolution = _controller.Resolution();
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->ErrorCode = Web::STATUS_OK;
                    result->Body(response);
                    result->Message = _T("Get Device Setting Resolution");
                    TRACE(Trace::Information, (string(_T("Successfully get the Display Resolution"))));
                } else
                    TRACE(Trace::Information, (string(_T("Not a valid GET method"))));
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> DSResolution::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        TRACE(Trace::Information, (string(_T("PostMethod"))));
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<DSResolution::Config>> response(jsonBodyDataFactory.Element());
                if (index.Current() == _T("Resolution")) {
                    DSResolutionHAL::PixelResolution format(DSResolutionHAL::PixelResolution_Unknown);
                    if (request.HasBody() == true) {
                        format = request.Body<const Config>()->Resolution.Value();
                        TRACE(Trace::Information, (_T("Resolution is : %d"), format));
                    } else if (index.Next() == true) {
                        format = Core::EnumerateType<DSResolutionHAL::PixelResolution>(index.Current().Data()).Value();
                        TRACE(Trace::Information, (_T("Resolution is : %d"), format));
                    }

                    if (format != DSResolutionHAL::PixelResolution_Unknown) {
                        bool status = true;
                        status = _controller.Resolution(format);
                        if (status) {
                            result->ContentType = Web::MIMETypes::MIME_JSON;
                            result->ErrorCode = Web::STATUS_OK;
                            result->Body(response);
                            result->Message = _T("Set Device Setting Resolution");
                            TRACE(Trace::Information, (string(_T("Successfully set the Display Resolution"))));
                        } else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = _T("Unknown Pixel Resolution: ");
                            TRACE(Trace::Error, (string(_T("Failed to set the Display Resolution"))));
                        }
                    }
                }
            }
        }
        return result;
    }
} //Plugin
} //WPEFramework
