#! /bin/sh

source /etc/utopia/service.d/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source /usr/ccsp/tad/corrective_action.sh

	# Checking snmp subagent PID
	SNMP_PID=`pidof snmp_subagnet`
	if [ "$SNMP_PID" = "" ]; then
		echo "RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
		resetNeeded snmp snmp_subagent 
	fi

	HOMESEC_PID=`pidof CcspHomeSecurity`
	if [ "$HOMESEC_PID" = "" ]; then
		echo "RDKB_PROCESS_CRASHED : HomeSecurity process is not running, need restart"
		resetNeeded "" CcspHomeSecurity 
	fi

	HOTSPOT_ENABLE=`dmcli eRT getv Device.DeviceInfo.X_COMCAST_COM_xfinitywifiEnable | grep value | cut -f3 -d : | cut -f2 -d" "`
	if [ "$HOTSPOT_ENABLE" = "true" ]
	then
	
   		HOTSPOTDAEMON_PID=`pidof hotspotfd`
   		if [ "$HOTSPOTDAEMON_PID" = "" ]; then
			echo "RDKB_PROCESS_CRASHED : HotSpot_process is not running, need restart"
			resetNeeded "" hotspotfd 
   		fi   
 		DHCP_SNOOPERD_PID=`pidof dhcp_snooperd`
   		if [ "$DHCP_SNOOPERD_PID" = "" ]; then
			echo "RDKB_PROCESS_CRASHED : DhcpSnooper_process is not running, need restart"
			resetNeeded "" dhcp_snooperd 
   		fi 
		DHCP_ARP_PID=`pidof hotspot_arpd`
   		if [ "$DHCP_ARP_PID" = "" ]; then
			echo "RDKB_PROCESS_CRASHED : DhcpArp_process is not running, need restart"
			resetNeeded "" hotspot_arpd 
   		fi

	fi
	# Checking webpa PID
	WEBPA_PID=`pidof webpa`
	if [ "$WEBPA_PID" = "" ]; then
		ENABLEWEBPA=`cat /nvram/webpa_cfg.json | grep -r EnablePa | awk '{print $2}' | sed 's|[\"\",]||g'`
		if [ "$ENABLEWEBPA" = "true" ];then
		echo "RDKB_PROCESS_CRASHED : WebPA_process is not running, need restart"
			resetNeeded webpa webpa
		fi
	
	fi

	DROPBEAR_PID=`pidof dropbear`
	if [ "$DROPBEAR_PID" = "" ]; then
		echo "RDKB_PROCESS_CRASHED : dropbear_process is not running, restarting it"
		sh /etc/utopia/service.d/service_sshd.sh sshd-restart &
	fi

