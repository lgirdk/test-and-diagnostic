#! /bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################

UTOPIA_PATH="/etc/utopia/service.d"
TAD_PATH="/usr/ccsp/tad"
RDKLOGGER_PATH="/rdklogger"

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi
source /etc/log_timestamp.sh
ping_failed=0
ping_success=0
SyseventdCrashed="/rdklogs/syseventd_crashed"
PING_PATH="/usr/sbin"
WAN_INTERFACE="erouter0"
PEER_COMM_ID="/tmp/elxrretyt.swr"

if [ ! -f /usr/bin/GetConfigFile ];then
    echo "Error: GetConfigFile Not Found"
    exit
fi
IDLE_TIMEOUT=30
source $UTOPIA_PATH/log_env_var.sh

CCSP_ERR_TIMEOUT=191
CCSP_ERR_NOT_EXIST=192

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source $TAD_PATH/corrective_action.sh

rebootDeviceNeeded=0
reb_window=0

LIGHTTPD_CONF="/var/lighttpd.conf"

	# Checking PSM's PID
	PSM_PID=`pidof PsmSsp`
	if [ "$PSM_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
#		vendor=`getVendorName`
#		modelName=`getModelName`
#		CMMac=`getCMMac`
#		timestamp=`getDate`
#		echo "[`getDateTime`] Setting Last reboot reason"
#		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string Psm_crash
#		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
#		echo "[`getDateTime`] SET succeeded"
#		echo "[`getDateTime`] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM PsmSsp process died,need reboot"
#		touch $HAVECRASH		
#		rebootNeeded RM "PSM"
		echo_t "RDKB_PROCESS_CRASHED : PSM_process is not running, need restart"
		resetNeeded psm PsmSsp
	else
		psm_name=`dmcli eRT getv com.cisco.spvtg.ccsp.psm.Name`
		psm_name_timeout=`echo $psm_name | grep "$CCSP_ERR_TIMEOUT"`
		psm_name_notexist=`echo $psm_name | grep "$CCSP_ERR_NOT_EXIST"`
		if [ "$psm_name_timeout" != "" ] || [ "$psm_name_notexist" != "" ]; then
			psm_health=`dmcli eRT getv com.cisco.spvtg.ccsp.psm.Health`
			psm_health_timeout=`echo $psm_health | grep "$CCSP_ERR_TIMEOUT"`
			psm_health_notexist=`echo $psm_health | grep "$CCSP_ERR_NOT_EXIST"`
			if [ "$psm_health_timeout" != "" ] || [ "$psm_health_notexist" != "" ]; then
				echo_t "RDKB_PROCESS_CRASHED : PSM_process is in hung state, need restart"
				kill -9 `pidof PsmSsp`
				resetNeeded psm PsmSsp
			fi
		fi
	fi
##################################
	if [ "$BOX_TYPE" = "XB3" ]; then
		  wifi_check=`dmcli eRT getv Device.WiFi.SSID.1.Enable`
		  wifi_timeout=`echo $wifi_check | grep "$CCSP_ERR_TIMEOUT"`
		  if [ "$wifi_timeout" != "" ]; then
				  echo_t "[RDKB_SELFHEAL] : Wifi query timeout"
		  fi

                  
          GetConfigFile $PEER_COMM_ID
		  SSH_ATOM_TEST=$(ssh -i $PEER_COMM_ID root@$ATOM_IP exit 2>&1)
		  SSH_ERROR=`echo $SSH_ATOM_TEST | grep "Remote closed the connection"`
                  rm -f $PEER_COMM_ID
		  if [ "$SSH_ERROR" != "" ]; then
				  echo_t "[RDKB_SELFHEAL] : ssh to atom failed"
		  fi

		  if [ "$wifi_timeout" != "" ] && [ "$SSH_ERROR" != "" ]
		  then
				  atom_hang_count=`sysevent get atom_hang_count`
				  echo_t "[RDKB_SELFHEAL] : Atom is not responding. Count $atom_hang_count"
				  if [ $atom_hang_count -ge 2 ]; then
						  CheckRebootCretiriaForAtomHang
						  atom_hang_reboot_count=`syscfg get todays_atom_reboot_count`
						  if [ $atom_hang_reboot_count -eq 0 ]; then
							  echo_t "[RDKB_PLATFORM_ERROR] : Atom is not responding. Rebooting box.."
							  reason="ATOM_HANG"
							  rebootCount=1
							  setRebootreason $reason $rebootCount
							  rebootNeeded $reason ""
						  else
							  echo_t "[RDKB_SELFHEAL] : Reboot allowed for only one time per day. It will reboot in next 24hrs."
						  fi
				  else
						  atom_hang_count=$((atom_hang_count + 1))
						  sysevent set atom_hang_count $atom_hang_count
				  fi
		  else
				  sysevent set atom_hang_count 0
		  fi
	fi
###########################################

if [ "$MULTI_CORE" = "yes" ]; then
if [ -f $PING_PATH/ping_peer ]
then
## Check Peer ip is accessible
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
				echo_t "RDKB_SELFHEAL : Ping to Peer IP is success"
				break
			else
				ping_failed=1
			fi
		else
			ping_failed=1
		fi
		
		if [ "$ping_failed" -eq 1 ] && [ "$loop" -lt 3 ]
		then
			echo_t "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
			echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
		else
			echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
			echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
			echo_t "RDKB_REBOOT : Peer is not up ,Rebooting device "
            		echo_t " RDKB_SELFHEAL : Setting Last reboot reason as Peer_down"
            		reason="Peer_down"
          		rebootCount=1
         	    	setRebootreason $reason $rebootCount
			rebootNeeded RM ""

		fi
		loop=$((loop+1))
		sleep 5
	done
else
   echo_t "RDKB_SELFHEAL : ping_peer command not found"
fi

if [ -f $PING_PATH/arping_peer ]
then
    $PING_PATH/arping_peer
else
   echo_t "RDKB_SELFHEAL : arping_peer command not found"
fi
fi
########################################

	atomOnlyReboot=`dmesg -n 8 && dmesg | grep -i "Atom only"`
	if [ x$atomOnlyReboot = x ];then
		crTestop=`dmcli eRT getv com.cisco.spvtg.ccsp.CR.Name`
 		isCRAlive=`echo $crTestop | grep "Can't find destination compo"`
		if [ "$isCRAlive" != "" ]; then
			# Retest by querying some other parameter
			crReTestop=`dmcli eRT getv Device.X_CISCO_COM_DeviceControl.DeviceMode`
 			isCRAlive=`echo $crReTestop | grep "Can't find destination compo"`
		  	if [ "$isCRAlive" != "" ]; then
		#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
				echo_t "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
				vendor=`getVendorName`
				modelName=`getModelName`
				CMMac=`getCMMac`
				timestamp=`getDate`
				echo_t "Setting Last reboot reason"
				reason="CR_crash"
				rebootCount=1
				setRebootreason $reason $rebootCount
				echo_t "SET succeeded"
				echo_t "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"
				touch $HAVECRASH
				rebootNeeded RM "CR"
		 	fi		
		fi
	else
		echo_t "[RDKB_SELFHEAL] : Atom only reboot is triggered"
	fi

###########################################


	PAM_PID=`pidof CcspPandMSsp`
	if [ "$PAM_PID" = "" ]; then
		# Remove the P&M initialized flag
		rm -rf /tmp/pam_initialized
		echo_t "RDKB_PROCESS_CRASHED : PAM_process is not running, need restart"
		resetNeeded pam CcspPandMSsp
	fi
	
	# Checking MTA's PID
	MTA_PID=`pidof CcspMtaAgentSsp`
	if [ "$MTA_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
		resetNeeded mta CcspMtaAgentSsp

	fi

	WiFi_Flag=false
	# Checking Wifi's PID
	WIFI_PID=`pidof CcspWifiSsp`
	if [ "$WIFI_PID" = "" ]; then
			# Remove the wifi initialized flag
			rm -rf /tmp/wifi_initialized
			echo_t "RDKB_PROCESS_CRASHED : WIFI_process is not running, need restart"
			resetNeeded wifi CcspWifiSsp
	else
		radioenable=`dmcli eRT getv Device.WiFi.Radio.1.Enable`
		radioenable_timeout=`echo $radioenable | grep "$CCSP_ERR_TIMEOUT"`
		radioenable_notexist=`echo $radioenable | grep "$CCSP_ERR_NOT_EXIST"`
		if [ "$radioenable_timeout" != "" ] || [ "$radioenable_notexist" != "" ]; then
			wifi_name=`dmcli eRT getv com.cisco.spvtg.ccsp.wifi.Name`
			wifi_name_timeout=`echo $wifi_name | grep "$CCSP_ERR_TIMEOUT"`
			wifi_name_notexist=`echo $wifi_name | grep "$CCSP_ERR_NOT_EXIST"`
			if [ "$wifi_name_timeout" != "" ] || [ "$wifi_name_notexist" != "" ]; then
				echo_t "[RDKB_PLATFORM_ERROR] : CcspWifiSsp process is restarting"
				# Remove the wifi initialized flag
				rm -rf /tmp/wifi_initialized
				resetNeeded wifi CcspWifiSsp
				WiFi_Flag=true
			fi
		fi
	fi
	
	if [ -f /tmp/wifi_eapd_restart_required ] ; then
		echo_t "RDKB_PROCESS_CRASHED : eapd wifi process needs restart"
		killall eapd
		#starting the eapd process 
		eapd
		rm -rf /tmp/wifi_eapd_restart_required 
	fi
	
	# Checking CM's PID
	CM_PID=`pidof CcspCMAgentSsp`
	if [ "$CM_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : CM_process is not running, need restart"
		resetNeeded cm CcspCMAgentSsp
	fi

	# Checking WEBController's PID
#	WEBC_PID=`pidof CcspWecbController`
#	if [ "$WEBC_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : WECBController_process is not running, restarting it"
#		echo_t "RDKB_PROCESS_CRASHED : WECBController_process is not running, need restart"
#		resetNeeded wecb CcspWecbController
#	fi

	# Checking RebootManager's PID
#	Rm_PID=`pidof CcspRmSsp`
#	if [ "$Rm_PID" = "" ]; then
	#	echo "[`getDateTime`] RDKB_PROCESS_CRASHED : RebootManager_process is not running, restarting it"
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : RebootManager_process is not running, need restart"
#		resetNeeded "rm" CcspRmSsp

#	fi

	# Checking Test adn Daignostic's PID
	TandD_PID=`pidof CcspTandDSsp`
	if [ "$TandD_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : TandD_process is not running, need restart"
		resetNeeded tad CcspTandDSsp
	fi

	# Checking Lan Manager PID
	LM_PID=`pidof CcspLMLite`
	if [ "$LM_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : LanManager_process is not running, need restart"
		resetNeeded lm CcspLMLite
	
	fi

	# Checking XdnsSsp PID
	XDNS_PID=`pidof CcspXdnsSsp`
	if [ "$XDNS_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : CcspXdnsSsp_process is not running, need restart"
		resetNeeded xdns CcspXdnsSsp

	fi

	# Checking CcspEthAgent PID
	ETHAGENT_PID=`pidof CcspEthAgent`
	if [ "$ETHAGENT_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : CcspEthAgent_process is not running, need restart"
		resetNeeded ethagent CcspEthAgent

	fi

	# Checking snmp subagent PID
	SNMP_PID=`pidof snmp_subagent`
	if [ "$SNMP_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
		resetNeeded snmp snmp_subagent 
	fi

	HOTSPOT_ENABLE=`dmcli eRT getv Device.DeviceInfo.X_COMCAST_COM_xfinitywifiEnable | grep value | cut -f3 -d : | cut -f2 -d" "`
	if [ "$HOTSPOT_ENABLE" = "true" ]
	then
	
		DHCP_ARP_PID=`pidof hotspot_arpd`
		if [ "$DHCP_ARP_PID" = "" ] && [ -f /tmp/hotspot_arpd_up ]; then
		     echo_t "RDKB_PROCESS_CRASHED : DhcpArp_process is not running, need restart"
		     resetNeeded "" hotspot_arpd 
		fi
	fi
	#Check dropbear is alive to do rsync/scp to/fro ATOM
	if [ "$ARM_INTERFACE_IP" != "" ]
	then
           DROPBEAR_ENABLE=`ps -w | grep dropbear | grep $ARM_INTERFACE_IP`
           if [ "$DROPBEAR_ENABLE" == "" ]
           then
                  echo_t "RDKB_PROCESS_CRASHED : rsync_dropbear_process is not running, need restart"
               dropbear -E -s -p $ARM_INTERFACE_IP:22 > /dev/null 2>&1
           fi
        fi

	# Checking lighttpd PID
	LIGHTTPD_PID=`pidof lighttpd`
	if [ "$LIGHTTPD_PID" = "" ]; then
		isPortKilled=`netstat -anp | grep 51515`
		if [ "$isPortKilled" != "" ]
		then
		    echo_t "Port 51515 is still alive. Killing processes associated to 51515"
		    fuser -k 51515/tcp
		fi
		echo_t "RDKB_PROCESS_CRASHED : lighttpd is not running, restarting it"
		#lighttpd -f $LIGHTTPD_CONF
		sh /etc/webgui.sh
	fi
	
# Checking syseventd PID
 	SYSEVENT_PID=`pidof syseventd`
	if [ "$SYSEVENT_PID" == "" ]
	then
		if [ ! -f "$SyseventdCrashed"  ]
		then
			echo_t "[RDKB_PROCESS_CRASHED] : syseventd is crashed, need to reboot the device in maintanance window." 
			touch $SyseventdCrashed
		fi
		rebootDeviceNeeded=1


	fi



# Checking whether brlan0 created properly , if not recreate it
lanSelfheal=`sysevent get lan_selfheal`
echo_t "[RDKB_SELFHEAL] : Value of lanSelfheal : $lanSelfheal"
if [ "$lanSelfheal" != "done" ]
then
        check_device_mode=`dmcli eRT getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode`
        check_param_get_succeed=`echo $check_device_mode | grep "Execution succeed"`
        if [ "$check_param_get_succeed" != "" ]
        then
            check_device_in_router_mode=`echo $check_param_get_succeed | grep router`
            if [ "$check_device_in_router_mode" != "" ]
            then
         	   check_if_brlan0_created=`ifconfig | grep brlan0`
		   check_if_brlan0_up=`ifconfig brlan0 | grep UP`
		   check_if_brlan0_hasip=`ifconfig brlan0 | grep "inet addr"`
		   if [ "$check_if_brlan0_created" = "" ] || [ "$check_if_brlan0_up" = "" ] || [ "$check_if_brlan0_hasip" = "" ]
		   then
			echo_t "[RDKB_PLATFORM_ERROR] : brlan0 is not completely up, setting event to recreate brlan0 interface"
			logNetworkInfo

			ipv4_status=`sysevent get ipv4_4-status`
			lan_status=`sysevent get lan-status`

			if [ "$lan_status" != "started" ]
			then
				if [ "$ipv4_status" = "" ] || [ "$ipv4_status" = "down" ]
				then
					echo_t "[RDKB_SELFHEAL] : ipv4_4-status is not set or lan is not started, setting lan-start event"
					sysevent set lan-start
					sleep 5
				fi
			fi

			sysevent set multinet-down 1
			sleep 5
			sysevent set multinet-up 1
			sleep 30
			sysevent set lan_selfheal done
		   fi

            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while fetching device mode "
        fi
else
	echo_t "[RDKB_SELFHEAL] : brlan0 already restarted. Not restarting again"
fi


	if [ "$WiFi_Flag" == "false" ]; then
        SSID_DISABLED=0
        BR_MODE=0
        ssidEnable=`dmcli eRT getv Device.WiFi.SSID.2.Enable`
        ssidExecution=`echo $ssidEnable | grep "Execution succeed"`
        if [ "$ssidExecution" != "" ]
        then
           isEnabled=`echo $ssidEnable | grep "false"`
           if [ "$isEnabled" != "" ]
           then
             SSID_DISABLED=1
             echo_t "[RDKB_SELFHEAL] : SSID 5GHZ is disabled"
           fi
        else
           destinationError=`echo $ssidEnable | grep "Can't find destination component"`
           if [ "$destinationError" != "" ]
           then
                echo_t "[RDKB_PLATFORM_ERROR] : Parameter cannot be found on WiFi subsystem"
           else
                echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Enable"
                echo "$ssidEnable"
           fi
           fi
        fi

        bridgeMode=`dmcli eRT getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode`
        # RDKB-6895
        bridgeSucceed=`echo $bridgeMode | grep "Execution succeed"`
        if [ "$bridgeSucceed" != "" ]
        then
           isBridging=`echo $bridgeMode | grep router`
           if [ "$isBridging" = "" ]
           then
               BR_MODE=1
               echo_t "[RDKB_SELFHEAL] : Device in bridge mode"
           fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking bridge mode."
	      echo_t "LanMode dmcli called failed with error $bridgeMode "
	       isBridging=`syscfg get bridge_mode`
		if [ "$isBridging" != "0" ]
		then
		       BR_MODE=1
		      echo_t "[RDKB_SELFHEAL] : Device in bridge mode"
		fi
	    pandm_timeout=`echo $bridgeMode | grep "$CCSP_ERR_TIMEOUT"`
	    pandm_notexist=`echo $bridgeMode | grep "$CCSP_ERR_NOT_EXIST"`
	    if [ "$pandm_timeout" != "" ] || [ "$pandm_notexist" != "" ]
	    then
		echo_t "[RDKB_PLATFORM_ERROR] : pandm parameter timed out or failed to return"
		cr_query=`dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name`
		cr_timeout=`echo $cr_query | grep "$CCSP_ERR_TIMEOUT"`
		cr_pam_notexist=`echo $cr_query | grep "$CCSP_ERR_NOT_EXIST"`
		if [ "$cr_timeout" != "" ] || [ "$cr_pam_notexist" != "" ]
		then
			echo_t "[RDKB_PLATFORM_ERROR] : pandm process is not responding. Restarting it"
			PANDM_PID=`pidof CcspPandMSsp`
			if [ "$PANDM_PID" != "" ]; then
				kill -9 $PANDM_PID
			fi
			rm -rf /tmp/pam_initialized
			resetNeeded pam CcspPandMSsp
		fi
	    fi

        fi

        # If bridge mode is not set and WiFI is not disabled by user,
        # check the status of SSID
        if [ "$WiFi_Flag" == "false" ]; then
        if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED -eq 0 ]
        then           
	    ssidStatus_5=`dmcli eRT getv Device.WiFi.SSID.2.Status`
            isExecutionSucceed=`echo $ssidStatus_5 | grep "Execution succeed"`
            if [ "$isExecutionSucceed" != "" ]
            then       
        
	        isUp=`echo $ssidStatus_5 | grep "Up"`
                if [ "$isUp" = "" ]
                then
                   # We need to verify if it was a dmcli crash or is WiFi really down
	           isDown=`echo $ssidStatus_5 | grep "Down"`
	           if [ "$isDown" != "" ]; then
                      echo_t "[RDKB_PLATFORM_ERROR] : 5G private SSID is off."
                   else
                      echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G status."
                      echo "$ssidStatus_5"
                   fi
                fi
            else
               echo_t "[RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 5G status."
	           echo "$ssidStatus_5"
            fi
        fi

        # Check the status if 2.4GHz Wifi SSID
        SSID_DISABLED_2G=0
        ssidEnable_2=`dmcli eRT getv Device.WiFi.SSID.1.Enable`
        ssidExecution_2=`echo $ssidEnable_2 | grep "Execution succeed"`

        if [ "$ssidExecution_2" != "" ]
        then
            isEnabled_2=`echo $ssidEnable_2 | grep "false"`
            if [ "$isEnabled_2" != "" ]
            then
               SSID_DISABLED_2G=1
               echo_t "[RDKB_SELFHEAL] : SSID 2.4GHZ is disabled"
            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G Enable"
			echo "$ssidEnable_2"
        fi

        # If bridge mode is not set and WiFI is not disabled by user,
        # check the status of SSID
        if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED_2G -eq 0 ]
        then
	        ssidStatus_2=`dmcli eRT getv Device.WiFi.SSID.1.Status`
            isExecutionSucceed_2=`echo $ssidStatus_2 | grep "Execution succeed"`
            if [ "$isExecutionSucceed_2" != "" ]
            then       
        
                isUp=`echo $ssidStatus_2 | grep "Up"`
                if [ "$isUp" = "" ]
                then
                    # We need to verify if it was a dmcli crash or is WiFi really down
	            isDown=`echo $ssidStatus_2 | grep "Down"`
	            if [ "$isDown" != "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : 2.4G private SSID (ath0) is off."
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G status."
		                echo "$ssidStatus_2"
                    fi
                fi
            else
               echo_t "[RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 2.4G status."
	           echo "$ssidStatus_2"
            fi
        fi
        fi
	FIREWALL_ENABLED=`syscfg get firewall_enabled`

	echo_t "[RDKB_SELFHEAL] : BRIDGE_MODE is $BR_MODE"
	echo_t "[RDKB_SELFHEAL] : FIREWALL_ENABLED is $FIREWALL_ENABLED"

	if [ $BR_MODE -eq 0 ] 
	then
		iptables-save -t nat | grep "A PREROUTING -i"
		if [ $? == 1 ]; then
		echo_t "[RDKB_PLATFORM_ERROR] : iptable corrupted."
		#sysevent set firewall-restart
		fi
     fi

#Logging to check the DHCP range corruption
    lan_ipaddr=`syscfg get lan_ipaddr`
    lan_netmask=`syscfg get lan_netmask`
    echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : lan_ipaddr = $lan_ipaddr lan_netmask = $lan_netmask"

    lost_and_found_enable=`syscfg get lost_and_found_enable`
    echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] :  lost_and_found_enable = $lost_and_found_enable"
    if [ "$lost_and_found_enable" == "true" ]
    then
        iot_ifname=`syscfg get iot_ifname`
        iot_dhcp_start=`syscfg get iot_dhcp_start`
        iot_dhcp_end=`syscfg get iot_dhcp_end`
        iot_netmask=`syscfg get iot_netmask`
        echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : DHCP server configuring for IOT iot_ifname = $iot_ifname "
        echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : iot_dhcp_start = $iot_dhcp_start iot_dhcp_end=$iot_dhcp_end iot_netmask=$iot_netmask"
    fi

#Checking whether dnsmasq is running or not
   DNS_PID=`pidof dnsmasq`
   if [ "$DNS_PID" == "" ]
   then
		 echo_t "[RDKB_SELFHEAL] : dnsmasq is not running"   
   else
	     brlan0up=`cat /var/dnsmasq.conf | grep brlan0`

	     IsAnyOneInfFailtoUp=0	

	     if [ $BR_MODE -eq 0 ]
	     then
			if [ "$brlan0up" == "" ]
			then
			    echo_t "[RDKB_SELFHEAL] : brlan0 info is not availble in dnsmasq.conf"
			    IsAnyOneInfFailtoUp=1
			fi
	     fi


	     if [ ! -f /tmp/dnsmasq_restarted_via_selfheal ] 
	     then
		     if [ $IsAnyOneInfFailtoUp -eq 1 ]
		     then
				 touch /tmp/dnsmasq_restarted_via_selfheal

		         echo_t "[RDKB_SELFHEAL] : dnsmasq.conf is."   
			 	 echo "`cat /var/dnsmasq.conf`"

				 echo_t "[RDKB_SELFHEAL] : Setting an event to restart dnsmasq"
		         sysevent set dhcp_server-stop
		         sysevent set dhcp_server-start
		     fi
	     fi
   fi

#Checking dibbler server is running or not RDKB_10683
	DIBBLER_PID=`pidof dibbler-server`
	if [ "$DIBBLER_PID" = "" ]; then

#		DHCPV6C_ENABLED=`sysevent get dhcpv6c_enabled`
#		if [ "$BR_MODE" == "0" ] && [ "$DHCPV6C_ENABLED" == "1" ]; then
		if [ "$BR_MODE" == "0" ]; then
                        DHCPv6EnableStatus=`syscfg get dhcpv6s00::serverenable`
                        if [ "$IS_BCI" = "yes" ] && [ "0" = "$DHCPv6EnableStatus" ]; then
                           echo_t "DHCPv6 Disabled. Restart of Dibbler process not Required"
                        else
			   echo_t "RDKB_PROCESS_CRASHED : Dibbler is not running, restarting the dibbler"
			   if [ -f "/etc/dibbler/server.conf" ]
			   then
				dibbler-server stop
				sleep 2
				dibbler-server start
			    else
				echo_t "RDKB_PROCESS_CRASHED : Server.conf file not present, Cannot restart dibbler"
			    fi
			fi
		fi
	fi

#Checking the zebra is running or not
	ZEBRA_PID=`pidof zebra`
	if [ "$ZEBRA_PID" = "" ]; then
		if [ "$BR_MODE" == "0" ]; then
			   echo_t "RDKB_PROCESS_CRASHED : zebra is not running, restarting the zebra"
			   /etc/utopia/registration.d/20_routing restart
			   sysevent set zebra-restart
		fi
	fi


# Checking for WAN_INTERFACE ipv6 address
DHCPV6_ERROR_FILE="/tmp/.dhcpv6SolicitLoopError"
WAN_STATUS=`sysevent get wan-status`
WAN_IPv4_Addr=`ifconfig $WAN_INTERFACE | grep inet | grep -v inet6`
DHCPV6_HANDLER="/etc/utopia/service.d/service_dhcpv6_client.sh"

if [ -f "$DHCPV6_ERROR_FILE" ] && [ "$WAN_STATUS" = "started" ] && [ "$WAN_IPv4_Addr" != "" ]
then
	          isIPv6=`ifconfig $WAN_INTERFACE | grep inet6 | grep "Scope:Global"`
		echo_t "isIPv6 = $isIPv6"
        	 if [ "$isIPv6" == "" ]
		 then
			echo_t "[RDKB_SELFHEAL] : $DHCPV6_ERROR_FILE file present and $WAN_INTERFACE ipv6 address is empty, restarting ti_dhcp6c"
			rm -rf $DHCPV6_ERROR_FILE
			sh $DHCPV6_HANDLER disable
			sleep 2
			sh $DHCPV6_HANDLER enable
           	fi 
fi


if [ "$WAN_STATUS" = "started" ];then

	wan_dhcp_client_v4=1
	wan_dhcp_client_v6=1

	check_wan_dhcp_client_v4=`ps w | grep udhcpc | grep erouter`
	check_wan_dhcp_client_v6=`ps w | grep dibbler-client | grep -v grep`

	if [ "x$check_wan_dhcp_client_v4" = "x" ]; then
		echo_t "RDKB_PROCESS_CRASHED : DHCP Client for v4 is not running, need restart "
		wan_dhcp_client_v4=0
	fi

	if [ "x$check_wan_dhcp_client_v6" = "x" ]; then
		echo_t "RDKB_PROCESS_CRASHED : DHCP Client for v6 is not running, need restart"
		wan_dhcp_client_v6=0
	fi

	DHCP_STATUS_query=`dmcli eRT getv Device.DHCPv4.Client.1.DHCPStatus`
	DHCP_STATUS_execution=`echo $DHCP_STATUS_query | grep "Execution succeed"`
	DHCP_STATUS=`echo "$DHCP_STATUS_query" | grep value | cut -f3 -d : | awk '{print $1}'`

	if [ "$DHCP_STATUS_execution" != "" ] && [ "$DHCP_STATUS" != "Bound" ] ; then

		echo_t "DHCP_CLIENT : DHCPStatusValue is $DHCP_STATUS"
		if [ $wan_dhcp_client_v4 -eq 0 ] || [ $wan_dhcp_client_v6 -eq 0 ]; then
			echo_t "DHCP_CLIENT : DHCPStatus is not Bound, restarting WAN"
			sh /etc/utopia/service.d/service_wan.sh wan-stop
			sh /etc/utopia/service.d/service_wan.sh wan-start
			wan_dhcp_client_v4=1
			wan_dhcp_client_v6=1
		fi
	fi

	if [ $wan_dhcp_client_v4 -eq 0 ];
	then
		V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
		echo_t "DHCP_CLIENT : Restarting DHCP Client for v4"
		eval "$V4_EXEC_CMD"
		sleep 5
		wan_dhcp_client_v4=1
	fi

	if [ $wan_dhcp_client_v6 -eq 0 ];
	then
		echo_t "DHCP_CLIENT : Restarting DHCP Client for v6"
		/etc/dibbler/dibbler-init.sh
		sleep 2
		/usr/sbin/dibbler-client start
		wan_dhcp_client_v6=1
	fi

fi

if [ "$rebootDeviceNeeded" -eq 1 ]
then

	checkMaintenanceWindow
	if [ $reb_window -eq 0 ]
	then
		echo_t "Maintanance window for the current day is over , unit will be rebooted in next Maintanance window "
	else
		#Check if we have already flagged reboot is needed
		if [ ! -e $FLAG_REBOOT ]
		then
			echo_t "rebootDeviceNeeded"
			sh /etc/calc_random_time_to_reboot_dev.sh "" &
			touch $FLAG_REBOOT
		else
			echo_t "Already waiting for reboot"
		fi					
	fi
fi
