#!/bin/sh
UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`
if [ "$UPTIME" -lt 900 ]
then
    exit 0
fi

rm -rf /etc/cron/cron.every10minute/start_gw_heath.sh

sh /usr/ccsp/tad/check_gw_health.sh bootup-check


