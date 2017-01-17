#!/bin/sh

UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`

if [ "$UPTIME" -lt 3600 ]
then
    exit 0
fi

source /etc/utopia/service.d/log_capture_path.sh

#Gateway sends out DHCP discover message on the MoCA interface every 60 minutes.
echo_t "Gateway started to send DHCP discover message on the MoCA interface"
/usr/bin/dhcpsrv_detect
echo_t "Gateway sends out DHCP discover message on the MoCA interface"

