#! /bin/sh
UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`

if [ "$UPTIME" -lt 1800 ]
then
    exit 0
fi

sh /usr/ccsp/tad/log_mem_cpu_info.sh &
sh /usr/ccsp/tad/uptime.sh &

