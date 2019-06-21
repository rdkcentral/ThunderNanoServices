cmake_minimum_required(VERSION 3.3)

option(PLUGIN_JSONRPC "Include JSONRPCExamplePlugin plugin" OFF)
option(PLUGIN_TESTUTILITY "Include TestUtility plugin" OFF)
option(PLUGIN_TESTCONTROLLER "Include TestController plugin" OFF)

if(PLUGIN_TESTUTILITY)
    add_subdirectory(TestUtility)
endif()

if(PLUGIN_TESTCONTROLLER)
    add_subdirectory(TestController)
endif()

if(PLUGIN_JSONRPC)
    add_subdirectory(JSONRPCPlugin)
    add_subdirectory(JSONRPCClient)
endif()
