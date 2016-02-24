#! /bin/sh

source /fss/gw/etc/utopia/service.d/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source /fss/gw/usr/ccsp/tad/corrective_action.sh

	# Checking PandM's PID
	PAM_PID=`pidof CcspPandMSsp`
	if [ "$PAM_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : PAM_process is not running, need to reboot the unit now"
		echo "RDKB_PROCESS_CRASHED : PAM_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspPandMSsp process died,need reboot"
		touch $HAVECRASH
		rebootNeeded RM "PAM"
	fi

	# Checking PSM's PID
	PSM_PID=`pidof PsmSsp`
	if [ "$PSM_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
		echo "RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM PsmSsp process died,need reboot"
		touch $HAVECRASH		
		rebootNeeded RM "PSM"
	fi

	# Checking CR's PID
	CR_PID=`pidof CcspCrSsp`
	if [ "$CR_PID" = "" ]; then
	#	echo "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
		echo "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"

		touch $HAVECRASH
		rebootNeeded RM "CR"
	fi

	
	# Checking MTA's PID
	MTA_PID=`pidof CcspMtaAgentSsp`
	if [ "$MTA_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : MTA_process is not running, restarting it"
		echo "RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
		resetNeeded mta CcspMtaAgentSsp

	fi

	# Checking CM's PID
	CM_PID=`pidof CcspCMAgentSsp`
	if [ "$CM_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : CM_process is not running, restarting it"
		echo "RDKB_PROCESS_CRASHED : CM_process is not running, need restart"
		resetNeeded cm CcspCMAgentSsp
	fi

	# Checking WEBController's PID
	WEBC_PID=`pidof CcspWecbController`
	if [ "$WEBC_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : WECBController_process is not running, restarting it"
		echo "RDKB_PROCESS_CRASHED : WECBController_process is not running, need restart"
		resetNeeded wecb CcspWecbController
	fi

	# Checking RebootManager's PID
	Rm_PID=`pidof CcspRmSsp`
	if [ "$Rm_PID" = "" ]; then
	#	echo "RDKB_PROCESS_CRASHED : RebootManager_process is not running, restarting it"
		echo "RDKB_PROCESS_CRASHED : RebootManager_process is not running, need restart"
		resetNeeded "rm" CcspRmSsp

	fi

	# Checking TR69's PID
	TR69_PID=`pidof CcspTr069PaSsp`
	if [ "$TR69_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
		echo "RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspTr069PaSsp process died,need reboot"

		touch $HAVECRASH
		rebootNeeded RM "TR69"
	fi

	# Checking Test adn Daignostic's PID
	TandD_PID=`pidof CcspTandDSsp`
	if [ "$TandD_PID" = "" ]; then
#		echo "RDKB_PROCESS_CRASHED : TandD_process is not running, restarting it"
		echo "RDKB_PROCESS_CRASHED : TandD_process is not running, need restart"
		resetNeeded tad CcspTandDSsp
	fi

	# Checking Lan Manager PID
	LM_PID=`pidof CcspLMLite`
	if [ "$LM_PID" = "" ]; then
		echo "RDKB_PROCESS_CRASHED : LanManager_process is not running, need restart"
		resetNeeded lm CcspLMLite noSubsys
	
	fi

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

