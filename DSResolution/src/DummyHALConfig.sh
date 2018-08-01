#!/bin/sh

: <<'MENU'
1.HOST_INIT false
2.HOST_INIT true
3.HOST_INIT true;VIDEOPORT_INIT true
4.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true
5.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true;VIDEOPORT_ENABLED true
6.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true;VIDEOPORT_ENABLED true;ISDISPLAY_CONNECTED true
7.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true;VIDEOPORT_ENABLED true;ISDISPLAY_CONNECTED true;GET_RESOLUTION true
8.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true;VIDEOPORT_ENABLED true;ISDISPLAY_CONNECTED true;SET_RESOLUTION true
9.HOST_INIT true;VIDEOPORT_INIT true;GETVIDEOPORT true;VIDEOPORT_ENABLED true;ISDISPLAY_CONNECTED true;GET_RESOLUTION true;SET_RESOLUTION true
MENU

OPTION=1
TRUE=true
FALSE=false
case "$OPTION" in
"1")
    echo "Option1"
    sed -i -e "s/\(HOST_INIT=\).*/\1$FALSE/" HALConfig 
    ;;
"2")
    echo "Option2"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$FALSE/" HALConfig 
   ;;
"3")
    echo "Option3"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$FALSE/" HALConfig 
   ;;
"4")
    echo "Option4"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$FALSE/" HALConfig
   ;;
"5")
    echo "Option5"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$TRUE/" HALConfig 
    sed -i -e "s/\(ISDISPLAY_CONNECTED=\).*/\1$FALSE/" HALConfig
   ;;
"6")
    echo "Option6"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(ISDISPLAY_CONNECTED=\).*/\1$TRUE/" HALConfig 
    sed -i -e "s/\(GET_RESOLUTION=\).*/\1$FALSE/" HALConfig
    sed -i -e "s/\(SET_RESOLUTION=\).*/\1$FALSE/" HALConfig
   ;;
"7")
    echo "Option7"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(ISDISPLAY_CONNECTED=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GET_RESOLUTION=\).*/\1$TRUE/" HALConfig 
    sed -i -e "s/\(SET_RESOLUTION=\).*/\1$FALSE/" HALConfig
   ;;
"8")
    echo "Option8"
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(ISDISPLAY_CONNECTED=\).*/\1$TRUE/" HALConfig
    sed -i -e "s/\(GET_RESOLUTION=\).*/\1$FALSE/" HALConfig
    sed -i -e "s/\(SET_RESOLUTION=\).*/\1$TRUE/" HALConfig 
   ;;
"9")                                                            
    echo "Option9"                                              
    sed -i -e "s/\(HOST_INIT=\).*/\1$TRUE/" HALConfig           
    sed -i -e "s/\(VIDEOPORT_INIT=\).*/\1$TRUE/" HALConfig      
    sed -i -e "s/\(GETVIDEOPORT=\).*/\1$TRUE/" HALConfig        
    sed -i -e "s/\(VIDEOPORT_ENABLED=\).*/\1$TRUE/" HALConfig   
    sed -i -e "s/\(ISDISPLAY_CONNECTED=\).*/\1$TRUE/" HALConfig 
    sed -i -e "s/\(GET_RESOLUTION=\).*/\1$TRUE/" HALConfig      
    sed -i -e "s/\(SET_RESOLUTION=\).*/\1$TRUE/" HALConfig      
   ;;     
esac
