#!/bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
#  Copyright 2018 RDK Management
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
#######################################################################################

#This script is used to enable / start the speedtest tool
LOG_FILE=/rdklogs/logs/speedtest-init.log

echo "$(date +"[%Y-%m-%d %H:%M:%S]" ) Enabling / Starting speedtest..." >> $LOG_FILE
. /etc/device.properties

# C client implementation of the speedtest currently supported for our platforms.
# Node.js client support removed for XB3 ,XB6 and XF3 devices.
  # If Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType exists, then:
  # Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType = 1 implies the C client should run.
ST_CLIENT_TYPE=`dmcli eRT retv Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType`

if [ "x$ST_CLIENT_TYPE" = 'x1' ]; then
    case "$BOX_TYPE" in
        "XB3")
            if [ "$MODEL_NUM" = "TG1682G" ]; then
                # Execute speedtest-client-c for XB3A
                echo "$(date +"[%Y-%m-%d %H:%M:%S]") Executing speedtest-client-c for XB3A" >> "$LOG_FILE"
                rpcclient "$ATOM_ARPING_IP" "/usr/bin/speedtest-client &"
            elif [ "$MODEL_NUM" = "DPC3941" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
                # Execute speedtest-client-c for XB3C
                echo "$(date +"[%Y-%m-%d %H:%M:%S]") Downloading and/or executing speedtest-client-c for XB3C" >> "$LOG_FILE"
                rpcclient "$ATOM_ARPING_IP" "sh /etc/measurement-client-download.sh &"
            else
                # Unsupported speedtest client
                echo "$(date +"[%Y-%m-%d %H:%M:%S]") Unsupported device model" >> "$LOG_FILE"
            fi
            ;;
        "XB6" | "VNTXER5")
            # Execute speedtest-client-c for XB6/XB7/XB8
            echo "$(date +"[%Y-%m-%d %H:%M:%S]") Executing speedtest-client-c for XB6/XB7/XB8" >> "$LOG_FILE"
            if [ "$MANUFACTURE" = "Technicolor" ]; then
                nice -n 19 /usr/bin/speedtest-client
            else
                /usr/bin/speedtest-client
            fi
            ;;
        "TCCBR" | "WNXL11BWL" | "SR213")
            # Execute speedtest-client-c for TCCBR, WNXL11BWL, or SR213
            echo "$(date +"[%Y-%m-%d %H:%M:%S]") Executing speedtest-client-c for $BOX_TYPE" >> "$LOG_FILE"
            /usr/bin/speedtest-client
            ;;
        *)
            # Unsupported speedtest client
            echo "$(date +"[%Y-%m-%d %H:%M:%S]") Unsupported device model" >> "$LOG_FILE"
            ;;
    esac
else
    echo "$(date +"[%Y-%m-%d %H:%M:%S]") Unsupported client" >> "$LOG_FILE"
fi
