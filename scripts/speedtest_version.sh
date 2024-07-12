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

#This script is used to execute speedtest-client binary to retrieve version
VERSION_LOG_FILE=/tmp/.speedtest-client-version.log
. /etc/device.properties

case "$BOX_TYPE" in
    "XB3")
        if [ "$MODEL_NUM" = "TG1682G" ]; then
            # C speedtest client
            rpcclient "$ATOM_ARPING_IP" "/usr/bin/speedtest-client -v" > "$VERSION_LOG_FILE"
        elif [ "$MODEL_NUM" = "DPC3941" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
            # C speedtest client
            rpcclient "$ATOM_ARPING_IP" "sh /etc/measurement-client-download.sh &"
        else
            # Unsupported speedtest client
            echo "Unsupported device model" > "$VERSION_LOG_FILE"
        fi
        ;;
    "XB6" | "TCCBR" | "WNXL11BWL" | "SR213" | "VNTXER5")
        # C speedtest client
        /usr/bin/speedtest-client -v > "$VERSION_LOG_FILE"
        ;;
    *)
        # Unsupported speedtest client
        echo "Unsupported device model" > "$VERSION_LOG_FILE"
        ;;
esac
