#! /bin/sh

UTOPIA_PATH="/etc/utopia/service.d"
TAD_PATH="/usr/ccsp/tad"
RDKLOGGER_PATH="/rdklogger"

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi

if [ -f /usr/ccsp/tad/tadUtilsDevice.sh ]
then
    source /usr/ccsp/tad/tadUtilsDevice.sh
fi

ping_failed=0
ping_success=0
SyseventdCrashed="/rdklogs/syseventd_crashed"
PING_PATH="/usr/sbin"
WAN_INTERFACE="erouter0"
source $UTOPIA_PATH/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source $TAD_PATH/corrective_action.sh

rebootDeviceNeeded=0


LIGHTTPD_CONF="/var/lighttpd.conf"

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
			#We'll set the reason only if webpa reconnect is not due to DNS resolve
			syscfg get X_RDKCENTRAL-COM_LastReconnectReason | grep "Dns_Res_webpa_reconnect"
			if [ $? != 0 ]; then
				echo "setting reconnect reason from sysd_task_health_monitor.sh"
				echo "[`getDateTime`] Setting Last reconnect reason"
				syscfg set X_RDKCENTRAL-COM_LastReconnectReason WebPa_crash
				result=`echo $?`
				if [ "$result" != "0" ]
				then
					echo "SET for Reconnect Reason failed"
				fi
				syscfg commit
				result=`echo $?`
				if [ "$result" != "0" ]
				then
					echo "Commit for Reconnect Reason failed"
				fi
				echo "[`getDateTime`] SET succeeded"
			fi
			resetNeeded webpa webpa
		fi
	
	fi

	#Checking dropbear PID 
	DROPBEAR_PID=`pidof dropbear`
	if [ "$DROPBEAR_PID" = "" ]; then
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : dropbear_process is not running, restarting it"
		sh /etc/utopia/service.d/service_sshd.sh sshd-restart &
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
		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : lighttpd is not running, restarting it"
		sh /etc/webgui.sh
	fi
# Checking syseventd PID
 	SYSEVENT_PID=`pidof syseventd`
	if [ "$SYSEVENT_PID" == "" ]
	then
		if [ ! -f "$SyseventdCrashed"  ]
		then
			echo "[`getDateTime`] [RDKB_PROCESS_CRASHED] : syseventd is crashed, need to reboot the device in maintanance window." 
			touch $SyseventdCrashed
		fi
		rebootDeviceNeeded=1
	fi
	ifconfig | grep brlan1
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : brlan1 interface is not up, need to reboot the unit" 
		rebootNeededforbrlan1=1
		rebootDeviceNeeded=1
	else
		ifconfig brlan1 | grep "inet addr"
		if [ $? == 1 ]; then
			echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : brlan1 interface is not having ip, creating it"
			sysevent set multinet_2-status stopped
		    deviceBrlan1Up

			ifconfig brlan1 | grep "inet addr"
			if [ $? == 1 ]; then
				echo "[RDKB_PLATFORM_ERROR] : brlan1 is not created at First Retry, try again after 2 sec"
				sleep 2
				sysevent set multinet_2-status stopped
				deviceBrlan1Up

				ifconfig brlan1 | grep "inet addr"
				if [ $? == 1 ]; then
					echo "[RDKB_PLATFORM_ERROR] : brlan1 is not created after Second Retry, no more retries !!!"
				fi
			else
				echo "[RDKB_PLATFORM_ERROR] : brlan1 Created at First Retry itself"
			fi
		fi
	fi

	ifconfig | grep brlan0
	if [ $? == 1 ]; then
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : brlan0 interface is not up" 
		echo "[`getDateTime`] RDKB_REBOOT : brlan0 interface is not up, rebooting the device"
		reason="brlan0_down"
		rebootCount=1
		rebootNeeded RM "" $reason $rebootCount
	else
		ifconfig brlan0 | grep "inet addr"
		if [ $? == 1 ]; then
			echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : brlan0 interface is not having ip, creating it"
			sysevent set multinet_1-status stopped
		    deviceBrlan0Up

			ifconfig brlan0 | grep "inet addr"
			if [ $? == 1 ]; then
				echo "[RDKB_PLATFORM_ERROR] : brlan0 is not created at First Retry, try again after 2 sec"
				sleep 2
				sysevent set multinet_1-status stopped
				deviceBrlan0Up

				ifconfig brlan0 | grep "inet addr"
				if [ $? == 1 ]; then
					echo "[RDKB_PLATFORM_ERROR] : brlan0 is not created after Second Retry, no more retries !!!"
				fi
			else
				echo "[RDKB_PLATFORM_ERROR] : brlan0 Created at First Retry itself"
			fi
		fi
	fi

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
		 echo "[`getDateTime`] [RDKB_SELFHEAL] : SSID 5GHZ is disabled"
	   fi
	else
	   destinationError=`echo $ssidEnable | grep "Can't find destination component"`
       if [ "$destinationError" != "" ]
       then
            echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Parameter cannot be found on WiFi subsystem"
       else
            echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Enable"            
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
               echo "[`getDateTime`] [RDKB_SELFHEAL] : Device in bridge mode"
           fi
        else
            echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Something went wrong while checking bridge mode."

	    pandm_timeout=`echo $bridgeMode | grep "CCSP_ERR_TIMEOUT"`
	    if [ "$pandm_timeout" != "" ]; then
			echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : pandm parameter time out"
			cr_query=`dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name`
			cr_timeout=`echo $cr_query | grep "CCSP_ERR_TIMEOUT"`
			if [ "$cr_timeout" != "" ]; then
				echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : pandm process is not responding. Restarting it"
				PANDM_PID=`pidof CcspPandMSsp`
				rm -rf /tmp/pam_initialized
				systemctl restart CcspPandMSsp.service
			fi
	    fi
        fi

	# If bridge mode is not set and WiFI is not disabled by user,
	# check the status of SSID
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
				  echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : 5G private SSID (ath1) is off."
			   else
				  echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G status."                      
			   fi
			fi
		else
		   echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 5G status."
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
		   echo "[`getDateTime`] [RDKB_SELFHEAL] : SSID 2.4GHZ is disabled"
		fi
	else
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G Enable"            
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
					echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : 2.4G private SSID (ath0) is off."
				else
					echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G status."                      
				fi
			fi
		else
		   echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 2.4G status."
		fi
	fi
        
	FIREWALL_ENABLED=`syscfg get firewall_enabled`

	echo "[`getDateTime`] [RDKB_SELFHEAL] : BRIDGE_MODE is $BR_MODE"
    echo "[`getDateTime`] [RDKB_SELFHEAL] : FIREWALL_ENABLED is $FIREWALL_ENABLED"

	if [ $BR_MODE -eq 0 ] 
	then
		iptables-save -t nat | grep "A PREROUTING -i"
		if [ $? == 1 ]; then
		echo "[`getDateTime`] [RDKB_PLATFORM_ERROR] : iptable corrupted."
		#sysevent set firewall-restart
		fi
     fi

