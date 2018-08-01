#include "dsVideoPort.h"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <vector>

uint16_t _pixelResolution = dsVIDEO_PIXELRES_MAX;

struct DummyConfig
{
    std::string func;
    std::string value; 
};

std::vector<DummyConfig> configuraton;

void initConfig()
{
    DummyConfig dummyConfig;
    std::string config;
    char* token;
    std::ifstream configFile("/usr/HALConfig");
    if (configFile.is_open()){
        while(!configFile.eof())
        {
            getline(configFile, config);
            char *conf = new char[config.length() + 1];
            strcpy(conf, config.c_str());
            token = strtok (conf,"=");
            int func=1;
            while (token!= NULL)
            {
                if(func) {
                    dummyConfig.func = token;
                    func =0;
                }
                else {
                    dummyConfig.value = token;
                }
                token = strtok (NULL, "=");
            }
            configuraton.push_back(dummyConfig);
            delete [] conf;
        }
    }
    configFile.close();
}
dsError_t  dsHostInit()
{
    initConfig();    
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    for( int i=0;i<configuraton.size();i++)
        if("HOST_INIT" == configuraton[i].func)
           if("true" == configuraton[i].value)
               return dsERR_NONE;
           else
               return dsERR_GENERAL;
}

dsError_t  dsVideoPortInit()
{
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    for( int i=0;i<configuraton.size();i++)
        if("VIDEOPORT_INIT" == configuraton[i].func)
           if("true" == configuraton[i].value)
               return dsERR_NONE;
           else
               return dsERR_GENERAL;
}

dsError_t  dsGetVideoPort(dsVideoPortType_t type, int index, int *handle)
{
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    for( int i=0;i<configuraton.size();i++)
        if("GETVIDEOPORT" == configuraton[i].func)
           if("true" == configuraton[i].value)
               return dsERR_NONE;
           else
               return dsERR_GENERAL;
}

dsError_t  dsIsVideoPortEnabled(int handle, bool *enabled)
{
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    *enabled = true;
   for( int i=0;i<configuraton.size();i++)
        if("VIDEOPORT_ENABLED" == configuraton[i].func)
           if("true" == configuraton[i].value)
               return dsERR_NONE;
           else
               return dsERR_GENERAL; 
}

dsError_t dsIsDisplayConnected(int handle, bool *connected)
{
    *connected = true;
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    for( int i=0;i<configuraton.size();i++)
        if("ISDISPLAY_CONNECTED" == configuraton[i].func)
           if("true" == configuraton[i].value)
               return dsERR_NONE;
           else
               return dsERR_GENERAL;
}

dsError_t  dsGetResolution(int handle, dsVideoPortResolution_t *resolution)
{
    dsError_t dsRet = dsERR_GENERAL;
    printf("%s:%s:%d\n",__FILE__,__func__,__LINE__);
    for( int i=0;i<configuraton.size();i++) {
        if("GET_RESOLUTION" == configuraton[i].func) {
            if("true" == configuraton[i].value) {
                printf("PixelResolution is :");
                switch(_pixelResolution)
                {
                    case 0:
                        resolution->pixelResolution = (dsVideoResolution_t)1;
                        printf("dsVIDEO_PIXELRES_720x480");
                        break;
                    case 1:
                        resolution->pixelResolution = (dsVideoResolution_t)2;
                        printf("dsVIDEO_PIXELRES_720x576");
                        break;
                    case 2:
                        resolution->pixelResolution = (dsVideoResolution_t)3;
                        printf("dsVIDEO_PIXELRES_1280x720");
                        break;
                    case 3:
                        resolution->pixelResolution = (dsVideoResolution_t)4;
                        printf("dsVIDEO_PIXELRES_1920x1080");
                        break;
                    case 4:
                        resolution->pixelResolution =(dsVideoResolution_t)5;
                        printf("dsVIDEO_PIXELRES_3840x2160");
                        break;
                    case 5:
                        resolution->pixelResolution = (dsVideoResolution_t)6;
                        printf("dsVIDEO_PIXELRES_4096x2160");
                        break;
                    default:
                        printf("Deafult");
                }
                printf("\n");
                dsRet = dsERR_NONE;
            }
        }
    }
    return dsRet;
}

dsError_t  dsSetResolution(int handle, dsVideoPortResolution_t *resolution)
{
    dsError_t dsRet = dsERR_GENERAL;
    printf("%s:%s:%d Resolution : [%d]\n",__FILE__,__func__,__LINE__,(uint16_t)resolution->pixelResolution);
    for( int i=0;i<configuraton.size();i++) {
        if("SET_RESOLUTION" == configuraton[i].func) {
            if("true" == configuraton[i].value) {
                _pixelResolution = (uint16_t)resolution->pixelResolution;
                printf("PixelResolution is :");
                switch((uint16_t)resolution->pixelResolution)
                {
                    case 0:
                        printf("dsVIDEO_PIXELRES_720x480");
                        break;
                    case 1:
                        printf("dsVIDEO_PIXELRES_720x576");
                        break;
                    case 2:
                        printf("dsVIDEO_PIXELRES_1280x720");
                        break;
                    case 3:
                        printf("dsVIDEO_PIXELRES_1920x1080");
                        break;
                    case 4:
                        printf("dsVIDEO_PIXELRES_3840x2160");
                        break;
                    case 5:
                        printf("dsVIDEO_PIXELRES_4096x2160");
                        break;
                    default:
                        printf("Deafult");
                }
                printf("\n");
                dsRet =   dsERR_NONE;
            }
         }
     }
     return dsRet;
}
