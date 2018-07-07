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
grePrefix="gretap0"
brlanPrefix="brlan"
l2sd0Prefix="l2sd0"

if [ -f /etc/device.properties ]
then
    source /etc/device.properties
fi
source /etc/log_timestamp.sh

if [ -f /etc/mount-utils/getConfigFile.sh ];then
     . /etc/mount-utils/getConfigFile.sh
fi

if [[ "$MODEL_NUM" = "DPC3939" || "$MODEL_NUM" = "DPC3941" ]]; then
	ADVSEC_PATH="/tmp/cujo_dnld/usr/ccsp/advsec/usr/libexec/advsec.sh"
else
	ADVSEC_PATH="/usr/ccsp/advsec/usr/libexec/advsec.sh"
fi

ping_failed=0
ping_success=0
SyseventdCrashed="/rdklogs/syseventd_crashed"
SNMPMASTERCRASHED="/tmp/snmp_cm_crashed"
PING_PATH="/usr/sbin"
WAN_INTERFACE="erouter0"
PEER_COMM_ID="/tmp/elxrretyt.swr"

brlan1_firewall="/tmp/brlan1_firewall_rule_validated"
if [ ! -f /usr/bin/GetConfigFile ];then
    echo "Error: GetConfigFile Not Found"
    exit
fi
IDLE_TIMEOUT=30
source $UTOPIA_PATH/log_env_var.sh


exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

source $TAD_PATH/corrective_action.sh

if [ -f $ADVSEC_PATH ]
then
    source $ADVSEC_PATH
fi
rebootDeviceNeeded=0
reboot_needed_atom_ro=0

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
		psm_name_timeout=`echo $psm_name | grep "CCSP_ERR_TIMEOUT"`
		psm_name_notexist=`echo $psm_name | grep "CCSP_ERR_NOT_EXIST"`
		if [ "$psm_name_timeout" != "" ] || [ "$psm_name_notexist" != "" ]; then
			psm_health=`dmcli eRT getv com.cisco.spvtg.ccsp.psm.Health`
			psm_health_timeout=`echo $psm_health | grep "CCSP_ERR_TIMEOUT"`
			psm_health_notexist=`echo $psm_health | grep "CCSP_ERR_NOT_EXIST"`
			if [ "$psm_health_timeout" != "" ] || [ "$psm_health_notexist" != "" ]; then
				echo_t "RDKB_PROCESS_CRASHED : PSM_process is in hung state, need restart"
				kill -9 `pidof PsmSsp`
				resetNeeded psm PsmSsp
			fi
		fi
	fi

###########################################

if [ "$MULTI_CORE" = "yes" ]; then
	if [ "$CORE_TYPE" = "arm" ]; then
		# Checking logbackup PID
		LOGBACKUP_PID=`pidof logbackup`
		if [ "$LOGBACKUP_PID" == "" ]; then
			echo_t "RDKB_PROCESS_CRASHED : logbackup process is not running, need restart"
			/usr/bin/logbackup &
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
					echo_t "RDKB_SELFHEAL : Ping to Peer IP is success"
					break
				else
                                        echo_t "[RDKB_PLATFORM_ERROR] : ATOM interface is not reachable"
					ping_failed=1
				fi
			else
                                if [ "$BOX_TYPE" = "XB3" ]; then
                                        check_if_l2sd0_500_up=`ifconfig l2sd0.500 | grep UP `
                                        check_if_l2sd0_500_ip=`ifconfig l2sd0.500 | grep inet `
                                        if [ "$check_if_l2sd0_500_up" = "" ] || [ "$check_if_l2sd0_500_ip" = "" ]
                                        then
                                                echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is not up, setting to recreate interface"                                     
                                                rpc_ifconfig l2sd0.500 >/dev/null 2>&1
						sleep 3
                                        fi
                                        PING_RES=`ping_peer`
                                        CHECK_PING_RES=`echo $PING_RES | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1`
                                        if [ "$CHECK_PING_RES" != "" ]
                                        then
                                                if [ "$CHECK_PING_RES" -ne 100 ]
                                                then
                                                echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is up,Ping to Peer IP is success"
                                                break
                                                fi
                                        fi
                                fi
				ping_failed=1
			fi
			
			if [ "$ping_failed" -eq 1 ] && [ "$loop" -lt 3 ]
			then
				echo_t "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
				echo "RDKB_SELFHEAL : Ping command output is $PING_RES"
			else
				echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
				echo "RDKB_SELFHEAL : Ping command output is $PING_RES"
				echo_t "RDKB_REBOOT : Peer is not up ,Rebooting device "
				#echo_t " RDKB_SELFHEAL : Setting Last reboot reason as Peer_down"
				reason="Peer_down"
				rebootCount=1
				#setRebootreason $reason $rebootCount
				rebootNeeded RM "" $reason $rebootCount

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
else
        echo_t "RDKB_SELFHEAL : MULTI_CORE is not defined as yes. Define it as yes if it's a multi core device."
