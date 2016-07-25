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

source /usr/ccsp/tad/corrective_action.sh

hours=0
days=0
isUpforDays=`uptime | grep days`

if [ "$isUpforDays" = "" ]
then
    days=0
    notInDays=`uptime | awk -F, '{sub(".*up ",x,$1);print $1}'`
    
    # Check whether we have got any hour field
    isMoreThanHour=`uptime | awk -F, '{sub(".*up ",x,$1);print $1}' | grep ":"`
    if [ "$isMoreThanHour" != "" ]
    then
        hours=`echo $isMoreThanHour | tr -d " " | cut -f1 -d:`
    else
        echo "RDKB_UPTIME: Unit is up for less than 1 hour"
        hours=0
    fi
    
else
    # Get days and uptime
    upInDays=`uptime | awk -F, '{sub(".*up ",x,$1);print $1,$2}'`
     
    days=`echo $upInDays | cut -f1 -d" "`
    # Check whether we have got any hour field
    isHourPresent=`echo $upInDays | awk -F, '{sub(".*days ",x,$1);print $1}' | grep ":"`
    if [ "$isHourPresent" != "" ]
    then
       hours=`echo $isHourPresent | tr -d " " | cut -f1 -d:`
    else
       hours=0    
    fi
fi
    
echo "UPTIME:$days,$hours"

