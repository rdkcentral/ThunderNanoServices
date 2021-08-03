#ResourceMonitor
Resource Monitor Plugin

Resource Monitor gathers memory and cpu usage of given process(s).

USS is the total private memory for a process.
PSS is the proportional size of its shared libraries.
RSS is the total memory actually held in RAM for a process.
VSS is the total accessible address space of a process,



By default all processes contain 'WPE' in the process name or the executable name will be monitored.
    Time[s];Name;USS[KiB];PSS[KiB];RSS[KiB];VSS[KiB];UserTotalCPU[%];SystemTotalCPU[%]
    1627665769;WPEFramework (189);16836;20710;29576;141884;0.6;0.12
    1627665769;WebServer (253);768;3166;10568;31616;0.045;0.037
    1627665769;OCDM (302);3656;7203;15724;50220;0.052;0
    1627665769;ResourceMonitor (312);728;3060;10400;24300;0.059;0.015

You can configure either by specific process name
    "names":["OCDM"]

    Time[s];Name;USS[KiB];PSS[KiB];RSS[KiB];VSS[KiB];UserTotalCPU[%];SystemTotalCPU[%]
    1627666233;OCDM (439);3564;5990;15680;50220;0.0071;0.00051
    1627666238;OCDM (439);3604;7169;15680;50220;0;0
    1627666243;OCDM (439);3604;7165;15680;50220;0;0

Or by the executable name
    "names":["WPEProcess"]

    Time[s];Name;USS[KiB];PSS[KiB];RSS[KiB];VSS[KiB];UserTotalCPU[%];SystemTotalCPU[%]
    1627665891;WebServer (253);768;3162;10568;31616;0;0
    1627665891;OCDM (302);3656;7201;15724;50220;0;0
    1627665891;ResourceMonitor (312);728;3058;10400;24300;0.4;0.05

Since 'names' is a json array, you can configure multiple process/executable name(s)
    "names":["WPEFramework", "OCDM"]

    Time[s];Name;USS[KiB];PSS[KiB];RSS[KiB];VSS[KiB];UserTotalCPU[%];SystemTotalCPU[%]
    1627666722;WPEFramework (501);16904;20747;29516;141876;0.01;0.0015
    1627666722;OCDM (561);3672;7262;15884;50220;0.0023;0.0013

Only the most recent 10 stats will be returned in the response.
    curl -GET "http://rpi/Service/ResourceMonitor/history"

Full log is under /tmp/resource.csv. You can get the full log via secure copy.
    scp root@<IP-ADDRESS>:/tmp/resource.csv .


