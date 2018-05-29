cmake_minimum_required(VERSION 2.8)

option(PLUGIN_TVCONTROL_LINUXTV "Include LinuxTV TVControl" OFF)

if(PLUGIN_TVCONTROL_LINUXTV)
    add_subdirectory (LinuxTV)
endif(PLUGIN_TVCONTROL_LINUXTV)