#Checking whether dnsmasq is running or not
   DNS_PID=`pidof dnsmasq`
   if [ "$DNS_PID" == "" ]
   then
		 echo_t "[RDKB_SELFHEAL] : dnsmasq is not running"   
   else
	     brlan1up=`cat /var/dnsmasq.conf | grep brlan1`
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

	     if [ "$brlan1up" == "" ]
	     then
	         echo_t "[RDKB_SELFHEAL] : brlan1 info is not availble in dnsmasq.conf"
			 IsAnyOneInfFailtoUp=1
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
	
	checkIfDnsmasqIsZombie=`ps | grep dnsmasq | grep "Z" | awk '{ print $1 }'`
	if [ "$checkIfDnsmasqIsZombie" != "" ] ; then
		confirmZombie=`grep "State:" /proc/$checkIfDnsmasqIsZombie/status | grep -i "zombie"`
		if [ "$confirmZombie" != "" ] ; then
			echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, restarting dnsmasq"
			kill -9 `pidof dnsmasq`
			sysevent set dhcp_server-stop
			sysevent set dhcp_server-start
		fi
	fi

   fi

#Checking dibbler server is running or not RDKB_10683
	DIBBLER_PID=`pidof dibbler-server`
	if [ "$DIBBLER_PID" = "" ]; then

		DHCPV6C_ENABLED=`sysevent get dhcpv6c_enabled`
		if [ "$BR_MODE" == "0" ] && [ "$DHCPV6C_ENABLED" == "1" ]; then

			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : Dibbler is not running, restarting the dibbler"
			if [ -f "/etc/dibbler/server.conf" ]
			then
				dibbler-server stop
				sleep 2
				dibbler-server start
			else
				echo "[`getDateTime`] RDKB_PROCESS_CRASHED : Server.conf file not present, Cannot restart dibbler"
			fi
		fi
	fi

