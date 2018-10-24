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

echo "Enabling / Starting speedtest..." > /dev/console
. /etc/device.properties
if [ "$BOX_TYPE" = XB3 ] && [ "$MODEL_NUM" = TG1682G ]
then
  # There are two implementations of the speedtest that currently must co-exist on the XB3 - a NodeJS based client, and a newer C/CPP client.
  # If Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType exists, then:
  # Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType = 1 implies the C client must be executed.
  # Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType = 2 implies the NodeJS client must be executed.
  ST_CLIENT_TYPE=`dmcli eRT getv Device.IP.Diagnostics.X_RDKCENTRAL-COM_SpeedTest.ClientType | grep value | cut -d ":" -f 3 | tr -d ' '`
  if [ "x$ST_CLIENT_TYPE" = 'x1' ]
  then
    # C speedtest client
    echo "Executing native c speedtest-client for xb3" > /dev/console
    rpcclient $ATOM_ARPING_IP "/usr/bin/speedtest-client &"
  else
    # NodeJS speedtest client
    echo "Executing run_speedtest.sh for xb3" > /dev/console
    rpcclient $ATOM_ARPING_IP "/etc/speedtest/run_speedtest.sh &"
  fi
elif [ "$BOX_TYPE" = XB6 ] || [ "$BOX_TYPE" = XF3 ] || [ "$BOX_TYPE" = TCCBR ]
then
  echo "Executing run_speedtest.sh" > /dev/console
  sh /etc/speedtest/run_speedtest.sh
else
  echo "Unsupported device model" > /dev/console
fi
