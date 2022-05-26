set(MESSAGE_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/MenuLanguage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/OSDName.cpp
    ${CMAKE_CURRENT_LIST_DIR}/OSDString.cpp
    ${CMAKE_CURRENT_LIST_DIR}/PhysicalAddress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SetStreamPath.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Standby.cpp
    ${CMAKE_CURRENT_LIST_DIR}/VendorID.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Version.cpp
)

target_sources(_cec_message_processor PUBLIC ${MESSAGE_SRCS})