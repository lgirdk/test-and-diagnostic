#!/bin/sh

UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`

if [ "$UPTIME" -lt 86400 ]
then
    exit 0
fi

UTOPIA_PATH="/etc/utopia/service.d"

source $UTOPIA_PATH/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

getDateTime()
{
	dandtwithns_now=`date +'%Y-%m-%d:%H:%M:%S:%6N'`
	echo "$dandtwithns_now"
}

if [ -f /tmp/CPUUsageReachedMAXThreshold ]; then
	rm -rf /tmp/CPUUsageReachedMAXThreshold
	echo "[`getDateTime`] RDKB_SELFHEAL : Removed /tmp/CPUUsageReachedMAXThreshold file after 24hrs uptime"
fi
