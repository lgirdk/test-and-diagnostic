#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

source /etc/utopia/service.d/log_env_var.sh
source /etc/utopia/service.d/log_capture_path.sh
source /lib/rdk/t2Shared_api.sh
source /etc/device.properties
# RDKB-6628 : Periodically log whether SSIDs are same or not
ssid24value=""
ssid5value=""
got_24=0
got_5=0
if [ "x$BOX_TYPE" = "xXB3" ]; then
GET_PID_FROM_PEER=`rpcclient2 "busybox pidof CcspWifiSsp"`
WiFi_PID=`echo "$GET_PID_FROM_PEER" | awk 'END{print}' | grep -v "RPC CONNECTED"`
else
   if [ "$OneWiFiEnabled" == "true" ]; then
          WiFi_PID=$(busybox pidof OneWifi)
      else
          WiFi_PID=$(busybox pidof CcspWifiSsp)
  fi
fi
if [ "x$WiFi_PID" != "x" ]; then
    # check for wifi params only if wifi agent is up and running
    # Get 2.4GHz SSID and do sanity check
    getSsid24=`dmcli eRT getv Device.WiFi.SSID.1.SSID` 
    getSsid5=`dmcli eRT getv Device.WiFi.SSID.2.SSID`
    SSID_24=`echo $getSsid24 | grep "Execution succeed"`
    if [ "$SSID_24" == "" ]
    then
        echo "`date +'%Y-%m-%d:%H:%M:%S:%6N'` [RDKB_PLATFORM_ERROR] Didn't get WiFi 2.4 GHz SSID from agent"
        t2CountNotify "WIFI_ERROR_atomConsoleDown_2G" 
    else
        ssid24value=`echo $getSsid24 | cut -f6 -d:`
        got_24=1
    fi
    
    # Get 5GHz SSID and do sanity check
    SSID_5=`echo $getSsid5 | grep "Execution succeed"`
    if [ "$SSID_5" == "" ]
    then
        echo "`date +'%Y-%m-%d:%H:%M:%S:%6N'` [RDKB_PLATFORM_ERROR] Didn't get WiFi 5 GHz SSID from agent"
        t2CountNotify "WIFI_ERROR_atomConsoleDown_5G"
    else
        ssid5value=`echo $getSsid5 | cut -f6 -d:`
        got_5=1
    fi
    
    # Compare 2.4GHz and 5GHz SSID and log
    if [ $got_24 -eq 1 ] && [ $got_5 -eq 1 ]
    then
         if [ "$ssid5value" == "$ssid24value" ]
         then
            echo "`date +'%Y-%m-%d:%H:%M:%S:%6N'` [RDKB_STAT_LOG] 2.4G and 5G SSIDs are same"
    	t2CountNotify "SYS_INFO_sameSSID"
         else
            echo "`date +'%Y-%m-%d:%H:%M:%S:%6N'` [RDKB_STAT_LOG] 2.4G and 5G SSIDs are different"
    	t2CountNotify "SYS_INFO_differentSSID"
         fi
         got_24=0
         got_5=0 
    fi
else
    echo "`date +'%Y-%m-%d:%H:%M:%S:%6N'` [RDKB_PLATFORM_ERROR] WiFi Agent is not running skipping getSsidNames"
    t2CountNotify "WIFI_INFO_skipSSID"
fi
