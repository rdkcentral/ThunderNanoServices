include(list_to_string)
# Converts a CMake list to a CMake string and displays this in a status message along with the text specified.
function(display_list text)
    set(list_str "")
    foreach(item ${ARGN})
        string(CONCAT list_str "${list_str} ${item}")
    endforeach()
    message(STATUS ${text} ${list_str})
endfunction()
