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
 
#include "dsVideoPort.h"

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

#define CONFIG_LOCATION "/usr/local/etc/WPEFramework"
#define HAL_CONFIG_FILE "rdk_devicesettings_hal.conf"

struct DummyConfig {
    std::string func;
    std::string value;
};
uint16_t _pixelResolution = dsVIDEO_PIXELRES_MAX;

std::vector<DummyConfig> configuraton;

bool initConfig()
{
    DummyConfig dummyConfig;
    std::string config;
    char* token;
    char file[70];
    sprintf(file, "%s/%s", CONFIG_LOCATION, HAL_CONFIG_FILE);
    std::ifstream configFile(file);
    if (configFile.is_open()) {
        while (!configFile.eof()) {
            getline(configFile, config);
            char* conf = new char[config.length() + 1];
            strcpy(conf, config.c_str());
            token = strtok(conf, "=");
            int func = 1;
            while (token != NULL) {
                if (func) {
                    dummyConfig.func = token;
                    func = 0;
                } else {
                    dummyConfig.value = token;
                }
                token = strtok(NULL, "=");
            }
            configuraton.push_back(dummyConfig);
            delete[] conf;
        }
    } else
        return false;
    configFile.close();
    return true;
}

dsError_t dsHostInit()
{
    if (initConfig()) {
        for (int i = 0; i < configuraton.size(); i++)
            if ("HOST_INIT" == configuraton[i].func)
                if ("true" == configuraton[i].value)
                    return dsERR_NONE;
                else
                    return dsERR_GENERAL;
    }
    return dsERR_GENERAL;
}

dsError_t dsVideoPortInit()
{
    for (int i = 0; i < configuraton.size(); i++)
        if ("VIDEOPORT_INIT" == configuraton[i].func)
            if ("true" == configuraton[i].value)
                return dsERR_NONE;
            else
                return dsERR_GENERAL;
}

dsError_t dsGetVideoPort(dsVideoPortType_t type, int index, int* handle)
{
    for (int i = 0; i < configuraton.size(); i++)
        if ("GETVIDEOPORT" == configuraton[i].func)
            if ("true" == configuraton[i].value)
                return dsERR_NONE;
            else
                return dsERR_GENERAL;
}

dsError_t dsIsVideoPortEnabled(int handle, bool* enabled)
{
    *enabled = true;
    for (int i = 0; i < configuraton.size(); i++)
        if ("VIDEOPORT_ENABLED" == configuraton[i].func)
            if ("true" == configuraton[i].value)
                return dsERR_NONE;
            else
                return dsERR_GENERAL;
}

dsError_t dsIsDisplayConnected(int handle, bool* connected)
{
    *connected = true;
    for (int i = 0; i < configuraton.size(); i++)
        if ("ISDISPLAY_CONNECTED" == configuraton[i].func)
            if ("true" == configuraton[i].value)
                return dsERR_NONE;
            else
                return dsERR_GENERAL;
}

dsError_t dsGetResolution(int handle, dsVideoPortResolution_t* resolution)
{
    dsError_t dsRet = dsERR_GENERAL;
    for (int i = 0; i < configuraton.size(); i++) {
        if ("GET_RESOLUTION" == configuraton[i].func) {
            if ("true" == configuraton[i].value) {
                resolution->pixelResolution = dsVideoResolution_t(_pixelResolution);
                dsRet = dsERR_NONE;
            }
        }
    }
    return dsRet;
}

dsError_t dsSetResolution(int handle, dsVideoPortResolution_t* resolution)
{
    dsError_t dsRet = dsERR_GENERAL;
    for (int i = 0; i < configuraton.size(); i++) {
        if ("SET_RESOLUTION" == configuraton[i].func) {
            if ("true" == configuraton[i].value) {
                _pixelResolution = resolution->pixelResolution;
                dsRet = dsERR_NONE;
            }
        }
    }
    return dsRet;
}
