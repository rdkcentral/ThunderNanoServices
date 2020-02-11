DEBUG: NOT RECOMMENDED WAY OF WORKING. LEAVE THE STARTING OF WPA_SUPPLICANT UP TO THE PLUGIN !!!!

=== attach to running instance of wpa_supplicant ===

If you want to start WifiControl on a system where wpa_supplicant is already running:
1) execute "iwconfig" and find the wifi interface name in use 
2) run "netstat -anp | grep wpa" to find the connector used (e.g. something similar to /run/wpa_supplicant/wlp3s0 will be returned where "/run/wpa_supplicant/" is the connector part and the "wlp3s0" part you should recognize as the interface found in the step 1.
3) add a configuration section to the WifiControl plugin similar to this:

"configuration":{
    "application":"null",
    "connector":"/run/wpa_supplicant/",
    "interface":"wlp3s0"
 }
 
application:null makes sure wpa_supplicant is not started but connected to, and obviously replace the the connector and   interface values in the example above with the values found in step 1 and 2

Note if an ls of the connector folder (so for example "ls -la /run/wpa_supplicant") results in a "permision denied" message then you must be running as root to be able to connect.

=== restart wpa_supplicant  ===

If you need to restart wpa_supplicant:

execute "sudo systemctl restart wpa_supplicant.service"
