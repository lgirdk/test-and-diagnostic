#! /bin/sh

UTOPIA_PATH="/etc/utopia/service.d"
TAD_PATH="/usr/ccsp/tad"

source $UTOPIA_PATH/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source $TAD_PATH/corrective_action.sh

rebootDeviceNeeded=0

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
		echo "[`getDateTime`] Setting Last reboot reason"
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string Psm_crash
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
		echo "[`getDateTime`] SET succeeded"
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
		echo "[`getDateTime`] Setting Last reboot reason"
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string TR69_crash
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
		echo "[`getDateTime`] SET succeeded"
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
		resetNeeded lm CcspLMLite
	
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
		echo "[`getDateTime`] Setting Last reboot reason"
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string brlan0_down
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
		echo "[`getDateTime`] SET succeeded"
		rebootNeeded RM ""
	fi

	 if [ ! -f /tmp/l2sd0.100logged ] ; then
     
          ifconfig -a | grep l2sd0.100
        if [ $? == 1 ]; then
                echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : l2sd0.100 interface is not created"
	      logNetworkInfo 
	 touch /tmp/l2sd0.100logged
	else
		
	   ifconfig l2sd0.100 | grep UP
	   if [ $? == 1 ]; then
                echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : l2sd0.100 interface is not up"
              logNetworkInfo
		touch /tmp/l2sd0.100logged
	   fi
        fi

	fi

	if [ ! -f /tmp/l2sd0.101logged ] ; then

        ifconfig -a | grep l2sd0.101
        if [ $? == 1 ]; then
                echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : l2sd0.101 interface is not created" 
		touch /tmp/l2sd0.101logged
	else

           ifconfig l2sd0.101 | grep UP
           if [ $? == 1 ]; then
                echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : l2sd0.101 interface is not up"
		touch /tmp/l2sd0.101logged
     
           fi
        fi
	fi

        SSID_DISABLED=0
        BR_MODE=0
        dmcli eRT getv Device.WiFi.SSID.2.Enable | grep "true"
        if [ $? == 1 ]
        then
            SSID_DISABLED=1
            echo "[`getDateTime`] [RDKB_SELFHEAL] : SSID 5GHZ is disabled"
            
        fi

        dmcli eRT getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode | grep router
        if [ $? == 1 ]
        then
            BR_MODE=1
            echo "[`getDateTime`] [RDKB_SELFHEAL] : Device in bridge mode"
            
        fi

        if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED -eq 0 ]
        then
	    dmcli eRT getv Device.WiFi.SSID.2.Status | grep Up
	    if [ $? == 1 ]; then
		echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : 5G private SSID (ath1) is off."
		#dmcli eRT setv Device.X_CISCO_COM_DeviceControl.RebootDevice string Wifi
	    fi
        fi

        if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED -eq 0 ]
        then
	    dmcli eRT getv Device.WiFi.SSID.1.Status | grep Up
	    if [ $? == 1 ]; then
		echo "[`getDateTime`] [RKDB_PLATFORM_ERROR] : 2G private SSID (ath0) is off."
		#dmcli eRT setv Device.X_CISCO_COM_DeviceControl.RebootDevice string Wifi
	    fi
        fi

	iptables-save -t nat | grep "A PREROUTING -i"
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : iptable corrupted."
		#sysevent set firewall-restart
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
						echo "[`getDateTime`] Setting Last reboot reason"
						dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string brlan1_down
						dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
						echo "[`getDateTime`] SET succeeded"
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
