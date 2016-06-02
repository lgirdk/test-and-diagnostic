#! /bin/sh

source /fss/gw/etc/utopia/service.d/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source /fss/gw/usr/ccsp/tad/corrective_action.sh

LIGHTTPD_CONF="/var/lighttpd.conf"

	# Checking PSM's PID
	PSM_PID=`pidof PsmSsp`
	if [ "$PSM_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "[`getDateTime`] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM PsmSsp process died,need reboot"
		touch $HAVECRASH		
		rebootNeeded RM "PSM"
	fi

	# Checking CR's PID
	CR_PID=`pidof CcspCrSsp`
	if [ "$CR_PID" = "" ]; then
	#	echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "[`getDateTime`] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"

		touch $HAVECRASH
		rebootNeeded RM "CR"
	fi

	PAM_PID=`pidof CcspPandMSsp`
	if [ "$PAM_PID" = "" ]; then
		# Remove the P&M initialized flag
		rm -rf /tmp/pam_initialized
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PAM_process is not running, need restart"
		resetNeeded pam CcspPandMSsp
	fi
	
	# Checking MTA's PID
	MTA_PID=`pidof CcspMtaAgentSsp`
	if [ "$MTA_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : MTA_process is not running, restarting it"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
		resetNeeded mta CcspMtaAgentSsp

	fi

	# Checking CM's PID
	CM_PID=`pidof CcspCMAgentSsp`
	if [ "$CM_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CM_process is not running, restarting it"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CM_process is not running, need restart"
		resetNeeded cm CcspCMAgentSsp
	fi

	# Checking WEBController's PID
	WEBC_PID=`pidof CcspWecbController`
	if [ "$WEBC_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : WECBController_process is not running, restarting it"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : WECBController_process is not running, need restart"
		resetNeeded wecb CcspWecbController
	fi

	# Checking RebootManager's PID
#	Rm_PID=`pidof CcspRmSsp`
#	if [ "$Rm_PID" = "" ]; then
	#	echo "[`getDateTime`] RDKB_PROCESS_CRASHED : RebootManager_process is not running, restarting it"
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : RebootManager_process is not running, need restart"
#		resetNeeded "rm" CcspRmSsp

#	fi

	# Checking TR69's PID
	TR69_PID=`pidof CcspTr069PaSsp`
	if [ "$TR69_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "[`getDateTime`] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspTr069PaSsp process died,need reboot"

		touch $HAVECRASH
		rebootNeeded RM "TR69"
	fi

	# Checking Test adn Daignostic's PID
	TandD_PID=`pidof CcspTandDSsp`
	if [ "$TandD_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TandD_process is not running, restarting it"
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TandD_process is not running, need restart"
		resetNeeded tad CcspTandDSsp
	fi

	# Checking Lan Manager PID
	LM_PID=`pidof CcspLMLite`
	if [ "$LM_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : LanManager_process is not running, need restart"
		resetNeeded lm CcspLMLite noSubsys
	
	fi

	# Checking snmp subagent PID
	SNMP_PID=`pidof snmp_subagnet`
	if [ "$SNMP_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
		resetNeeded snmp snmp_subagent 
	fi

	HOMESEC_PID=`pidof CcspHomeSecurity`
	if [ "$HOMESEC_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : HomeSecurity process is not running, need restart"
		resetNeeded "" CcspHomeSecurity 
	fi

	HOTSPOT_ENABLE=`dmcli eRT getv Device.DeviceInfo.X_COMCAST_COM_xfinitywifiEnable | grep value | cut -f3 -d : | cut -f2 -d" "`
	if [ "$HOTSPOT_ENABLE" = "true" ]
	then
	
   		HOTSPOTDAEMON_PID=`pidof hotspotfd`
        if [ "$HOTSPOTDAEMON_PID" = "" ] && [ -f /tmp/hotspotfd_up ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : HotSpot_process is not running, need restart"
			resetNeeded "" hotspotfd 
   		fi   
 		DHCP_SNOOPERD_PID=`pidof dhcp_snooperd`
        if [ "$DHCP_SNOOPERD_PID" = "" ] && [ -f /tmp/dhcp_snooperd_up ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : DhcpSnooper_process is not running, need restart"
			resetNeeded "" dhcp_snooperd 
   		fi 
		DHCP_ARP_PID=`pidof hotspot_arpd`
        if [ "$DHCP_ARP_PID" = "" ] && [ -f /tmp/hotspot_arpd_up ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : DhcpArp_process is not running, need restart"
			resetNeeded "" hotspot_arpd 
   		fi

	fi
	# Checking webpa PID
	WEBPA_PID=`pidof webpa`
	if [ "$WEBPA_PID" = "" ]; then
		ENABLEWEBPA=`cat /nvram/webpa_cfg.json | grep -r EnablePa | awk '{print $2}' | sed 's|[\"\",]||g'`
		if [ "$ENABLEWEBPA" = "true" ];then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : WebPA_process is not running, need restart"
			resetNeeded webpa webpa
		fi
	
	fi

	DROPBEAR_PID=`pidof dropbear`
	if [ "$DROPBEAR_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : dropbear_process is not running, restarting it"
		sh /etc/utopia/service.d/service_sshd.sh sshd-restart &
	fi

	# Checking lighttpd PID
	LIGHTTPD_PID=`pidof lighttpd`
	if [ "$LIGHTTPD_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : lighttpd is not running, restarting it"
		lighttpd -f $LIGHTTPD_CONF
	fi

	ifconfig | grep brlan1
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : brlan1 interface is not up, need to reboot the unit" 
		rebootNeededforbrlan1=1
		rebootDeviceNeeded=1
	fi

	ifconfig | grep brlan0
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : brlan0 interface is not up" 
		echo "[`getDateTime`] RDKB_REBOOT : brlan0 interface is not up, rebooting the device"
		rebootNeeded RM ""
	fi

	dmcli eRT getv Device.WiFi.SSID.2.Status | grep Up
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : 5G private SSID (ath1) is off, resetting WiFi now"
		dmcli eRT setv Device.X_CISCO_COM_DeviceControl.RebootDevice string Wifi
	fi

	iptables-save -t nat | grep "A PREROUTING -i"
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : iptable corrupted. Restarting firewall"
		sysevent set firewall-restart
	fi

	if [ "$rebootDeviceNeeded" -eq 1 ]
	then
		cur_hr=`date +"%H"`
		cur_min=`date +"%M"`
		if [ $cur_hr -ge 02 ] && [ $cur_hr -le 03 ]
		then
			if [ $cur_hr -eq 03 ] && [ $cur_min -ne 00 ]
			then
				echo "Maintanance window for the current day is over , unit will be rebooted in next Maintanance window "
			else
			#Check if we have already flagged reboot is needed
				if [ ! -e $FLAG_REBOOT ]
				then
					if [ "$rebootNeededforbrlan1" -eq 1 ]
					then
						echo "rebootNeededforbrlan1"
						echo "[`getDateTime`] RDKB_REBOOT : brlan1 interface is not up, rebooting the device."
						sh /etc/calc_random_time_to_reboot_dev.sh "" &
					else 
						echo "rebootDeviceNeeded"
						sh /etc/calc_random_time_to_reboot_dev.sh "" &
					fi
					touch $FLAG_REBOOT
				else
					echo "Already waiting for reboot"
				fi					
			fi
		fi
	fi