fi	
##################################
	if [ "$BOX_TYPE" = "XB3" ]; then
		  wifi_check=`dmcli eRT getv Device.WiFi.SSID.1.Enable`
		  wifi_timeout=`echo $wifi_check | grep "CCSP_ERR_TIMEOUT"`
		  if [ "$wifi_timeout" != "" ]; then
				  echo_t "[RDKB_SELFHEAL] : Wifi query timeout"
		  fi

                  
          GetConfigFile $PEER_COMM_ID
		  SSH_ATOM_TEST=$(ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ATOM_IP exit 2>&1)
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
							  #setRebootreason $reason $rebootCount
							  rebootNeeded $reason "" $reason $rebootCount
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

		### SNMPv3 agent self-heal ####
                  SNMPv3_PID=`pidof snmpd`
                  if [ "$SNMPv3_PID" == "" ] && [ "x$ENABLE_SNMPv3" == "xtrue" ]; then
                      # Restart disconnected master and agent
                      v3AgentPid=`ps -ww | grep cm_snmp_ma_2 | grep -v grep | tr -s ' ' | cut -d ' ' -f2`
                      if [ ! -z "$v3AgentPid" ]; then
                          kill -9 $v3AgentPid
                      fi
                      if [ -f /tmp/snmpd.conf ]; then
                          rm -f /tmp/snmpd.conf
                      fi
                      if [ -f /lib/rdk/run_snmpv3_master.sh ]; then
                          sh /lib/rdk/run_snmpv3_master.sh &
                      fi
                  fi

	fi