#Checking the zebra is running or not
	ZEBRA_PID=`pidof zebra`
	if [ "$ZEBRA_PID" = "" ]; then
		if [ "$BR_MODE" == "0" ]; then

			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : zebra is not running, restarting the zebra"
			sysevent set zebra-restart
		fi
	fi

	
	#All CCSP Processes Now running on Single Processor. Add those Processes to Test & Diagnostic 
	# Checking for WAN_INTERFACE ipv6 address
	DHCPV6_ERROR_FILE="/tmp/.dhcpv6SolicitLoopError"
	WAN_STATUS=`sysevent get wan-status`
	WAN_IPv4_Addr=`ifconfig $WAN_INTERFACE | grep inet | grep -v inet6`
	DHCPV6_HANDLER="/etc/utopia/service.d/service_dhcpv6_client.sh"

	if [ -f "$DHCPV6_ERROR_FILE" ] && [ "$WAN_STATUS" = "started" ] && [ "$WAN_IPv4_Addr" != "" ]
	then
		          isIPv6=`ifconfig $WAN_INTERFACE | grep inet6 | grep "Scope:Global"`
			echo "isIPv6 = $isIPv6"
	        	 if [ "$isIPv6" == "" ]
			 then
				echo "[`getDateTime`] [RDKB_SELFHEAL] : $DHCPV6_ERROR_FILE file present and $WAN_INTERFACE ipv6 address is empty, restarting ti_dhcp6c"
				rm -rf $DHCPV6_ERROR_FILE
				sh $DHCPV6_HANDLER disable
				sleep 2
				sh $DHCPV6_HANDLER enable
	           	fi 
	fi
	
	if [ "$WAN_STATUS" = "started" ];then
		wan_dhcp_client_v4=1
		wan_dhcp_client_v6=1

		if [[ ( "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor" ) || ( "$BOX_TYPE" = "TCCBR" ) ]]; then
			check_wan_dhcp_client_v4=`ps w | grep udhcpc | grep erouter`
			check_wan_dhcp_client_v6=`ps w | grep dibbler-client | grep -v grep`
		else
			dhcp_cli_output=`ps w | grep ti_ | grep erouter0`
			check_wan_dhcp_client_v4=`echo $dhcp_cli_output | grep ti_udhcpc`
			check_wan_dhcp_client_v6=`echo $dhcp_cli_output | grep ti_dhcp6c`
		fi

		if [ "x$check_wan_dhcp_client_v4" = "x" ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : DHCP Client for v4 is not running, need restart "
			wan_dhcp_client_v4=0
		fi

		if [ "x$check_wan_dhcp_client_v6" = "x" ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : DHCP Client for v6 is not running, need restart"
			wan_dhcp_client_v6=0
		fi

		DHCP_STATUS=`dmcli eRT getv Device.DHCPv4.Client.1.DHCPStatus | grep value | cut -f3 -d : | cut -f2 -d" "`

		if [ "$DHCP_STATUS" = "Rebinding" ] ; then
			if [ $wan_dhcp_client_v4 -eq 0 ] || [ $wan_dhcp_client_v6 -eq 0 ]; then
				echo "[`getDateTime`] DHCP_CLIENT : DHCPStatus is rebinding, restarting WAN"
				sh /etc/utopia/service.d/service_wan.sh wan-stop
				sh /etc/utopia/service.d/service_wan.sh wan-start
				wan_dhcp_client_v4=1
				wan_dhcp_client_v6=1
			fi
		fi

		if [ $wan_dhcp_client_v4 -eq 0 ];
		then
			if [[ ( "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor" ) || ( "$BOX_TYPE" = "TCCBR" ) ]]; then
				V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
			else
				DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
				V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 1"
			fi

			echo "[`getDateTime`] DHCP_CLIENT : Restarting DHCP Client for v4"
			eval "$V4_EXEC_CMD"
			sleep 5
			wan_dhcp_client_v4=1
		fi

		if [ $wan_dhcp_client_v6 -eq 0 ];
		then
			echo "[`getDateTime`] DHCP_CLIENT : Restarting DHCP Client for v6"
			if [[ ( "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor" ) || ( "$BOX_TYPE" = "TCCBR" ) ]]; then
				/etc/dibbler/dibbler-init.sh
				sleep 2
				/usr/sbin/dibbler-client start
			else
				sh $DHCPV6_HANDLER disable
				sleep 2
				sh $DHCPV6_HANDLER enable
			fi
			wan_dhcp_client_v6=1
		fi

	fi
	
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
					echo "[`getDateTime`] RDKB_SELFHEAL : Ping to Peer IP is success"
					break
				else
					ping_failed=1
				fi
			else
				ping_failed=1
			fi

			if [ "$ping_failed" -eq 1 ] && [ "$loop" -lt 3 ]
			then
				echo "[`getDateTime`] RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
			else
				echo "[`getDateTime`] RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
				echo "[`getDateTime`] RDKB_REBOOT : Peer is not up ,Rebooting device "
				echo "[`getDateTime`] Setting Last reboot reason Peer_down"
                reason="Peer_down"
                rebootCount=1
                rebootNeeded RM "" $reason $rebootCount

			fi
			loop=$((loop+1))
			sleep 5
		done
	else
	   echo "[`getDateTime`] RDKB_SELFHEAL : ping_peer command not found"
	fi
	
	if [ -f $PING_PATH/arping_peer ]
	then
		$PING_PATH/arping_peer
	else
	   echo "[`getDateTime`] RDKB_SELFHEAL : arping_peer command not found"
	fi
	
	if [ "$rebootDeviceNeeded" -eq 1 ]
	then
	
		if [ "$UTC_ENABLE" == "true" ]
		then
			cur_hr=`LTime H`
			cur_min=`LTime M`
		else
			cur_hr=`date +"%H"`
			cur_min=`date +"%M"`
		fi
		
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
