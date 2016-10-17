#This script is used to enable / start the speedtest tool

#!/bin/sh
echo "Enabling / Starting speedtest..." > /dev/console
. /etc/device.properties
if [ "$BOX_TYPE" = XB3 ]
then
  rpcclient 192.168.254.254 "/etc/speedtest/run_speedtest.sh"
else
  echo "Unsupported device model" > /dev/console
fi
