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

# Purpose of this script is to get the connected clients count.
source /etc/utopia/service.d/log_capture_path.sh

#Initialize all variables
numberofDevices="0"
numberofOffline="0"
numberofOnline="0"

# Get online device count
connected=`dmcli eRT getv Device.Hosts.X_CISCO_COM_ConnectedDeviceNumber | grep value`

if [ "$connected" = "" ]
then
     echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : Cannot get connected devices entry" 
else
     numberofOnline=`echo $connected | cut -f3 -d: | tr -d " "`
     if [ "$numberofOnline" != "" ]
     then
        echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : No. of Online clients is $numberofOnline"
     else
        echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : No. of Online clients is 0"
     fi
fi

# Get total devices count
result=`dmcli eRT getv Device.Hosts.HostNumberOfEntries | grep value`

if [ "$result" = "" ]
then
    echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : Cannot get Host entries"
else
     numberofDevices=`echo $result | cut -f3 -d: | tr -d " "`
     if [ "$numberofDevices" != "" ]
     then
        echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : Total clients count is $numberofDevices"
     else
        echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : Total clients count is 0"
     fi

    if [ "$numberofDevices" != "" ] && [ "$numberofOnline" != "" ]
    then
       if [ "$numberofDevices" -gt "$numberofOnline" ]
       then
           numberofOffline="$((numberofDevices - numberofOnline))"
           echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : No. of Offline clients is $numberofOffline"
       fi
      
       if [ "$numberofDevices" -le "$numberofOnline" ]
       then
           echo "[`date +'%Y:%m:%d:%H:%M:%S'`] RDKB_CONNECTED_CLIENTS : No. of Offline clients is 0"
       fi
    fi
fi

   
