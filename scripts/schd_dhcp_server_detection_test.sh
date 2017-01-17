#!/bin/sh

source /etc/utopia/service.d/log_capture_path.sh

DhcpServDetectEnableStatus=`syscfg get DhcpServDetectEnable`

#DhcpServDetectEnableStatus is true then place the file in /etc/cron/cron.hourly directory for hourly basis execution
#DhcpServDetectEnableStatus is false then remove the file dhcp_rouge_server_detection.sh from /etc/cron/cron.hourly directory

if [ "$DhcpServDetectEnableStatus" = "true" ]
then
	#Gateway sends out DHCP discover message on the MoCA interface every 60 minutes.
      	echo "#! /bin/sh" > /etc/cron/cron.hourly/dhcp_rouge_server_detection.sh
        echo "/usr/ccsp/tad/dhcp_rouge_server_detection.sh" >> /etc/cron/cron.hourly/dhcp_rouge_server_detection.sh
        chmod 700 /etc/cron/cron.hourly/dhcp_rouge_server_detection.sh
else
	if [ -f "/etc/cron/cron.hourly/dhcp_rouge_server_detection.sh" ] 
	then
		rm -rf /etc/cron/cron.hourly/dhcp_rouge_server_detection.sh
	fi
fi


