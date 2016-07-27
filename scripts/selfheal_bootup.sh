#!/bin/sh

CM_INTERFACE="wan0"
WAN_INTERFACE="erouter0"
Check_CM_Ip=0
Check_WAN_Ip=0

isIPv4=""
isIPv6=""

RDKLOGGER_PATH="/rdklogger"
TAD_PATH="/usr/ccsp/tad"
UTOPIA_PATH="/etc/utopia/service.d"
PING_PATH="/usr/sbin"
ping_failed=0
ping_success=0

UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`

source $UTOPIA_PATH/log_env_var.sh

if [ "$UPTIME" -lt 600 ]
then
    exit 0
fi

log_nvram2=`syscfg get logbackup_enable`


if [ ! -d "$LOG_SYNC_PATH" ] 
then
	mkdir -p $LOG_SYNC_PATH
fi

if [ "$log_nvram2" == "true" ]
then
	exec 3>&1 4>&2 >>$SELFHEALFILE_BOOTUP 2>&1
fi


# This function will check if captive portal needs to be enabled or not.
checkCaptivePortal()
{

# Get all flags from DBs
isWiFiConfigured=`syscfg get redirection_flag`
psmNotificationCP=`psmcli get eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges`

#Read the http response value
networkResponse=`cat /var/tmp/networkresponse.txt`

iter=0
max_iter=2
while [ "$psmNotificationCP" = "" ] && [ "$iter" -le $max_iter ]
do
	iter=$((iter+1))
	echo "$iter"
	psmNotificationCP=`psmcli get eRT.com.cisco.spvtg.ccsp.Device.WiFi.NotifyWiFiChanges`
done

echo "[`getDateTime`] RDKB_SELFHEAL : NotifyWiFiChanges is $psmNotificationCP"
echo "[`getDateTime`] RDKB_SELFHEAL : redirection_flag val is $isWiFiConfigured"

if [ "$isWiFiConfigured" = "true" ]
then
	if [ "$networkResponse" = "204" ] && [ "$psmNotificationCP" = "true" ]
	then
		# Check if P&M is up and able to find the captive portal parameter
		while : ; do
			echo "[`getDateTime`] RDKB_SELFHEAL : Waiting for PandM to initalize completely to set ConfigureWiFi flag"
			CHECK_PAM_INITIALIZED=`find /tmp/ -name "pam_initialized"`
			echo "[`getDateTime`] RDKB_SELFHEAL : CHECK_PAM_INITIALIZED is $CHECK_PAM_INITIALIZED"
			if [ "$CHECK_PAM_INITIALIZED" != "" ]
			then
				echo "[`getDateTime`] RDKB_SELFHEAL : WiFi is not configured, setting ConfigureWiFi to true"
				output=`dmcli eRT setvalues Device.DeviceInfo.X_RDKCENTRAL-COM_ConfigureWiFi bool TRUE`
				check_success=`echo $output | grep  "Execution succeed."`
				if [ "$check_success" != "" ]
				then
					echo "[`getDateTime`] RDKB_SELFHEAL : Setting ConfigureWiFi to true is success"
				fi
				break
			fi
			sleep 2
		done
	else
		echo "[`getDateTime`] RDKB_SELFHEAL : We have not received a 204 response or PSM valus is not in sync"
	fi
else
	echo "[`getDateTime`] RDKB_SELFHEAL : Syscfg DB value is : $isWiFiConfigured"
fi	

}
getDateTime()
{
	dandtwithns_now=`date +'%Y-%m-%d:%H:%M:%S:%6N'`
	echo "$dandtwithns_now"
}
 

resetNeeded()
{
	ProcessName=$1
	BINPATH="/usr/bin"
	export LD_LIBRARY_PATH=$PWD:.:$PWD/../../lib:$PWD/../../.:/lib:/usr/lib:$LD_LIBRARY_PATH
	export DBUS_SYSTEM_BUS_ADDRESS=unix:path=/var/run/dbus/system_bus_socket

	if [ -f /tmp/cp_subsys_ert ]; then
        	Subsys="eRT."
	elif [ -e ./cp_subsys_emg ]; then
        	Subsys="eMG."
	else
        	Subsys=""
	fi

	echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : Resetting process $ProcessName"
	cd /usr/ccsp/pam/
	$BINPATH/CcspPandMSsp -subsys $Subsys
	cd -
	# We need to check whether to enable captive portal flag
	checkCaptivePortal

}

if [ "$log_nvram2" == "true" ]
then    
# Check for CM IP Address 
 	isIPv4=`ifconfig $CM_INTERFACE | grep inet | grep -v inet6`
	 if [ "$isIPv4" == "" ]
	 then
        	 isIPv6=`ifconfig $CM_INTERFACE | grep inet6 | grep "Scope:Global"`
        	 if [ "$isIPv6" != "" ]
		 then
			Check_CM_Ip=1
		 else
		   	Check_CM_Ip=0
			echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : CM interface doesn't have IP"
		 fi
	 else
		Check_CM_Ip=1
	 fi

#Check whether system time is in 1970's
	
         if [ "$Check_CM_Ip" -eq 1 ]
	 then
		year_is=`date +"%Y"`
		if [ "$year_is" == "1970" ]
		then
			echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : System time is in 1970's"
		fi
	 fi

isIPv4=""
isIPv6=""
# Check for WAN IP Address 

	 isIPv4=`ifconfig $WAN_INTERFACE | grep inet | grep -v inet6`
	 if [ "$isIPv4" == "" ]
	 then
        	 isIPv6=`ifconfig $WAN_INTERFACE | grep inet6 | grep "Scope:Global"`
        	 if [ "$isIPv6" != "" ]
		 then
			Check_WAN_Ip=1
		 else
		   	Check_WAN_Ip=0
			echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : WAN interface doesn't have IP"
		 fi
	 else
		Check_WAN_Ip=1
	 fi


# Check whether PSM process is running
	# Checking PSM's PID
	PSM_PID=`pidof PsmSsp`
	if [ "$PSM_PID" == "" ]; then

		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
		echo "[`getDateTime`] Setting Last reboot reason"
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string Psm_crash
		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
		echo "[`getDateTime`] SET succeeded"
		touch $HAVECRASH		
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need reboot"
		echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : PSM_process is not running, need reboot"

			if [ ! -f "/nvram/self_healreboot" ]
			then
				touch /nvram/self_healreboot
				$RDKLOGGER_PATH/backupLogs.sh "true" "PSM"
			else
				rm -rf /nvram/self_healreboot
			fi

	fi


# Check whether PAM process is running
	PAM_PID=`pidof CcspPandMSsp`
	if [ "$PAM_PID" == "" ]; then
		# Remove the P&M initialized flag
		rm -rf /tmp/pam_initialized
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PAM_process is not running, need restart"
		echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : PAM_process is not running, need restart"
		resetNeeded CcspPandMSsp
	fi

## Check Peer ip is accessible
if [ -f $PING_PATH/ping_peer ]
then
loop=1
	while [ "$loop" -le 3 ]
	do
	
        PING_RES=`ping_peer`
	CHECK_PING_RES=`echo $PING_RES | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1`

		if [ "$CHECK_PING_RES" != "" ]
		then
			if [ "$CHECK_PING_RES" -ne 100 ] 
			then
				ping_success=1
				echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : Ping to Peer IP is success"
				break
			else
				ping_failed=1
			fi
		else
			ping_failed=1
		fi
		
		if [ "$ping_failed" -eq 1 ] && [ "$loop" -lt 3 ]
		then
			echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : Ping to Peer IP failed in iteration $loop"
		else
			echo "[`getDateTime`] RDKB_SELFHEAL_BOOTUP : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
			if [ ! -f "/nvram/self_healreboot" ]
			then
				touch /nvram/self_healreboot
				echo "[`getDateTime`] RDKB_REBOOT : Peer is not up ,Rebooting device "
				$RDKLOGGER_PATH/backupLogs.sh "true" ""
			else
				rm -rf /nvram/self_healreboot
			fi

		fi
		loop=$((loop+1))
		sleep 5
	done
else
   echo "RDKB_SELFHEAL_BOOTUP : ping_peer command not found"
fi

# Check for iptable corruption

        bridgeMode=`dmcli eRT getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode`
        isBridging=`echo $bridgeMode | grep router`
        if [ "$isBridging" != "" ]
        then
		Check_Iptable_Rules=`iptables-save -t nat | grep "A PREROUTING -i"`
		if [ "$Check_Iptable_Rules" == "" ]; then
		echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : iptable corrupted."
		#sysevent set firewall-restart
		fi
        fi

# Check brlan0,brlan1.l2sd0.100 and l2sd0.101 interface state

	check_brlan0=`ifconfig -a | grep brlan0`
	if [ "$check_brlan0" != "" ]
	then
		check_brlan0_state=`ifconfig brlan0 | grep UP`
		if [ "$check_brlan0_state" == "" ]
		then
			echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : brlan0 interface is not up" 
		fi
	else
		echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : brlan0 interface is not created" 
	fi

	check_brlan1=`ifconfig -a | grep brlan1`
	if [ "$check_brlan1" != "" ]
	then
		check_brlan1_state=`ifconfig brlan1 | grep UP`
		if [ "$check_brlan1_state" == "" ]
		then
			echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : brlan1 interface is not up" 
		fi
	else
		echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : brlan1 interface is not created" 
	fi


	check_l2sd0_100=`ifconfig -a | grep l2sd0.100`
	if [ "$check_l2sd0_100" != "" ]
	then
		check_l2sd0_100_state=`ifconfig l2sd0.100 | grep UP`
		if [ "$check_l2sd0_100_state" == "" ]
		then
			echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : l2sd0.100 interface is not up" 
		fi
	else
		echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : l2sd0.100 interface is not created" 
	fi


	check_l2sd0_101=`ifconfig -a | grep l2sd0.101`
	if [ "$check_l2sd0_101" != "" ]
	then
		check_l2sd0_101_state=`ifconfig l2sd0.101 | grep UP`
		if [ "$check_l2sd0_101_state" == "" ]
		then
			echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : l2sd0.101 interface is not up" 
		fi
	else
		echo "[`getDateTime`] [RDKB_SELFHEAL_BOOTUP] : l2sd0.101 interface is not created" 
	fi

	rm -rf /etc/cron/cron.everyminute/selfheal_bootup.sh
else
	echo "RDKB_SELFHEAL_BOOTUP : nvram2 logging is disabled , not logging data"
	rm -rf /etc/cron/cron.everyminute/selfheal_bootup.sh
fi