###########################################
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
				#echo_t "Setting Last reboot reason"
				reason="CR_crash"
				rebootCount=1
				#setRebootreason $reason $rebootCount
				echo_t "SET succeeded"
				echo_t "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"
				touch $HAVECRASH
				rebootNeeded RM "CR" $reason $rebootCount
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
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : MTA_process is not running, restarting it"
		echo_t "RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
		resetNeeded mta CcspMtaAgentSsp

	fi

	# Checking CM's PID
	CM_PID=`pidof CcspCMAgentSsp`
	if [ "$CM_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : CM_process is not running, restarting it"
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

	# Checking TR69's PID
	TR69_PID=`pidof CcspTr069PaSsp`
	if [ "$TR69_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TR69_process is not running, need to reboot the unit"
#		vendor=`getVendorName`
#		modelName=`getModelName`
#		CMMac=`getCMMac`
#		timestamp=`getDate`
#		echo "[`getDateTime`] Setting Last reboot reason"
#		dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string TR69_crash
#		echo "[`getDateTime`] SET succeeded"
#		echo "[`getDateTime`] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspTr069PaSsp process died,need reboot"

#		touch $HAVECRASH
#		rebootNeeded RM "TR69"
		echo_t "RDKB_PROCESS_CRASHED : TR69_process is not running, need restart"
		resetNeeded TR69 CcspTr069PaSsp
	fi

	# Checking Test adn Daignostic's PID
	TandD_PID=`pidof CcspTandDSsp`
	if [ "$TandD_PID" = "" ]; then
#		echo "[`getDateTime`] RDKB_PROCESS_CRASHED : TandD_process is not running, restarting it"
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
	SNMP_PID=`ps -ww | grep snmp_subagent | grep -v cm_snmp_ma_2 | grep -v grep | awk '{print $1}'`
	if [ "$SNMP_PID" = "" ]; then
		if [ -f /tmp/.snmp_agent_restarting ]; then
			echo_t "[RDKB_SELFHEAL] : snmp process is restarted through maintanance window"
		else
			echo_t "RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
			resetNeeded snmp snmp_subagent 
		fi
	fi

	# Checking CcspMoCA PID
	MOCA_PID=`pidof CcspMoCA`
	if [ "$MOCA_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : CcspMoCA process is not running, need restart"
		resetNeeded moca CcspMoCA 
	fi

	HOMESEC_PID=`pidof CcspHomeSecurity`
	if [ "$HOMESEC_PID" = "" ]; then
		echo_t "RDKB_PROCESS_CRASHED : HomeSecurity_process is not running, need restart"
		resetNeeded "" CcspHomeSecurity 
	fi

	DEVICE_FINGERPRINT_VALUE=`syscfg get Advsecurity_DeviceFingerPrint`
	if [ "$DEVICE_FINGERPRINT_VALUE" = "1" ] ; then
	        DEVICE_FINGERPRINT_ENABLE=true
	else
	        DEVICE_FINGERPRINT_ENABLE=false
	fi

	advsec_bridge_mode=`syscfg get bridge_mode`
	if [ "$DEVICE_FINGERPRINT_ENABLE" = "true" ]  && [ "$advsec_bridge_mode" != "2" ]; then

		if [ -f $ADVSEC_PATH ]
		then
			# CcspAdvSecurity
			ADV_PID=`pidof CcspAdvSecuritySsp`
			if [ "$ADV_PID" = "" ] ; then
				echo_t "RDKB_PROCESS_CRASHED : CcspAdvSecurity_process is not running, need restart"
				resetNeeded advsec CcspAdvSecuritySsp
			else
				if [ ! -f $ADVSEC_INITIALIZING ]
				then
					ADV_AG_PID=`advsec_is_alive agent`
					if [ "$ADV_AG_PID" = "" ] ; then
						echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Agent process is not running, need restart"
						resetNeeded advsec_bin AdvSecurityAgent
					fi
					ADV_DHCP_PID=`advsec_is_alive dhcpcap`
					if [ "$ADV_DHCP_PID" = "" ] ; then
						echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Dhcpcap process is not running, need restart"
						resetNeeded advsec_bin AdvSecurityDhcp
					fi
					if [ ! -f "$DAEMONS_HIBERNATING" ] ; then
						ADV_DNS_PID=`advsec_is_alive dnscap`
						if [ "$ADV_DNS_PID" = "" ] ; then
							echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Dnscap process is not running, need restart"
							resetNeeded advsec_bin AdvSecurityDns
						fi
						ADV_MDNS_PID=`advsec_is_alive mdnscap`
						if [ "$ADV_MDNS_PID" = "" ] ; then
							echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Mdnscap process is not running, need restart"
							resetNeeded advsec_bin AdvSecurityMdns
						fi
						ADV_P0F_PID=`advsec_is_alive p0f`
						if [ "$ADV_P0F_PID" = "" ] ; then
							echo_t "RDKB_PROCESS_CRASHED : AdvSecurity PoF process is not running, need restart"
							resetNeeded advsec_bin AdvSecurityPof
						fi
					fi
					ADV_SCAN_PID=`advsec_is_alive scannerd`
					if [ "$ADV_SCAN_PID" = "" ] ; then
						echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Scanner process is not running, need restart"
						resetNeeded advsec_bin AdvSecurityScanner
					fi
					if [ -e ${SAFEBRO_ENABLE} ] ; then
						ADV_SB_PID=`advsec_is_alive threatd`
						if [ "$ADV_SB_PID" = "" ] ; then
							echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Threat process is not running, need restart"
							resetNeeded advsec_bin AdvSecurityThreat
						fi
					fi
					if [ -e ${SOFTFLOWD_ENABLE} ] ; then
						ADV_SF_PID=`advsec_is_alive softflowd`
						if [ "$ADV_SF_PID" = "" ] ; then
							echo_t "RDKB_PROCESS_CRASHED : AdvSecurity Softflowd process is not running, need restart"
							resetNeeded advsec_bin AdvSecuritySoftflowd
						fi
					fi
				fi
			fi
		else
			if [[ "$MODEL_NUM" = "DPC3939" || "$MODEL_NUM" = "DPC3941" ]]; then
				/usr/sbin/cujo_download.sh &
			fi
		fi
	fi
	
	HOTSPOT_ENABLE=`dmcli eRT getv Device.DeviceInfo.X_COMCAST_COM_xfinitywifiEnable | grep value | cut -f3 -d : | cut -f2 -d" "`
	if [ "$HOTSPOT_ENABLE" = "true" ]
	then
	
		DHCP_ARP_PID=`pidof hotspot_arpd`
		if [ "$DHCP_ARP_PID" = "" ] && [ -f /tmp/hotspot_arpd_up ]; then
		     echo_t "RDKB_PROCESS_CRASHED : DhcpArp_process is not running, need restart"
		     resetNeeded "" hotspot_arpd 
		fi

		#When Xfinitywifi is enabled, l2sd0.102 and l2sd0.103 should be present.
		#If they are not present below code shall re-create them
		#l2sd0.102 case , also adding a strict rule that they are up, since some
                #devices we observed l2sd0 not up
		ifconfig | grep l2sd0.102
		if [ $? == 1 ]; then
		     echo_t "XfinityWifi is enabled, but l2sd0.102 interface is not created try creating it" 
		     sysevent set multinet_3-status stopped
		     $UTOPIA_PATH/service_multinet_exec multinet-start 3
                     ifconfig l2sd0.102 up
		     ifconfig | grep l2sd0.102
		     if [ $? == 1 ]; then
		       echo "l2sd0.102 is not created at First Retry, try again after 2 sec"
		       sleep 2
		       sysevent set multinet_3-status stopped
		       $UTOPIA_PATH/service_multinet_exec multinet-start 3
                       ifconfig l2sd0.102 up
		       ifconfig | grep l2sd0.102
		       if [ $? == 1 ]; then
		          echo "[RDKB_PLATFORM_ERROR] : l2sd0.102 is not created after Second Retry, no more retries !!!"
		       fi
		     else
		       echo "[RDKB_PLATFORM_ERROR] : l2sd0.102 created at First Retry itself"
		     fi
		else
		   echo "XfinityWifi is enabled and l2sd0.102 is present"  
		fi

		#l2sd0.103 case
		ifconfig | grep l2sd0.103
		if [ $? == 1 ]; then
		   echo_t "XfinityWifi is enabled, but l2sd0.103 interface is not created try creatig it" 
		   sysevent set multinet_4-status stopped
		   $UTOPIA_PATH/service_multinet_exec multinet-start 4
                   ifconfig l2sd0.103 up
		   ifconfig | grep l2sd0.103
		   if [ $? == 1 ]; then
		      echo "l2sd0.103 is not created at First Retry, try again after 2 sec"
		      sleep 2
		      sysevent set multinet_4-status stopped
		      $UTOPIA_PATH/service_multinet_exec multinet-start 4
                      ifconfig l2sd0.103 up
		      ifconfig | grep l2sd0.103
		      if [ $? == 1 ]; then
		         echo "[RDKB_PLATFORM_ERROR] : l2sd0.103 is not created after Second Retry, no more retries !!!"
		      fi
		   else
		        echo "[RDKB_PLATFORM_ERROR] : l2sd0.103 created at First Retry itself"
		   fi
		else
		   echo "Xfinitywifi is enabled and l2sd0.103 is present"
		fi
                
                #RDKB-16889: We need to make sure Xfinity hotspot Vlan IDs are attached to the bridges
 		#if found not attached , then add the device to bridges
                for index in 2 3 4 5
	        do
                    grePresent=`ifconfig -a | grep $grePrefix.10$index`
                    if [ -n "$grePresent" ]; then
                      vlanAdded=`brctl show $brlanPrefix$index | grep $l2sd0Prefix.10$index`
                      if [ -z "$vlanAdded" ]; then
                        echo "[RDKB_PLATFORM_ERROR] : Vlan not added $l2sd0Prefix.10$index"
                        brctl addif $brlanPrefix$index $l2sd0Prefix.10$index
                      fi 
                    fi
                done
                
                SECURED_24=`dmcli eRT getv Device.WiFi.SSID.9.Enable | grep value | cut -f3 -d : | cut -f2 -d" "`
                SECURED_5=`dmcli eRT getv Device.WiFi.SSID.10.Enable | grep value | cut -f3 -d : | cut -f2 -d" "`

                #Check for Secured Xfinity hotspot briges and associate them properly if 
		#not proper
                #l2sd0.103 case
                
		#Secured Xfinity 2.4
                grePresent=`ifconfig -a | grep $grePrefix.104`
                if [ -n "$grePresent" ]; then
                 ifconfig | grep l2sd0.104
                 if [ $? == 1 ]; then
                    echo_t "XfinityWifi is enabled Secured gre created, but l2sd0.104 interface is not created try creatig it"
                    sysevent set multinet_7-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 7
                    ifconfig l2sd0.104 up
                    ifconfig | grep l2sd0.104
                    if [ $? == 1 ]; then
                       echo "l2sd0.104 is not created at First Retry, try again after 2 sec"
                       sleep 2
                       sysevent set multinet_7-status stopped
                       $UTOPIA_PATH/service_multinet_exec multinet-start 7
                       ifconfig l2sd0.104 up
                       ifconfig | grep l2sd0.104
                       if [ $? == 1 ]; then
                          echo "[RDKB_PLATFORM_ERROR] : l2sd0.104 is not created after Second Retry, no more retries !!!"
                       fi
                    else
                         echo "[RDKB_PLATFORM_ERROR] : l2sd0.104 created at First Retry itself"
                    fi
                 else
                    echo "Xfinitywifi is enabled and l2sd0.104 is present"
                 fi
                else
                #RDKB-17221: In some rare devices we found though Xfinity secured ssid enabled, but it did'nt create the gre tunnels
                #but all secured SSIDs Vaps were up and system remained in this state for long not allowing clients to
                #connect
                 if [ "$SECURED_24" = "true" ]; then
                  echo "[RDKB_PLATFORM_ERROR] :XfinityWifi: Secured SSID 2.4 is enabled but gre tunnels not present, restoring it"
                  sysevent set multinet_7-status stopped
                  $UTOPIA_PATH/service_multinet_exec multinet-start 7
                 fi
                fi
                
                #Secured Xfinity 5
                grePresent=`ifconfig -a | grep $grePrefix.105`
                if [ -n "$grePresent" ]; then
                 ifconfig | grep l2sd0.105
                 if [ $? == 1 ]; then
                    echo_t "XfinityWifi is enabled Secured gre created, but l2sd0.105 interface is not created try creatig it"
                    sysevent set multinet_8-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 8
                    ifconfig l2sd0.105 up
                    ifconfig | grep l2sd0.105
                    if [ $? == 1 ]; then
                       echo "l2sd0.105 is not created at First Retry, try again after 2 sec"
                       sleep 2
                       sysevent set multinet_8-status stopped
                       $UTOPIA_PATH/service_multinet_exec multinet-start 8
                       ifconfig l2sd0.105 up
                       ifconfig | grep l2sd0.105
                       if [ $? == 1 ]; then
                          echo "[RDKB_PLATFORM_ERROR] : l2sd0.105 is not created after Second Retry, no more retries !!!"
                       fi
                    else
                         echo "[RDKB_PLATFORM_ERROR] : l2sd0.105 created at First Retry itself"
                    fi
                 else
                    echo "Xfinitywifi is enabled and l2sd0.105 is present"
                 fi
                else
                 if [ "$SECURED_5" = "true" ]; then
                  echo "[RDKB_PLATFORM_ERROR] :XfinityWifi: Secured SSID 5GHz is enabled but gre tunnels not present, restoring it"
                  sysevent set multinet_8-status stopped
                  $UTOPIA_PATH/service_multinet_exec multinet-start 8
                 fi
                fi
                 
	fi
if [ -f "/etc/PARODUS_ENABLE" ]; then
	# Checking parodus PID
        PARODUS_PID=`pidof parodus`
        if [ "$PARODUS_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : parodus process is not running, need restart"
                echo_t "Check parodusCmd.cmd in /tmp"
		if [ -e /tmp/parodusCmd.cmd ]; then
			 echo_t "parodusCmd.cmd exists in tmp"
			 parodusCmd=`cat /tmp/parodusCmd.cmd`
			 #start parodus
			 $parodusCmd &
			 echo_t "Started parodusCmd in background"
		else
		 	 echo_t "parodusCmd.cmd does not exist in tmp, unable to start parodus"
            	fi
        fi
else
	# Checking webpa PID
	WEBPA_PID=`pidof webpa`
	if [ "$WEBPA_PID" = "" ]; then
		ENABLEWEBPA=`cat /nvram/webpa_cfg.json | grep -r EnablePa | awk '{print $2}' | sed 's|[\"\",]||g'`
		if [ "$ENABLEWEBPA" = "true" ];then
		echo_t "RDKB_PROCESS_CRASHED : WebPA_process is not running, need restart"
			#We'll set the reason only if webpa reconnect is not due to DNS resolve
			syscfg get X_RDKCENTRAL-COM_LastReconnectReason | grep "Dns_Res_webpa_reconnect"
			if [ $? != 0 ]; then
				echo "setting reconnect reason from task_health_monitor.sh"
			echo_t "Setting Last reconnect reason"
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
			echo_t "SET succeeded"
		fi
			resetNeeded webpa webpa
		fi
	
	fi
fi

	#Check dropbear is alive to do rsync/scp to/fro ATOM
	if [ "$ARM_INTERFACE_IP" != "" ]
	then
           DROPBEAR_ENABLE=`ps -w | grep dropbear | grep $ARM_INTERFACE_IP`
           if [ "$DROPBEAR_ENABLE" == "" ]
           then
                echo_t "RDKB_PROCESS_CRASHED : rsync_dropbear_process is not running, need restart"
		DROPBEAR_PARAMS_1="/tmp/.dropbear/dropcfg1$$"
                DROPBEAR_PARAMS_2="/tmp/.dropbear/dropcfg2$$"
                if [ ! -d '/tmp/.dropbear' ]; then
              	    mkdir -p /tmp/.dropbear
                fi
                getConfigFile $DROPBEAR_PARAMS_1
                getConfigFile $DROPBEAR_PARAMS_2             
                dropbear -r $DROPBEAR_PARAMS_1 -r $DROPBEAR_PARAMS_2 -E -s -p $ARM_INTERFACE_IP:22 -P /var/run/dropbear_ipc.pid > /dev/null 2>&1
           fi
           rm -rf /tmp/.dropbear
        fi

	# Checking lighttpd PID
	LIGHTTPD_PID=`pidof lighttpd`
	if [ "$LIGHTTPD_PID" = "" ]; then
		isPortKilled=`netstat -anp | grep 21515`
		if [ "$isPortKilled" != "" ]
		then
		    echo_t "Port 21515 is still alive. Killing processes associated to 21515"
		    fuser -k 21515/tcp
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


# Checking snmp master PID
        if [ "$BOX_TYPE" = "XB3" ]; then
 		SNMP_MASTER_PID=`pidof snmp_agent_cm`
		if [ "$SNMP_MASTER_PID" == "" ] && [  ! -f "$SNMPMASTERCRASHED"  ];then
				echo_t "[RDKB_PROCESS_CRASHED] : snmp_agent_cm process crashed" 
				touch $SNMPMASTERCRASHED
		fi
	fi

	if [ -e /tmp/atom_ro ]; then
		reboot_needed_atom_ro=1
		rebootDeviceNeeded=1
	fi

        # Verify MDC is enabled in the build by
        # checking if /usr/bin/Arm_Mdc exists
        ArmMdc_PID=`pidof Arm_Mdc`
	if [ -e /usr/bin/Arm_Mdc ] && [ "$ArmMdc_PID" = "" ]; then
		echo "RDKB_PROCESS_CRASHED : Arm_Mdc is not running, restarting it"
		resetNeeded CcspArmMdc Arm_Mdc
	fi


# Checking whether brlan0 and l2sd0.100 are created properly , if not recreate it
	lanSelfheal=`sysevent get lan_selfheal`
	echo_t "[RDKB_SELFHEAL] : Value of lanSelfheal : $lanSelfheal"
	if [ "$lanSelfheal" != "done" ]
	then
      # Check device is in router mode
      # Get from syscfg instead of dmcli for performance reasons
			check_device_in_bridge_mode=`syscfg get bridge_mode`
			if [ "$check_device_in_bridge_mode" == "0" ]
			then
				check_if_brlan0_created=`ifconfig | grep brlan0`
				check_if_brlan0_up=`ifconfig brlan0 | grep UP`
				check_if_brlan0_hasip=`ifconfig brlan0 | grep "inet addr"`
				check_if_l2sd0_100_created=`ifconfig | grep l2sd0.100`
				check_if_l2sd0_100_up=`ifconfig l2sd0.100 | grep UP `
				if [ "$check_if_brlan0_created" = "" ] || [ "$check_if_brlan0_up" = "" ] || [ "$check_if_brlan0_hasip" = "" ] || [ "$check_if_l2sd0_100_created" = "" ] || [ "$check_if_l2sd0_100_up" = "" ]
				then
					echo_t "[RDKB_PLATFORM_ERROR] : Either brlan0 or l2sd0.100 is not completely up, setting event to recreate vlan and brlan0 interface"
					   echo_t "[RDKB_SELFHEAL_BOOTUP] : brlan0 and l2sd0.100 o/p "
					    ifconfig brlan0;ifconfig l2sd0.100; brctl show
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

					if [ "$check_if_brlan0_created" = "" ] && [ "$check_if_l2sd0_100_created" = "" ]; then	
						/etc/utopia/registration.d/02_multinet restart
					fi

					sysevent set multinet-down 1
					sleep 5
					sysevent set multinet-up 1
					sleep 30
					sysevent set lan_selfheal done
				fi

            		fi
	else
		echo_t "[RDKB_SELFHEAL] : brlan0 already restarted. Not restarting again"
	fi


# Checking whether brlan1 and l2sd0.101 interface are created properly

	l3netRestart=`sysevent get l3net_selfheal`
	echo_t "[RDKB_SELFHEAL] : Value of l3net_selfheal : $l3netRestart"

	if [ "$l3netRestart" != "done" ]
	then

		check_if_brlan1_created=`ifconfig | grep brlan1`
		check_if_brlan1_up=`ifconfig brlan1 | grep UP`
        	check_if_brlan1_hasip=`ifconfig brlan1 | grep "inet addr"`
		check_if_l2sd0_101_created=`ifconfig | grep l2sd0.101`
		check_if_l2sd0_101_up=`ifconfig l2sd0.101 | grep UP `
	
		if [ "$check_if_brlan1_created" = "" ] || [ "$check_if_brlan1_up" = "" ] || [ "$check_if_brlan1_hasip" = "" ] || [ "$check_if_l2sd0_101_created" = "" ] || [ "$check_if_l2sd0_101_up" = "" ]
        	then
				echo_t "[RDKB_SELFHEAL_BOOTUP] : brlan1 and l2sd0.101 o/p "
				ifconfig brlan1;ifconfig l2sd0.101; brctl show
	       		echo_t "[RDKB_PLATFORM_ERROR] : Either brlan1 or l2sd0.101 is not completely up, setting event to recreate vlan and brlan1 interface"

			ipv5_status=`sysevent get ipv4_5-status`
	        	lan_l3net=`sysevent get homesecurity_lan_l3net`

			if [ "$lan_l3net" != "" ]
			then
				if [ "$ipv5_status" = "" ] || [ "$ipv5_status" = "down" ]
				then
					echo_t "[RDKB_SELFHEAL] : ipv5_4-status is not set , setting event to create homesecurity lan"
					sysevent set ipv4-up $lan_l3net
					sleep 5
				fi
			fi

			if [ "$check_if_brlan1_created" = "" ] && [ "$check_if_l2sd0_101_created" = "" ] ; then
				/etc/utopia/registration.d/02_multinet restart
			fi
				
			sysevent set multinet-down 2
			sleep 5
			sysevent set multinet-up 2
			sleep 10
			sysevent set l3net_selfheal done
		fi
	else
		echo_t "[RDKB_SELFHEAL] : brlan1 already restarted. Not restarting again"
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

        # RDKB-6895
        isBridging=`syscfg get bridge_mode`
        if [ "$isBridging" != "0" ]
        then
               BR_MODE=1
               echo_t "[RDKB_SELFHEAL] : Device in bridge mode"
        fi

        #check for PandM response
        bridgeMode=`dmcli eRT getv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode`
        bridgeSucceed=`echo $bridgeMode | grep "Execution succeed"`
        if [ "$bridgeSucceed" == "" ]
        then
            echo_t "[RDKB_SELFHEAL_DEBUG] : bridge mode = $bridgeMode"
            serialNumber=`dmcli eRT getv Device.DeviceInfo.SerialNumber`
            echo_t "[RDKB_SELFHEAL_DEBUG] : SerialNumber = $serialNumber"
            modelName=`dmcli eRT getv Device.DeviceInfo.ModelName`
            echo_t "[RDKB_SELFHEAL_DEBUG] : modelName = $modelName"

            pandm_timeout=`echo $bridgeMode | grep "CCSP_ERR_TIMEOUT"`
            pandm_notexist=`echo $bridgeMode | grep "CCSP_ERR_NOT_EXIST"`
            if [ "$pandm_timeout" != "" ] || [ "$pandm_notexist" != "" ]
            then
                echo_t "[RDKB_PLATFORM_ERROR] : pandm parameter timed out or failed to return"
                cr_query=`dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name`
                cr_timeout=`echo $cr_query | grep "CCSP_ERR_TIMEOUT"`
                cr_pam_notexist=`echo $cr_query | grep "CCSP_ERR_NOT_EXIST"`
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
                      echo_t "[RDKB_PLATFORM_ERROR] : 5G private SSID (ath1) is off."
                   else
                      echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G status."                      
                      echo "$ssidStatus_5"
                   fi
                fi
            else
               echo_t "[RDKB_PLATFORM_ERROR] : wifi agent is off while checking 5G status."
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
            echo_t "[RDKB_PLATFORM_ERROR] : wifi agent is off while checking 2.4G Enable"
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
               echo_t "[RDKB_PLATFORM_ERROR] : wifi agent is off while checking 2.4G status."
               echo "$ssidStatus_2"
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

	if [ $BR_MODE -eq 0 ] && [ ! -f "$brlan1_firewall" ]
	then
		firewall_rules=`iptables-save`
		check_if_brlan1=`echo $firewall_rules | grep brlan1`
		if [ "$check_if_brlan1" == "" ]; then
			echo_t "[RDKB_PLATFORM_ERROR]:brlan1_firewall_rules_missing,restarting firewall"
			sysevent set firewall-restart
		fi
		touch $brlan1_firewall
         fi

#Logging to check the DHCP range corruption
    lan_ipaddr=`syscfg get lan_ipaddr`
    lan_netmask=`syscfg get lan_netmask`
    echo_t "DHCPCORRUPT_TRACE:lan_ipaddr=$lan_ipaddr,lan_netmask=$lan_netmask"

    lost_and_found_enable=`syscfg get lost_and_found_enable`
    echo_t "DHCPCORRUPT_TRACE:lost_and_found_enable=$lost_and_found_enable"
    if [ "$lost_and_found_enable" == "true" ]
    then
        iot_ifname=`syscfg get iot_ifname`
        iot_dhcp_start=`syscfg get iot_dhcp_start`
        iot_dhcp_end=`syscfg get iot_dhcp_end`
        iot_netmask=`syscfg get iot_netmask`
        echo_t "DHCPCORRUPT_TRACE:iot_ifname=$iot_ifname "
        echo_t "DHCPCORRUPT_TRACE:iot_dhcp_start=$iot_dhcp_start,iot_dhcp_end=$iot_dhcp_end,iot_netmask=$iot_netmask"
    fi

#Checking whether dnsmasq is running or not
   DNS_PID=`pidof dnsmasq`
   if [ "$DNS_PID" == "" ]
   then
		 echo_t "[RDKB_SELFHEAL] : dnsmasq is not running"   
   else
	     brlan1up=`cat /var/dnsmasq.conf | grep brlan1`
	     brlan0up=`cat /var/dnsmasq.conf | grep brlan0`
             lnf_ifname=`syscfg get iot_ifname`
             if [ "$lnf_ifname" != "" ]
             then
                echo_t "[RDKB_SELFHEAL] : LnF interface is: $lnf_ifname"
                infup=`cat /var/dnsmasq.conf | grep $lnf_ifname`
             else
                echo_t "[RDKB_SELFHEAL] : LnF interface not available in DB"
                #Set some value so that dnsmasq won't restart
                infup="NA"
             fi

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

             if [ "$infup" == "" ]
             then
                 echo_t "[RDKB_SELFHEAL] : $lnf_ifname info is not availble in dnsmasq.conf"
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
		for zombiepid in $checkIfDnsmasqIsZombie
		do
			confirmZombie=`grep "State:" /proc/$zombiepid/status | grep -i "zombie"`
			if [ "$confirmZombie" != "" ] ; then
				echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, restarting dnsmasq"
				kill -9 `pidof dnsmasq`
				sysevent set dhcp_server-stop
				sysevent set dhcp_server-start
				break
			fi
		done
	fi

   fi

#Checking dibbler server is running or not RDKB_10683
	DIBBLER_PID=`pidof dibbler-server`
	if [ "$DIBBLER_PID" = "" ]; then

		DHCPV6C_ENABLED=`sysevent get dhcpv6c_enabled`
		if [ "$BR_MODE" == "0" ] && [ "$DHCPV6C_ENABLED" == "1" ]; then

                        DHCPv6EnableStatus=`syscfg get dhcpv6s00::serverenable`
                        if [ "$IS_BCI" = "yes" ] && [ "0" = "$DHCPv6EnableStatus" ]; then
                           echo "DHCPv6 Disabled. Restart of Dibbler process not Required"
                        else
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
	fi

#Checking the zebra is running or not
	ZEBRA_PID=`pidof zebra`
	if [ "$ZEBRA_PID" = "" ]; then
		if [ "$BR_MODE" == "0" ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : zebra is not running, restarting the zebra"
			/etc/utopia/registration.d/20_routing restart
			sysevent set zebra-restart
		fi
	fi

#Checking the ntpd is running or not
	NTPD_PID=`pidof ntpd`
	if [ "$NTPD_PID" = "" ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : NTPD is not running, restarting the NTPD"
			sysevent set ntpd-restart
	fi

#Checking if rpcserver is running
	RPCSERVER_PID=`pidof rpcserver`
	if [ "$RPCSERVER_PID" = "" ] && [ -f /usr/bin/rpcserver ]; then
			echo "[`getDateTime`] RDKB_PROCESS_CRASHED : RPCSERVER is not running on ARM side, restarting "
			/usr/bin/rpcserver &
	fi

# Checking for WAN_INTERFACE ipv6 address
DHCPV6_ERROR_FILE="/tmp/.dhcpv6SolicitLoopError"
WAN_STATUS=`sysevent get wan-status`
WAN_IPv4_Addr=`ifconfig $WAN_INTERFACE | grep inet | grep -v inet6`
DHCPV6_HANDLER="/etc/utopia/service.d/service_dhcpv6_client.sh"

if [ "$WAN_STATUS" != "started" ]
then
	echo_t "WAN_STATUS : wan-status is $WAN_STATUS"
fi

if [ -f "$DHCPV6_ERROR_FILE" ] && [ "$WAN_STATUS" = "started" ] && [ "$WAN_IPv4_Addr" != "" ]
then
	          isIPv6=`ifconfig $WAN_INTERFACE | grep inet6 | grep "Scope:Global"`
		echo "isIPv6 = $isIPv6"
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

	if [[ "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor"  || "$BOX_TYPE" = "TCCBR"  ]]; then
		check_wan_dhcp_client_v4=`ps w | grep udhcpc | grep erouter`
		check_wan_dhcp_client_v6=`ps w | grep dibbler-client | grep -v grep`
	else
		dhcp_cli_output=`ps w | grep ti_ | grep erouter0`
		check_wan_dhcp_client_v4=`echo $dhcp_cli_output | grep ti_udhcpc`
		check_wan_dhcp_client_v6=`echo $dhcp_cli_output | grep ti_dhcp6c`
	fi

	if [ "x$check_wan_dhcp_client_v4" != "x" ] && [ "x$check_wan_dhcp_client_v6" != "x" ];then
		if [ `cat /proc/net/dbrctl/mode`  = "standbay" ]
		then
			echo "RDKB_SELFHEAL : dbrctl mode is standbay, changing mode to registered"
			echo "registered" > /proc/net/dbrctl/mode
		fi
	fi

	if [ "x$check_wan_dhcp_client_v4" = "x" ]; then
		echo "RDKB_PROCESS_CRASHED : DHCP Client for v4 is not running, need restart "
		wan_dhcp_client_v4=0
	fi

	if [ "x$check_wan_dhcp_client_v6" = "x" ]; then
		echo "RDKB_PROCESS_CRASHED : DHCP Client for v6 is not running, need restart"
		wan_dhcp_client_v6=0
	fi

	DHCP_STATUS=`dmcli eRT getv Device.DHCPv4.Client.1.DHCPStatus | grep value | cut -f3 -d : | cut -f2 -d" "`

	if [ "$DHCP_STATUS" != "Bound" ] ; then
		if [ $wan_dhcp_client_v4 -eq 0 ] || [ $wan_dhcp_client_v6 -eq 0 ]; then
			echo "DHCP_CLIENT : DHCPStatus is $DHCP_STATUS, restarting WAN"
			sh /etc/utopia/service.d/service_wan.sh wan-stop
			sh /etc/utopia/service.d/service_wan.sh wan-start
			wan_dhcp_client_v4=1
			wan_dhcp_client_v6=1
		fi
	fi

	if [ $wan_dhcp_client_v4 -eq 0 ];
	then
		if [[ "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor"  ||  "$BOX_TYPE" = "TCCBR"  ]]; then
			V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
		else
			DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
			V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 1"
		fi
		echo "DHCP_CLIENT : Restarting DHCP Client for v4"
		eval "$V4_EXEC_CMD"
		sleep 5
		wan_dhcp_client_v4=1
	fi

	if [ $wan_dhcp_client_v6 -eq 0 ];
	then
		echo "DHCP_CLIENT : Restarting DHCP Client for v6"
		if [[ "$BOX_TYPE" = "XB6" && "$MANUFACTURE" = "Technicolor"  || "$BOX_TYPE" = "TCCBR"  ]]; then
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

# Test to make sure that if mesh is enabled the backhaul tunnels are attached to the bridges
MESH_ENABLE=`dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Enable | grep value | cut -f3 -d : | cut -f2 -d" "`
if [ "$MESH_ENABLE" = "true" ]
then
   echo_t "[RDKB_SELFHEAL] : Mesh is enabled, test if tunnels are attached to bridges"

   # Fetch mesh tunnels from the brlan0 bridge if they exist
   brctl0_ifaces=`brctl show brlan0 | egrep "pgd"` 
   br0_ifaces=`ifconfig | egrep "^pgd" | egrep "\.100" | awk '{print $1}'`

   for ifn in $br0_ifaces; do
      brFound="false"

      for br in $brctl0_ifaces; do
         if [ "$br" == "$ifn" ]; then
            brFound="true"
         fi
      done
      if [ "$brFound" == "false" ]; then
         echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan0"
         brctl addif brlan0 $ifn;
      fi
   done

   # Fetch mesh tunnels from the brlan1 bridge if they exist
   brctl1_ifaces=`brctl show brlan1 | egrep "pgd"` 
   br1_ifaces=`ifconfig | egrep "^pgd" | egrep "\.101" | awk '{print $1}'`

   for ifn in $br1_ifaces; do
      brFound="false"

      for br in $brctl1_ifaces; do
         if [ "$br" == "$ifn" ]; then
            brFound="true"
         fi
      done
      if [ "$brFound" == "false" ]; then
         echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan1"
         brctl addif brlan1 $ifn;
      fi
   done
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
					if [ "$reboot_needed_atom_ro" -eq 1 ]; then
						echo_t "RDKB_REBOOT : atom is read only, rebooting the device."
						dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string atom_read_only
						sh /etc/calc_random_time_to_reboot_dev.sh "ATOM_RO" &
					elif [ "$rebootNeededforbrlan1" -eq 1 ]
					then
						echo "rebootNeededforbrlan1"
						echo_t "RDKB_REBOOT : brlan1 interface is not up, rebooting the device."
						echo_t "Setting Last reboot reason"
						dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string brlan1_down
						echo_t "SET succeeded"
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
