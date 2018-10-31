#!/bin/sh

UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`

if [ "$UPTIME" -lt 600 ]
then
    exit 0
fi

source /etc/utopia/service.d/log_capture_path.sh

if [ -f /tmp/cosa_start_rem_triggered ]; then
	echo "Already cosa_start_rem script triggered so no need to trigger again from selfheal"
	rm -rf /etc/cron/cron.every10minute/selfheal_cosa_start_rem.sh
else
	echo "cosa_start_rem script not triggered even after 10 minutes from boot-up so start from selfheal"
	sh /usr/ccsp/cosa_start_rem.sh &	
	rm -rf /etc/cron/cron.every10minute/selfheal_cosa_start_rem.sh
fi
