#!/bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################

source /etc/log_timestamp.sh    # define 'echo_t' ASAP!
if [ -f /etc/device.properties ];then
source /etc/device.properties
fi

SELFHEAL_TYPE="BASE"

case $1 in 
    "CCSP_WIFI_HUNG")
        case $SELFHEAL_TYPE in
            "TCCBR")
                echo_t "RDKB_SELFHEAL : Restarting wlmngr"
                #Restart wifi to get back wlmngr2 normal
                systemctl stop wifi
                sleep 5
                systemctl start wifi
                sleep 5
                #killing radiohealth.sh to stop wifi_api calls
                ps | grep radiohealth.sh | grep -v "grep" | awk '{print $1}'| xargs -r kill -9
                #killing wifi_apis which is hanging currently
                ps | grep wifi_api | grep -v "grep" | awk '{print $1}'| xargs -r kill -9
                #killing already spawned wifi process to avoid duplicate process creation
                busybox pidof CcspWifiSsp | tr ' ' '\n' | xargs -r kill -9
                #Initialization of wlmngr is getting nearly 50s to become fully functional.Inoder
                #to make proper CcspWifiSsp init we have to wait until wlmngr get ready.
                sleep 60
            ;;
        esac
    ;;
esac
