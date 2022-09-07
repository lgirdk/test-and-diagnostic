#!/bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2019 RDK Management

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

if [ "$(syscfg get selfheal_enable)" !=  "true" ]; then
   if [ -e "/tmp/deadlock_warning" ]; then
      rm "/tmp/deadlock_warning"
   fi
   exit 0
fi

fast_dmcli ()
{
    if [ "$BOX_TYPE" = "MV1" ]; then
        rpcclient2 "dmcli $1 $2 $3"
    else
        dmcli $1 $2 $3
    fi
}

fast_psmcli ()
{
    if [ "$BOX_TYPE" = "MV1" ]; then
        rpcclient2 "dmcli eRT psmget $2" | sed -ne 's/[[:blank:]]*$//; s/.*value: //p'
    else
        psmcli $1 $2
    fi
}

get_pid() {
    #populate ps cache
    if [ ! -f "$PROCESSES_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        busybox ps ww > $PROCESSES_CACHE
    fi

    awk -v word="$1" 'gensub(/.*\//, "", "g", $5) == word{print $1}' $PROCESSES_CACHE
}

get_ssid_enable() {
    #populate SSID data model cache
    if [ ! -f "$SSIDS_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        dmcli eRT getv Device.WiFi.SSID. > $SSIDS_CACHE
    fi

    awk -v ssid_path=Device.WiFi.SSID.$1.Enable 'found==1{print $5; exit;} $NF==ssid_path{ found=1; }' $SSIDS_CACHE
}

get_ssid_status() {
    #populate SSID data model cache
    if [ ! -f "$SSIDS_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        dmcli eRT getv Device.WiFi.SSID. > $SSIDS_CACHE
    fi

    awk -v ssid_path=Device.WiFi.SSID.$1.Status 'found==1{print $5; exit;} $NF==ssid_path{ found=1; }' $SSIDS_CACHE
}

get_ssid_name() {
    #populate SSID data model cache
    if [ ! -f "$SSIDS_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        dmcli eRT getv Device.WiFi.SSID. > $SSIDS_CACHE
    fi

    awk -v ssid_path=Device.WiFi.SSID.$1.Name 'found==1{print $5; exit;} $NF==ssid_path{ found=1; }' $SSIDS_CACHE
}

get_from_ssid_cache() {
    #populate SSID data model cache
    if [ ! -f "$SSIDS_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        dmcli eRT getv Device.WiFi.SSID. > $SSIDS_CACHE
    fi

    grep "$1" $SSIDS_CACHE
}

is_ssid_execution_succeed() {
    #populate SSID data model cache
    if [ ! -f "$SSIDS_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        dmcli eRT getv Device.WiFi.SSID. > $SSIDS_CACHE
    fi

    grep "Execution succeed" $SSIDS_CACHE > /dev/null
}

get_from_syscfg_cache() {
    #populate syscfg cache
    if [ ! -f "$SYSCFG_CACHE" ]; then
        if [ ! -d "$CACHE_PATH" ]; then
            mkdir "$CACHE_PATH"
        fi
        syscfg show > $SYSCFG_CACHE
    fi

    awk -F "=" -v word=$1 ' $1==word{print $2; exit;} ' $SYSCFG_CACHE
}

check_component_status(){

        if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
            echo_t "BWG doesn't support voice"
        else
            MTA_PID=$(get_pid CcspMtaAgentSsp)
            if [ "$MTA_PID" = "" ]; then
                #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : MTA_process is not running, restarting it"
                echo_t "RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
                resetNeeded mta CcspMtaAgentSsp

            fi
        fi

        # Checking CM's PID
        if [ "$WAN_TYPE" != "EPON" ]; then
            CM_PID=$(get_pid CcspCMAgentSsp)
            if [ "$CM_PID" = "" ]; then
                #           echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : CM_process is not running, restarting it"
                echo_t "RDKB_PROCESS_CRASHED : CM_process is not running, need restart"
                t2CountNotify "SYS_SH_CM_restart"
                resetNeeded cm CcspCMAgentSsp
            fi
        fi

        # Checking TR69's PID
        if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
            echo_t "BWG doesn't support TR069Pa "
        else
            TR69_PID=$(get_pid CcspTr069PaSsp)
            if [ "$TR69_PID" = "" ] && [ "$BR_MODE" -eq 0 ]; then
                enable_TR69_Binary=$(get_from_syscfg_cache EnableTR69Binary)
                if [ "" = "$enable_TR69_Binary" ] || [ "true" = "$enable_TR69_Binary" ]; then
                    echo_t "RDKB_PROCESS_CRASHED : TR69_process is not running, need restart"
                    t2CountNotify "SYS_SH_TR69Restart"
                    resetNeeded TR69 CcspTr069PaSsp
                fi
            fi
        fi

        # Checking Test and Diagnostic's PID
        TandD_PID=$(get_pid CcspTandDSsp)
        if [ "$TandD_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : TandD_process is not running, need restart"
            resetNeeded tad CcspTandDSsp
        fi

        # Checking Lan Manager PID
        LM_PID=$(get_pid CcspLMLite)
        if [ "$LM_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : LanManager_process is not running, need restart"
            t2CountNotify "SYS_SH_LM_restart"
            resetNeeded lm CcspLMLite
        fi

        # Checking CcspEthAgent PID
        ETHAGENT_PID=$(get_pid CcspEthAgent)
        if [ "$ETHAGENT_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : CcspEthAgent_process is not running, need restart"
            resetNeeded ethagent CcspEthAgent
        fi

        # Not needed for MV1 as wifi runs on ATOM side in MV1
        if [ "$FIRMWARE_TYPE" = "OFW" ] && [ "$BOX_TYPE" != "MV1" ]; then
            # Checking CcspWifiSsp PID
            WIFI_PID=$(get_pid CcspWifiSsp)
            if [ "$WIFI_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : CcspWifiSsp process is not running, need restart"
                resetNeeded wifi CcspWifiSsp
            fi
        fi

        # Checking notify_comp's PID
        notify_comp=$(get_pid notify_comp)
        if [ "$notify_comp" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED :notify_comp is not running, need restart"
            resetNeeded notifycomponent notify_comp
        fi

        PAM_PID=$(busybox pidof CcspPandMSsp)
        if [ "$PAM_PID" = "" ]; then
            # Remove the P&M initialized flag
            rm -rf /tmp/pam_initialized
            echo_t "RDKB_PROCESS_CRASHED : PAM_process is not running, need restart"
            t2CountNotify "SYS_SH_PAM_CRASH_RESTART"
            resetNeeded pam CcspPandMSsp
        fi
}

CACHE_PATH="/tmp/.thmCache/"
SSIDS_CACHE=$CACHE_PATH"SSIDs"
PROCESSES_CACHE=$CACHE_PATH"processes"
SYSCFG_CACHE=$CACHE_PATH"syscfg"
UTOPIA_PATH="/etc/utopia/service.d"
TAD_PATH="/usr/ccsp/tad"
RDKLOGGER_PATH="/rdklogger"
PRIVATE_LAN="brlan0"
BR_MODE=0
RADIO_COUNT=0

# Clear cache
if [ -d $CACHE_PATH ]; then
    rm -rf $CACHE_PATH
fi

DIBBLER_SERVER_CONF="/etc/dibbler/server.conf"
DHCPV6_HANDLER="/etc/utopia/service.d/service_dhcpv6_client.sh"
Unit_Activated=$(get_from_syscfg_cache unit_activated)

if [ -e /tmp/deadlock_warning ]; then
   dead_lock_recovery_needed=true
fi

source $TAD_PATH/corrective_action.sh
source /etc/utopia/service.d/event_handler_functions.sh

# ----------------------------------------------------------------------------
while true
do
# ----------------------------------------------------------------------------

monitor_interval=$(get_from_syscfg_cache process_monitor_interval)
[ -z "$monitor_interval" ] && monitor_interval="5"

source /etc/waninfo.sh
ovs_enable=false

if [ -d "/sys/module/openvswitch/" ];then
   ovs_enable=true
fi
bridgeUtilEnable=`get_from_syscfg_cache bridge_util_enable`
MAPT_CONFIG=`sysevent get mapt_config_flag`

PSM_SHUTDOWN="/tmp/.forcefull_psm_shutdown"

if [ "$BOX_TYPE" = "MV1" ]; then
    CONSOLE_LOG="/rdklogs/logs/ArmConsolelog.txt.0"
else
    CONSOLE_LOG="/rdklogs/logs/Consolelog.txt.0"
fi

SELFHEAL_TYPE="BASE"

CCSP_ERR_NOT_CONNECT=190
CCSP_ERR_TIMEOUT=191
CCSP_ERR_NOT_EXIST=192

if [ -n "$dead_lock_recovery_needed" ]; then

   echo_t "RDKB_SELFHEAL : DEAD LOCK WARNING RECEIVED Checking for component health"

   t2CountNotify "SYS_SH_DEADLOCK_warning"
   #Always keep the array1 & array2 Aligned
   declare -a array1=("cm" "lmlite" "tr069pa" "ethagent" "wifi"  "mta" "notifycomponent" "tdm" )
   declare -a array2=("CcspCMAgentSsp" "CcspLMLite" "CcspTr069PaSsp" "CcspEthAgent" "CcspWifiSsp" "CcspMtaAgentSsp" "notify_comp" "CcspTandDSsp" )

   if [ "$BOX_TYPE" = "MV1" ]; then
      wait_time=10
   else
      wait_time=5
   fi

   for i in "${!array1[@]}"; do
       dmcli eRT getv com.cisco.spvtg.ccsp.${array1[i]}.Health &
       BACK_PID=$!
       time_out=0

       #Checking component health with time out value wait_time sec
       while kill -0 $BACK_PID ; do
          sleep 1
          time_out=$time_out+1
          if [[ $time_out -gt $wait_time ]] ; then
             echo "pid is $BACK_PID"
             kill -9 $BACK_PID
             BACK_PID=0
             break;
          fi
       done
       time_out=0
       if [ "$BACK_PID" = "0" ] ; then
          echo_t "RDKB_SELFHEAL : DEAD LOCK WARNING RECEIVED Need to restart ${array2[i]}"
          t2CountNotify "SYS_SH_DEADLOCK_warning"
          if [ "$BOX_TYPE" = "MV1" ] && [ "${array2[i]}" = "CcspWifiSsp" ]; then
             rpcclient2 "kill -9 $(get_pid ${array2[i]})"
             #Process monitor script at ATOM console will reset CcspWifiSsp
          else
             kill -9 $(get_pid ${array2[i]})
             resetNeeded ${array1[i]} ${array2[i]}
          fi
       fi
   done
   dead_lock_recovery_needed=""
   check_component_status
   rm "/tmp/deadlock_warning"
else
   sleep ${monitor_interval}m
fi

# Clear cache after sleep for the new cycle
if [ -d $CACHE_PATH ]; then
    rm -rf $CACHE_PATH
fi

case $SELFHEAL_TYPE in
    "BASE")
        grePrefix="gretap0"
        brlanPrefix="brlan"
        l2sd0Prefix="l2sd0"
        #(already done by corrective_action.sh)source /etc/log_timestamp.sh

        if [ -f /etc/mount-utils/getConfigFile.sh ]; then
            . /etc/mount-utils/getConfigFile.sh
        fi

        if [ "$MODEL_NUM" = "DPC3939" ] || [ "$MODEL_NUM" = "DPC3941" ]; then
            ADVSEC_PATH="/tmp/cujo_dnld/usr/ccsp/advsec/advsec.sh"
        else
            ADVSEC_PATH="/usr/ccsp/advsec/advsec.sh"
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        ADVSEC_PATH="/usr/ccsp/advsec/advsec.sh"
    ;;
esac

ping_failed=0
ping_success=0
SyseventdCrashed="/rdklogs/syseventd_crashed"
PARCONNHEALTH_PATH="/tmp/parconnhealth.txt"
PING_PATH="/usr/sbin"

case $SELFHEAL_TYPE in
    "BASE")
        SNMPMASTERCRASHED="/tmp/snmp_cm_crashed"
        WAN_INTERFACE=$(getWanInterfaceName)
        PEER_COMM_ID="/tmp/elxrretyt.swr"

        if [ "$BOX_TYPE" = "XB3" ] && [ ! -f /usr/bin/GetConfigFile ]; then
            echo "Error: GetConfigFile Not Found"
            exit
        fi
        IDLE_TIMEOUT=60
    ;;
    "TCCBR")
        WAN_INTERFACE=$(getWanInterfaceName)
        PEER_COMM_ID="/tmp/elxrretyt.swr"

        if [ ! -f /usr/bin/GetConfigFile ]; then
            echo "Error: GetConfigFile Not Found"
            exit
        fi
        IDLE_TIMEOUT=30
    ;;
    "SYSTEMD")
    ;;
esac


exec 3>&1 4>&2 >> $SELFHEALFILE 2>&1

# set thisREADYFILE for several tests below:
case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
        thisREADYFILE="/tmp/.brcm_wifi_ready"
    ;;
    "SYSTEMD")
        thisREADYFILE="/tmp/.qtn_ready"
        case $MODEL_NUM in
            *CGM4331COM*) thisREADYFILE="/tmp/.brcm_wifi_ready";;
            *TG4482A*) thisREADYFILE="/tmp/.puma_wifi_ready";; ## This will need to change during integration effort
            *) ;;
        esac
    ;;
esac

# set thisWAN_TYPE for several tests below:
case $SELFHEAL_TYPE in
    "BASE")
        thisWAN_TYPE="$WAN_TYPE"
    ;;
    "TCCBR")
        thisWAN_TYPE="NOT_EPON" # WAN_TYPE is undefined for TCCBR, so kludge it so that tests fail for "EPON"
    ;;
    "SYSTEMD")
        thisWAN_TYPE="$WAN_TYPE"
    ;;
esac


# set thisIS_BCI for several tests below:
# 'thisIS_BCI' is used where 'IS_BCI' was added in recent changes (c.6/2019)
# 'IS_BCI' is still used when appearing in earlier code.
# TBD: may be able to set 'thisIS_BCI=$IS_BCI' for ALL devices?
case $SELFHEAL_TYPE in
    "BASE")
        thisIS_BCI="$IS_BCI"
    ;;
    "TCCBR")
        thisIS_BCI="no"
    ;;
    "SYSTEMD")
        thisIS_BCI="no"
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        if [ -f $ADVSEC_PATH ]; then
            source $ADVSEC_PATH
        fi
        reboot_needed_atom_ro=0
        if [ "$thisIS_BCI" != "yes" ]; then
            brlan1_firewall="/tmp/brlan1_firewall_rule_validated"
        fi
    ;;
    "TCCBR")
        reb_window=0
    ;;
    "SYSTEMD")
        WAN_INTERFACE=$(getWanInterfaceName)

        if [ -f $ADVSEC_PATH ]; then
            source $ADVSEC_PATH
        fi

        brlan1_firewall="/tmp/brlan1_firewall_rule_validated"
    ;;
esac

#Find the DHCPv6 client type 
ti_dhcpv6_type="$(get_pid ti_dhcp6c)"
dibbler_client_type="$(get_pid dibbler-client)"
if [ "$ti_dhcpv6_type" = "" ] && [ ! -z "$dibbler_client_type" ];then
	DHCPv6_TYPE="dibbler-client"
elif [ ! -z "$ti_dhcpv6_type" ] && [ "$dibbler_client_type" = "" ];then
	DHCPv6_TYPE="ti_dhcp6c"
else
	DHCPv6_TYPE=""
fi
echo_t "DHCPv6_Client_type is $DHCPv6_TYPE"
#function to restart Dhcpv6_Client
Dhcpv6_Client_restart ()
{
	if [ "$1" = "" ];then
		echo_t "DHCPv6 Client not running"
		return 
	fi
	process_restart_need=0
	if [ "$2" = "restart_for_dibbler-server" ];then
		PAM_UP="$(get_pid CcspPandMSsp)"
		if [ "$PAM_UP" != "" ];then
                	echo_t "PAM pid $PAM_UP & $1 pid $dibbler_client_type $ti_dhcpv6_type"
                        echo_t "RDKB_PROCESS_CRASHED : Restarting $1 to reconfigure server.conf"
			process_restart_need=1
		fi
	fi
	if [ "$process_restart_need" = "1" ] || [ "$2" = "Idle" ];then
		sysevent set dibbler_server_conf-status ""
		if [ "$1" = "dibbler-client" ];then
			dibbler-client stop
            		sleep 2
            		dibbler-client start
            		sleep 5
		elif [ "$1" = "ti_dhcp6c" ];then
			sh $DHCPV6_HANDLER disable
	                sleep 2
	                sh $DHCPV6_HANDLER enable
            		sleep 5
		fi
		wait_till_state "dibbler_server_conf" "ready"
		touch /tmp/dhcpv6-client_restarted
	fi
	if [ ! -f "$DIBBLER_SERVER_CONF" ];then
		return 2
	elif [ ! -s  "$DIBBLER_SERVER_CONF" ];then
		return 1
        elif [ -z "$(get_pid dibbler-server)" ];then
        	dibbler-server stop
                sleep 2
                dibbler-server start
	fi
}

rebootDeviceNeeded=0

LIGHTTPD_CONF="/var/lighttpd.conf"

case $SELFHEAL_TYPE in
    "BASE")
        ###########################################
        if [ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "MV1" ]; then
            wifi_timeout=$(get_from_ssid_cache "CCSP_ERR_TIMEOUT")
            wifi_not_exist=$(get_from_ssid_cache "CCSP_ERR_NOT_EXIST")
            WIFI_QUERY_ERROR=0
            if [ "$wifi_timeout" != "" ] || [ "$wifi_not_exist" != "" ]; then
                echo_t "[RDKB_SELFHEAL] : Wifi query timeout"
                t2CountNotify "WIFI_ERROR_Wifi_query_timeout"
                echo_t "WIFI_QUERY : " `cat SSIDS_CACHE`
                WIFI_QUERY_ERROR=1
            fi


            if [ ! -f $PEER_COMM_ID ]; then
                GetConfigFile $PEER_COMM_ID
            fi
            SSH_ATOM_TEST=$(ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ATOM_IP exit 2>&1)
            echo_t "SSH_ATOM_TEST : $SSH_ATOM_TEST"
            SSH_ERROR=`echo $SSH_ATOM_TEST | grep "Remote closed the connection"`
            SSH_TIMEOUT=`echo $SSH_ATOM_TEST | grep "Idle timeout"`
            ATM_HANG_ERROR=0
            # Do not remove $PEER_COMM_ID. reduces future decryptions
            if [ "$SSH_ERROR" != "" ] || [ "$SSH_TIMEOUT" != "" ]; then
                echo_t "[RDKB_SELFHEAL] : ssh to atom failed"
                ATM_HANG_ERROR=1
            fi

            if [ "$WIFI_QUERY_ERROR" = "1" ] && [ "$ATM_HANG_ERROR" = "1" ]; then
                atom_hang_count=$(sysevent get atom_hang_count)
                echo_t "[RDKB_SELFHEAL] : Atom is not responding. Count $atom_hang_count"
                if [ $atom_hang_count -ge 2 ]; then
                    CheckRebootCretiriaForAtomHang
                    atom_hang_reboot_count=$(get_from_syscfg_cache todays_atom_reboot_count)
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

            ### SNMPv3 master agent self-heal ####
            if [ -f "/etc/SNMP_PA_ENABLE" ]; then
                SNMPv3_PID=$(get_pid snmpd)
                if [ "$SNMPv3_PID" = "" ] && [ "$ENABLE_SNMPv3" = "true" ]; then
                    # Restart disconnected master and agent
                    v3AgentPid=$(grep -i "snmp_subagent" $PROCESSES_CACHE | grep -v "grep" | grep -i "cm_snmp_ma_2"  | awk '{print $1}')
                    if [ "$v3AgentPid" != "" ]; then
                        kill -9 "$v3AgentPid"
                    fi
                    pidOfListener=$(grep -i "inotify" $PROCESSES_CACHE | grep 'run_snmpv3_agent.sh' | awk '{print $1}')
                    if [ "$pidOfListener" != "" ]; then
                        kill -9 "$pidOfListener"
                    fi
                    if [ -f /tmp/snmpd.conf ]; then
                        rm -f /tmp/snmpd.conf
                    fi
                    if [ -f /lib/rdk/run_snmpv3_master.sh ]; then
                        sh /lib/rdk/run_snmpv3_master.sh &
                    fi
                else
                    ### SNMPv3 sub agent self-heal ####
                    v3AgentPid=$(grep -i "[s]nmp_subagent" $PROCESSES_CACHE | grep -i "cm_snmp_ma_2"  | awk '{print $1}')
                    if [ "$v3AgentPid" = "" ] && [ "$ENABLE_SNMPv3" = "true" ]; then
                        # Restart failed sub agent
                        if [ -f /lib/rdk/run_snmpv3_agent.sh ]; then
                            sh /lib/rdk/run_snmpv3_agent.sh &
                        fi
                    fi
                fi
            fi

        fi
        ###########################################

        if [ "$MULTI_CORE" = "yes" ]; then
            if [ "$CORE_TYPE" = "arm" ]; then
                if [ -f /usr/bin/logbackup ]; then
                    # Checking logbackup PID
                    LOGBACKUP_PID=$(get_pid logbackup)
                    if [ "$LOGBACKUP_PID" = "" ]; then
                        echo_t "RDKB_PROCESS_CRASHED : logbackup process is not running, need restart"
                        /usr/bin/logbackup &
                    fi
                fi
            fi
            if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
              # BWGRDK1271
              if [ -f $PING_PATH/ping_peer ]; then
                  WAN_STATUS=$(sysevent get wan-status)
                  if [ "$WAN_STATUS" = "started" ]; then
                      ## Check Peer ip is accessible
                      loop=1
                      ping_peer_rbt_thresh=$(get_from_syscfg_cache ping_peer_reboot_threshold)
                      if [ -z "$ping_peer_rbt_thresh" ]; then
                          echo "RDKB_SELFHEAL : syscfg ping_peer_reboot_threshold unavail. Set older value 3" >> $CONSOLE_LOG
                          ping_peer_rbt_thresh=3
                      fi
                      while [ "$loop" -le $ping_peer_rbt_thresh ]
                        do
                          PING_RES=$(ping_peer)
                          CHECK_PING_RES=$(echo $PING_RES | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1)
                          if [ "$CHECK_PING_RES" != "" ]
                          then
                              if [ "$CHECK_PING_RES" -ne 100 ]
                              then
                                  ping_success=1
                                  echo_t "RDKB_SELFHEAL : Ping to Peer IP is success"
                                  timestamp=$(date '+%d/%m/%Y %T')
                                  echo "$timestamp : RDKB_SELFHEAL : Ping to Peer IP is success" >> $CONSOLE_LOG
                                  break
                              else
                                  echo_t "[RDKB_PLATFORM_ERROR] : ATOM interface is not reachable"
                                  timestamp=$(date '+%d/%m/%Y %T')
                                  echo "$timestamp : [RDKB_PLATFORM_ERROR] : ATOM interface is not reachable" >> $CONSOLE_LOG
                                  ping_failed=1
                              fi
                          else
                              if [ "$DEVICE_MODEL" = "TCHXB3" ]; then
                                  check_if_l2sd0_500_up=$(ifconfig l2sd0.500 | grep "UP" )
                                  check_if_l2sd0_500_ip=$(ifconfig l2sd0.500 | grep "inet" )
                                  if [ "$check_if_l2sd0_500_up" = "" ] || [ "$check_if_l2sd0_500_ip" = "" ]
                                  then
                                      echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is not up, setting to recreate interface"
                                      rpc_ifconfig l2sd0.500 >/dev/null 2>&1
                                      sleep 3
                                  fi
                                  PING_RES=$(ping_peer)
                                  CHECK_PING_RES=$(echo "$PING_RES" | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1)
                                  if [ "$CHECK_PING_RES" != "" ]; then
                                      if [ "$CHECK_PING_RES" -ne 100 ]; then
                                          echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is up,Ping to Peer IP is success"
                                          break
                                      fi
                                  fi
                              fi
                              ping_failed=1
                          fi
                          if [ "$ping_failed" -eq 1 ] && [ "$loop" -lt $ping_peer_rbt_thresh ]; then
                              echo_t "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
                              t2CountNotify "SYS_SH_pingPeerIP_Failed"
                              echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
                              echo "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop" >> $CONSOLE_LOG
                          else
                              cli docsis/cmstatus | grep -i "The CM status is OPERATIONAL" >/dev/null 2>&1
                              if [ $? -eq 0 ]; then
                                  echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
                                  echo "RDKB_SELFHEAL : Ping to Peer IP failed after max retry. CM OPERATIONAL. Rebooting device" >> $CONSOLE_LOG
                                  t2CountNotify "SYS_SH_pingPeerIP_Failed"
                                  echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
                                  echo_t "RDKB_REBOOT : Peer is not up ,Rebooting device "
                                  #echo_t " RDKB_SELFHEAL : Setting Last reboot reason as Peer_down"
                                  reason="Peer_down"
                                  rebootCount=1
                                  #setRebootreason $reason $rebootCount
                                  rebootNeeded RM "" $reason $rebootCount
                              else
                                  echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop. Skip reboot as CM is not OPERATIONAL"
                                  echo "RDKB_SELFHEAL : Ping to Peer IP failed after max retry. CM NOT OPERATIONAL. Skip Reboot" >> $CONSOLE_LOG
                                  cli docsis/cmstatus
                                  echo_t "RDKB_SELFHEAL : Wan Status - $(sysevent get wan-status)"
                                  break
                              fi
                          fi
                          loop=$((loop+1))
                          sleep 5
                        done
                  else
                      echo_t "RDKB_SELFHEAL : wan-status is $WAN_STATUS , Peer_down check bypassed"
                      timestamp=$(date '+%d/%m/%Y %T')
                      echo "$timestamp : RDKB_SELFHEAL : wan-status is $WAN_STATUS , Peer_down check bypassed" >> $CONSOLE_LOG
                  fi
              else
                  echo_t "RDKB_SELFHEAL : ping_peer command not found"
              fi
            else
              if [ -f $PING_PATH/ping_peer ]; then
                  SYSEVENTD_PID=$(get_pid syseventd)
                  if [ "$SYSEVENTD_PID" != "" ]; then
                      ## Check Peer ip is accessible
                      loop=1
                      while [ $loop -le 3 ]
                        do
                          WAN_STATUS=$(sysevent get wan-status)
                          echo_t "RDKB_SELFHEAL : wan-status is $WAN_STATUS"
                          if [ $WAN_STATUS != "started" ]; then
                             echo_t "RDKB_SELFHEAL : wan-status is $WAN_STATUS, Peer_down check bypassed"
                             break
                          fi
                          PING_RES=$(ping_peer)
                          CHECK_PING_RES=$(echo "$PING_RES" | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1)
                          if [ "$CHECK_PING_RES" != "" ]; then
                              if [ $CHECK_PING_RES -ne 100 ]; then
                                  ping_success=1
                                  echo_t "RDKB_SELFHEAL : Ping to Peer IP is success"
                                  break
                              else
                                  echo_t "[RDKB_PLATFORM_ERROR] : ATOM interface is not reachable"
                                  ping_failed=1
                              fi
                          else
                              if [ "$DEVICE_MODEL" = "TCHXB3" ]; then
                                  check_if_l2sd0_500_up=$(ifconfig l2sd0.500 | grep "UP" )
                                  check_if_l2sd0_500_ip=$(ifconfig l2sd0.500 | grep "inet" )
                                  if [ "$check_if_l2sd0_500_up" = "" ] || [ "$check_if_l2sd0_500_ip" = "" ]; then
                                      echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is not up, setting to recreate interface"
                                      rpc_ifconfig l2sd0.500 >/dev/null 2>&1
                                      sleep 3
                                  fi
                                  PING_RES=$(ping_peer)
                                  CHECK_PING_RES=$(echo "$PING_RES" | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1)
                                  if [ "$CHECK_PING_RES" != "" ]; then
                                      if [ $CHECK_PING_RES -ne 100 ]; then
                                          echo_t "[RDKB_PLATFORM_ERROR] : l2sd0.500 is up,Ping to Peer IP is success"
                                          break
                                      fi
                                  fi
                              fi
                              ping_failed=1
                          fi

                          if [ $ping_failed -eq 1 ] && [ $loop -lt 3 ]; then
                              echo_t "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
                              t2CountNotify "SYS_SH_pingPeerIP_Failed"
                              echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
                          else
                              echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
                              t2CountNotify "SYS_SH_pingPeerIP_Failed"
                              echo_t "RDKB_SELFHEAL : Ping command output is $PING_RES"
                              echo_t "RDKB_REBOOT : Peer is not up ,Rebooting device "

                              if [ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "MV1" ]; then
                                 if [ -f /usr/bin/rpcclient2 ] ;then
                                 echo_t "Ping to peer failed check, whether ATOM is good through RPC"
                                    RPC_RES=`rpcclient2 pwd`
                                    RPC_OK=`echo $RPC_RES | grep "RPC CONNECTED"`
                                    if [ "$RPC_OK" != "" ]; then
                                       echo_t "RPC Communication with ATOM is OK"
                                       break
                                    else
                                       echo_t "RPC Communication with ATOM have an issue"
                                    fi
                                 else
                                    echo_t "Not checking communication using rpcclient"
                                 fi
                              fi
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
                      echo_t "RDKB_SELFHEAL : syseventd crashed , Peer_down check bypassed"
                  fi
              else
                  echo_t "RDKB_SELFHEAL : ping_peer command not found"
              fi
            fi

            if [ -f $PING_PATH/arping_peer ]; then
                $PING_PATH/arping_peer
            else
                echo_t "RDKB_SELFHEAL : arping_peer command not found"
            fi
        else
            echo_t "RDKB_SELFHEAL : MULTI_CORE is not defined as yes. Define it as yes if it's a multi core device."
        fi
        ########################################
        if [ "$BOX_TYPE" = "XB3" ] || [ "$FIRMWARE_TYPE" = "OFW" ]; then
            dmesg -n 8
            atomOnlyReboot=$(dmesg | grep -i "Atom only")
            dmesg -n 5
            if [ "x$atomOnlyReboot" = "x" ]; then
                cr_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.CR.Name)

                isCRAlive=$(echo "$cr_name" | grep "Can't find destination compo")
                isCRHung=$(echo "$cr_name" | grep "$CCSP_ERR_TIMEOUT")

                if [ "$isCRAlive" != "" ]; then
                    # Retest by querying some other parameter
                    crReTestop=$(fast_dmcli eRT getv Device.X_CISCO_COM_DeviceControl.DeviceMode)
                    isCRAlive=$(echo "$crReTestop" | grep "Can't find destination compo")
                    RBUS_STATUS=`dmcli eRT retv Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RBUS.Enable`
                    if [ "$isCRAlive" != "" ] || [ "$RBUS_STATUS" == "true" ]; then
                        echo_t "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
                        vendor=$(getVendorName)
                        modelName=$(getModelName)
                        CMMac=$(getCMMac)
                        timestamp=$(getDate)
                        #echo "Setting Last reboot reason"
                        reason="CR_crash"
                        rebootCount=1
                        #setRebootreason $reason $rebootCount
                        echo_t "SET succeeded"
                        echo_t "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"
                        touch $HAVECRASH
                        rebootNeeded RM "CR" $reason $rebootCount
                    fi
                fi

                if [ "$isCRHung" != "" ]; then
                    # Retest by querying some other parameter
                    crReTestop=$(fast_dmcli eRT getv Device.X_CISCO_COM_DeviceControl.DeviceMode)
                    isCRHung=$(echo "$crReTestop" | grep "$CCSP_ERR_TIMEOUT")
                    if [ "$isCRHung" != "" ]; then
                        echo_t "RDKB_PROCESS_CRASHED : CR_process is not responding, need to reboot the unit"
                        vendor=$(getVendorName)
                        modelName=$(getModelName)
                        CMMac=$(getCMMac)
                        timestamp=$(getDate)
                        #echo "Setting Last reboot reason"
                        reason="CR_hang"
                        rebootCount=1
                        #setRebootreason $reason $rebootCount
                        echo_t "SET succeeded"
                        echo_t "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process not responding, need reboot"
                        touch $HAVECRASH
                        rebootNeeded RM "CR" $reason $rebootCount
                    fi
                fi

            else
                echo_t "[RDKB_SELFHEAL] : Atom only reboot is triggered"
            fi
        elif [ "$WAN_TYPE" = "EPON" ]; then
            CR_PID=$(get_pid CcspCrSsp)
            if [ "$CR_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
                vendor=$(getVendorName)
                modelName=$(getModelName)
                CMMac=$(getCMMac)
                timestamp=$(getDate)
                #echo "Setting Last reboot reason"
                reason="CR_crash"
                rebootCount=1
                #setRebootreason $reason $rebootCount
                echo_t "SET succeeded"
                echo_t "RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM CcspCrSsp process died,need reboot"
                touch $HAVECRASH
                rebootNeeded RM "CR" $reason $rebootCount
            fi
        fi




        ###########################################
    ;;
    "SYSTEMD"|"TCCBR")
        if [ "$MODEL_NUM" = "CGM4140COM" ] || [ "$MODEL_NUM" = "CGM4331COM" ] || [ "$MODEL_NUM" = "CGM4981COM" ] || [ "$BOX_TYPE" = "TCCBR" ]; then
            Check_If_Erouter_Exists=$(ifconfig -a | grep "$WAN_INTERFACE")
            ifconfig $WAN_INTERFACE > /dev/null
            wan_exists=$?
            if [ "$Check_If_Erouter_Exists" = "" ] && [ $wan_exists -ne 0 ]; then
                echo_t "RDKB_REBOOT : Erouter0 interface is not up ,Rebooting device"
                echo_t "Setting Last reboot reason Erouter_Down"
                t2CountNotify "SYS_ERROR_ErouterDown_reboot"
                reason="Erouter_Down"
                rebootCount=1
                rebootNeeded RM "" $reason $rebootCount
            fi

        fi
	
        if [ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "TG4482A" ] || [ "$MODEL_NUM" = "CGM4140COM" ] || [ "$MODEL_NUM" = "CGM4331COM" ]; then
          HOME_LAN_ISOLATION=`fast_psmcli get dmsb.l2net.HomeNetworkIsolation`
          if [ "$HOME_LAN_ISOLATION" = "0" ];then
              #ARRISXB6-9443 temp fix. Need to generalize and improve.
                  if [ "x$ovs_enable" = "xtrue" ];then
                      ovs-vsctl list-ifaces brlan0 |grep "moca0" >> /dev/null
                  else
                      brctl show brlan0 | grep "moca0" >> /dev/null
                  fi
                  if [ $? -ne 0 ] ; then
                      echo_t "Moca is not part of brlan0.. adding it"
                      t2CountNotify "SYS_SH_MOCA_add_brlan0"
                      sysevent set multinet-syncMembers 1
                  fi

          else
                  if [ "x$ovs_enable" = "xtrue" ];then
                      ovs-vsctl list-ifaces brlan10 |grep "moca0" >> /dev/null
                  else
                      brctl show brlan10 | grep "moca0" >> /dev/null
                  fi
                  if [ $? -ne 0 ] ; then
                      echo_t "Moca is not part of brlan10.. adding it"
                      #t2CountNotify "SYS_SH_MOCA_add_brlan10"
                      sysevent set multinet-syncMembers 9
                  fi
          fi
	fi
    ;;
esac

BR_MODE=0
bridgeMode=$(dmcli eRT retv Device.X_CISCO_COM_DeviceControl.LanManagementEntry.1.LanMode)
# RDKB-6895
if [ "$bridgeMode" != "" ]; then
        if [ "$bridgeMode" != "router" ]; then
        	BR_MODE=1
                echo_t "[RDKB_SELFHEAL] : Device in bridge mode"
                if [ "$MODEL_NUM" = "CGA4332COM" ] || [ "$MODEL_NUM" = "CGA4131COM" ]; then
        		Bridge_Mode_Type=$(echo "$bridgeMode" | grep -oE "(full-bridge-static|bridge-static)")
        		if [ "$Bridge_Mode_Type" = "full-bridge-static" ]; then
            			echo_t "[RDKB_SELFHEAL] : Device in Basic Bridge mode"
        		elif [ "$Bridge_Mode_Type" = "bridge-static" ]; then
            			echo_t "[RDKB_SELFHEAL] : Device in Advanced Bridge mode"
        		fi
		fi
        fi
else
    echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking bridge mode."
    t2CountNotify "SYS_ERROR_DmCli_Bridge_mode_error"
    echo_t "LanMode dmcli called failed with error $bridgeMode"
    isBridging=$(get_from_syscfg_cache bridge_mode)
    if [ "$isBridging" != "0" ]; then
        BR_MODE=1
        echo_t "[RDKB_SELFHEAL] : Device in bridge mode"
        if [ "$MODEL_NUM" = "CGA4332COM" ] || [ "$MODEL_NUM" = "CGA4131COM" ]; then
        	if [ "$isBridging" = "3" ]; then
            		echo_t "[RDKB_SELFHEAL] : Device in Basic Bridge mode"
        	elif [ "$isBridging" = "2" ]; then
            		echo_t "[RDKB_SELFHEAL] : Device in Advanced Bridge mode"
       		fi
        fi
    fi

    case $SELFHEAL_TYPE in
        "BASE"|"TCCBR")
            pandm_timeout=$(echo "$bridgeMode" | grep "$CCSP_ERR_TIMEOUT")
            pandm_notexist=$(echo "$bridgeMode" | grep "$CCSP_ERR_NOT_EXIST")
            pandm_notconnect=$(echo "$bridgeMode" | grep "$CCSP_ERR_NOT_CONNECT")
            if [ "$pandm_timeout" != "" ] || [ "$pandm_notexist" != "" ] || [ "$pandm_notconnect" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : pandm parameter timed out or failed to return"

                pam_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name)
                cr_timeout=$(echo "$pam_name" | grep "$CCSP_ERR_TIMEOUT")
                cr_pam_notexist=$(echo "$pam_name" | grep "$CCSP_ERR_NOT_EXIST")
                cr_pam_notconnect=$(echo "$pam_name" | grep "$CCSP_ERR_NOT_CONNECT")
                if [ "$cr_timeout" != "" ] || [ "$cr_pam_notexist" != "" ] || [ "$cr_pam_notconnect" != "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : pandm process is not responding. Restarting it"
                    t2CountNotify "SYS_ERROR_PnM_Not_Responding"
                    PANDM_PID=$(busybox pidof CcspPandMSsp)
                    if [ "$PANDM_PID" != "" ]; then
                        kill -9 "$PANDM_PID"
                    fi
                    case $SELFHEAL_TYPE in
                        "BASE"|"TCCBR")
                            rm -rf /tmp/pam_initialized
                            resetNeeded pam CcspPandMSsp
                        ;;
                        "SYSTEMD")
                        ;;
                    esac
                fi  # [ "$cr_timeout" != "" ] || [ "$cr_pam_notexist" != "" ]
            fi  # [ "$pandm_timeout" != "" ] || [ "$pandm_notexist" != "" ]
        ;;
        "SYSTEMD")
            pandm_timeout=$(echo "$bridgeMode" | grep "$CCSP_ERR_TIMEOUT")
            if [ "$pandm_timeout" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : pandm parameter time out"

                pam_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name)
                cr_timeout=$(echo "$pam_name" | grep "$CCSP_ERR_TIMEOUT")
                if [ "$cr_timeout" != "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : pandm process is not responding. Restarting it"
                    t2CountNotify "SYS_ERROR_PnM_Not_Responding"
                    PANDM_PID=$(busybox pidof CcspPandMSsp)
                    rm -rf /tmp/pam_initialized
                    systemctl restart CcspPandMSsp.service
                fi
            else
                echo "$bridgeMode"
            fi
        ;;
    esac
fi  # [ "$bridgeSucceed" != "" ]

# Checking PSM's PID
PSM_PID=$(get_pid PsmSsp)
if [ "$PSM_PID" = "" ]; then
    case $SELFHEAL_TYPE in
        "BASE"|"TCCBR")
            #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
            #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : PSM_process is not running, need to reboot the unit"
            #       vendor=$(getVendorName)
            #       modelName=$(getModelName)
            #       CMMac=$(getCMMac)
            #       timestamp=$(getDate)
            #       echo "[$(getDateTime)] Setting Last reboot reason"
            #       dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string Psm_crash
            #       dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
            #       echo "[$(getDateTime)] SET succeeded"
            #       echo "[$(getDateTime)] RDKB_SELFHEAL : <$level>CABLEMODEM[$vendor]:<99000007><$timestamp><$CMMac><$modelName> RM PsmSsp process died,need reboot"
            #       touch $HAVECRASH
            #       rebootNeeded RM "PSM"

            if [ ! -f "$PSM_SHUTDOWN" ];then
                echo_t "RDKB_PROCESS_CRASHED : PSM_process is not running, need restart"
                resetNeeded psm PsmSsp
            fi
        ;;
        "SYSTEMD")
        ;;
    esac
else
    psm_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.psm.Name)
    psm_name_timeout=$(echo "$psm_name" | grep "$CCSP_ERR_TIMEOUT")
    psm_name_notexist=$(echo "$psm_name" | grep "$CCSP_ERR_NOT_EXIST")
    psm_name_notconnect=$(echo "$psm_name" | grep "$CCSP_ERR_NOT_CONNECT")
    if [ "$psm_name_timeout" != "" ] || [ "$psm_name_notexist" != "" ] || [ "$psm_name_notconnect" != "" ]; then
        psm_health=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.psm.Health)
        psm_health_timeout=$(echo "$psm_health" | grep "$CCSP_ERR_TIMEOUT")
        psm_health_notexist=$(echo "$psm_health" | grep "$CCSP_ERR_NOT_EXIST")
        psm_health_notconnect=$(echo "$psm_health" | grep "$CCSP_ERR_NOT_CONNECT")
        if [ "$psm_health_timeout" != "" ] || [ "$psm_health_notexist" != "" ] || [ "$psm_health_notconnect" != "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : PSM_process is in hung state, need restart"
            t2CountNotify "SYS_SH_PSMHung"
            case $SELFHEAL_TYPE in
                "BASE"|"TCCBR")
                    kill -9 "$(get_pid PsmSsp)"
                    resetNeeded psm PsmSsp
                ;;
                "SYSTEMD")
                    systemctl restart PsmSsp.service
                ;;
            esac
        fi
    fi
fi

case $SELFHEAL_TYPE in
    "BASE")

        check_component_status

        if [ "$WAN_TYPE" = "EPON" ]; then
            #Checking EPONAgent is running.
            EPON_AGENT_PID=$(get_pid CcspEPONAgentSsp)
            if [ "$EPON_AGENT_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : EPON_process is not running, need restart"
                resetNeeded epon CcspEPONAgentSsp
            fi
        fi

        LM_PID=$(get_pid CcspLMLite)
        if [ "$LM_PID" != "" ]; then
            lmlite_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.lmlite.Name)
            cr_timeout=$(echo "$lmlite_name" | grep "$CCSP_ERR_TIMEOUT")
            cr_lmlite_notexist=$(echo "$lmlite_name" | grep "$CCSP_ERR_NOT_EXIST")
            if [ "$cr_timeout" != "" ] || [ "$cr_lmlite_notexist" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : LMlite process is not responding. Restarting it"
                kill -9 "$LM_PID"
                resetNeeded lm CcspLMLite
            fi
        fi

        # Checking XdnsSsp PID
        XDNS_PID=$(get_pid CcspXdnsSsp)
        if [ "$XDNS_PID" = "" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
            echo_t "RDKB_PROCESS_CRASHED : CcspXdnsSsp_process is not running, need restart"
            resetNeeded xdns CcspXdnsSsp
        fi

        # Checking snmp v2 subagent PID
        if [ -f "/etc/SNMP_PA_ENABLE" ] && [ "$BOX_TYPE" != "MV2PLUS" ] && [ "$BOX_TYPE" != "MV3" ]; then
            SNMP_PID=$(grep "snmp_subagent" $PROCESSES_CACHE | grep -v "cm_snmp_ma_2" | grep -v "grep" | awk '{print $2}')
            if [ "$SNMP_PID" = "" ]; then
                if [ -f /tmp/.snmp_agent_restarting ]; then
                    echo_t "[RDKB_SELFHEAL] : snmp process is restarted through maintanance window"
                else
                    SNMPv2_RDKB_MIBS_SUPPORT=$(get_from_syscfg_cache V2Support)
                    if [ "$SNMPv2_RDKB_MIBS_SUPPORT" = "true" ] || [ "$SNMPv2_RDKB_MIBS_SUPPORT" = "" ]; then
                        echo_t "RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
                        t2CountNotify "SYS_SH_SNMP_NotRunning"
                        resetNeeded snmp snmp_subagent
                    fi
                fi
            fi
        fi

        # Checking CcspMoCA PID
        # MOCA_PID=$(get_pid CcspMoCA)
        # if [ "$MOCA_PID" = "" ]; then
        #     echo_t "RDKB_PROCESS_CRASHED : CcspMoCA process is not running, need restart"
        #     resetNeeded moca CcspMoCA
        # fi

        if [ "$MODEL_NUM" = "DPC3939" ] || [ "$MODEL_NUM" = "DPC3941" ]; then
            # Checking mocadlfw PID
            MOCADLFW_PID=$(get_pid mocadlfw)
            if [ "$MOCADLFW_PID" = "" ]; then
                echo_t "OEM_PROCESS_MOCADLFW_CRASHED : mocadlfw process is not running, need restart"
                /usr/sbin/mocadlfw > /dev/null 2>&1 &
            fi
        fi

	# BWGRDK-1384: Selfheal mechanism for ripd process
	if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
	    staticIp_check=$(fast_psmcli get dmsb.truestaticip.Enable)
            if [ "$staticIp_check" = "1" ]; then
		ripdPid=$(get_pid ripd)
		if [ -z "$ripdPid" ]; then
                    echo_t "RDKB_SELFHEAL : ripd process is not running, need restart"
                    /usr/sbin/ripd -d -f /var/ripd.conf -u root -g root -i /var/ripd.pid &
		fi
	    fi
	fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        case $BOX_TYPE in
            "HUB4"|"SR300"|"SE501"|"SR213"|"WNXL11BWL")
                Harvester_PID=$(get_pid harvester)
                if [ "$Harvester_PID" != "" ]; then
                    Harvester_CPU=$(top -bn1 | grep "harvester" | grep -v "grep" | head -n5 | awk -F'%' '{print $2}' | sed -e 's/^[ \t]*//' | awk '{$1=$1};1')
                    if [ "$Harvester_CPU" != "" ] && [ $Harvester_CPU -ge 30 ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : harvester process is hung and taking $Harvester_CPU% CPU, restarting it"
                        systemctl restart harvester.service
                    fi
                fi
            ;;
        esac
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
     if [ "$FIRMWARE_TYPE" = "OFW" ] && [ "$BOX_TYPE" != "MV1" ]; then
        WiFi_PID=$(get_pid CcspWifiSsp)
        if [ "$WiFi_PID" != "" ]; then
            wifi_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.wifi.Health)
            wifi_name_timeout=$(echo "$wifi_name" | grep "$CCSP_ERR_TIMEOUT")
            wifi_name_notexist=$(echo "$wifi_name" | grep "$CCSP_ERR_NOT_EXIST")
            if [ "$wifi_name_timeout" != "" ] || [ "$wifi_name_notexist" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : CcspWifiSsp process is hung , restarting it"
                kill -9 $WIFI_PID
                resetNeeded wifi CcspWifiSsp
            fi
        fi
    fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        OS_WANMANGR_ENABLE_FILE=/tmp/OS_WANMANAGER_ENABLED
        OS_WANMANGR_DIR=/usr/rdk/wanmanager/wanmanager

        if [ -f $OS_WANMANGR_ENABLE_FILE ];then
            echo_t "os-wanmanager enabled"
            if [ -f $OS_WANMANGR_DIR ];then
                # Checking wanmanager's PID
                WANMANAGER_PID=$(busybox pidof wanmanager)
                if [ "$WANMANAGER_PID" = "" ]; then
                    echo_t "RDKB_PROCESS_CRASHED : WANMANAGER_process is not running, need CPE reboot"
                    t2CountNotify "SYS_ERROR_wanmanager_crash_reboot"
                    reason="wanmanager_crash"
                    rebootCount=1
                    rebootNeeded RM "WANMANAGER" $reason $rebootCount
                fi
            fi
        fi
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        WiFi_Flag=false
        WiFi_PID=$(get_pid CcspWifiSsp)
        if [ "$WiFi_PID" != "" ]; then
            radioenable=$(fast_dmcli eRT getv Device.WiFi.Radio.1.Enable)
            radioenable_timeout=$(echo "$radioenable" | grep "$CCSP_ERR_TIMEOUT")
            radioenable_notexist=$(echo "$radioenable" | grep "$CCSP_ERR_NOT_EXIST")
            if [ "$radioenable_timeout" != "" ] || [ "$radioenable_notexist" != "" ]; then
                wifi_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.wifi.Name)
                wifi_name_timeout=$(echo "$wifi_name" | grep "$CCSP_ERR_TIMEOUT")
                wifi_name_notexist=$(echo "$wifi_name" | grep "$CCSP_ERR_NOT_EXIST")
                if [ "$wifi_name_timeout" != "" ] || [ "$wifi_name_notexist" != "" ]; then
                    if [ "$BOX_TYPE" = "XB6" ]; then
                        if [ -f "$thisREADYFILE" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : CcspWifiSsp process is hung , restarting it"
                            systemctl restart ccspwifiagent
                            WiFi_Flag=true
                            t2CountNotify "WIFI_SH_CcspWifiHung_restart"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : CcspWifiSsp process is hung , restarting it"
                        t2CountNotify "WIFI_SH_CcspWifiHung_restart"
                        systemctl restart ccspwifiagent
                        WiFi_Flag=true
                    fi
                fi
            fi
        fi
    ;;
esac

if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ] || [ "$MODEL_NUM" = "" ] || [ "$FIRMWARE_TYPE" = "OFW" ]; then
    echo_t "[RDKB_SELFHEAL] : Disabling CcpsHomeSecurity and CcspAdvSecurity"
else
    if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ]  && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then

        case $SELFHEAL_TYPE in
            "BASE"|"SYSTEMD")

                HOMESEC_PID=$(get_pid CcspHomeSecurity)
                if [ "$HOMESEC_PID" = "" ]; then
                    case $SELFHEAL_TYPE in
                        "BASE")
                            echo_t "RDKB_PROCESS_CRASHED : HomeSecurity_process is not running, need restart"
                            t2CountNotify "SYS_SH_HomeSecurity_restart"
                        ;;
                        "TCCBR")
                        ;;
                        "SYSTEMD")
                            echo_t "RDKB_PROCESS_CRASHED : HomeSecurity process is not running, need restart"
                            t2CountNotify "SYS_SH_HomeSecurity_restart"
                        ;;
                    esac
                    resetNeeded "" CcspHomeSecurity
                fi
	esac

    fi #Not HUb4

                isADVPID=0
                case $SELFHEAL_TYPE in
                    "BASE")
                        # CcspAdvSecurity
                        ADV_PID=$(get_pid CcspAdvSecuritySsp)
                        if [ "$ADV_PID" = "" ] ; then
                            echo_t "RDKB_PROCESS_CRASHED : CcspAdvSecurity_process is not running, need restart"
                            resetNeeded advsec CcspAdvSecuritySsp
                            isADVPID=1
                        fi
                    ;;
                    "TCCBR")
                    ;;
                    "SYSTEMD")
                    ;;
                esac
                advsec_bridge_mode=$(get_from_syscfg_cache bridge_mode)
                DF_ENABLED=$(get_from_syscfg_cache Advsecurity_DeviceFingerPrint)
                if [ "$advsec_bridge_mode" != "2" ]; then
                    if [ -f $ADVSEC_PATH ]; then
                        if [ $isADVPID -eq 0 ] && [ "$DF_ENABLED" = "1" ]; then
                            if [ "x$(advsec_is_agent_installed)" == "xYES" ]; then
                                if [ ! -f $ADVSEC_INITIALIZING ]; then
                                    ADV_AGENT_PID=$(advsec_is_alive ${CUJO_AGENT})
                                    if [ "${ADV_AGENT_PID}" = "" ] ; then
                                        if  [ ! -e ${ADVSEC_AGENT_SHUTDOWN} ]; then
                                            echo_t "RDKB_PROCESS_CRASHED : AdvSecurity ${CUJO_AGENT_LOG} process is not running, need restart"
                                        fi
                                        resetNeeded advsec_bin AdvSecurityAgent
                                    fi
                                fi
                            fi
                        fi
                    else
                        case $SELFHEAL_TYPE in
                            "BASE")
                                if [ "$MODEL_NUM" = "DPC3941" ]; then
                                    /usr/sbin/cujo_download.sh &
                                fi
                            ;;
                            "TCCBR")
                            ;;
                            "SYSTEMD")
                            ;;
                        esac
                    fi  # [ -f $ADVSEC_PATH ]
                fi  # [ "$advsec_bridge_mode" != "2" ]
fi #BWG
case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
        dmesg -n 8
        atomOnlyReboot=$(dmesg | grep -i "Atom only")
        dmesg -n 5
        if [ "x$atomOnlyReboot" = "x" ]; then
            cr_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.CR.Name)
            isCRAlive=$(echo "$cr_name" | grep "Can't find destination compo")
            if [ "$isCRAlive" != "" ]; then
                # Retest by querying some other parameter
                crReTestop=$(fast_dmcli eRT getv Device.X_CISCO_COM_DeviceControl.DeviceMode)
                isCRAlive=$(echo "$crReTestop" | grep "Can't find destination compo")
                if [ "$isCRAlive" != "" ]; then
                    #echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
                    echo_t "RDKB_PROCESS_CRASHED : CR_process is not running, need to reboot the unit"
                    vendor=$(getVendorName)
                    modelName=$(getModelName)
                    CMMac=$(getCMMac)
                    timestamp=$(getDate)
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


        PAM_PID=$(busybox pidof CcspPandMSsp)
        if [ "$PAM_PID" = "" ]; then
            # Remove the P&M initialized flag
            rm -rf /tmp/pam_initialized
            echo_t "RDKB_PROCESS_CRASHED : PAM_process is not running, need restart"
            resetNeeded pam CcspPandMSsp
            t2CountNotify "SYS_SH_PAM_CRASH_RESTART"
        fi

        # Checking MTA's PID
        MTA_PID=$(get_pid CcspMtaAgentSsp)
        if [ "$MTA_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : MTA_process is not running, need restart"
            resetNeeded mta CcspMtaAgentSsp
            t2CountNotify "SYS_SH_MTA_restart"
        fi

        WiFi_Flag=false
        # Checking Wifi's PID
        WIFI_PID=$(get_pid CcspWifiSsp)
        if [ "$WIFI_PID" = "" ]; then
            # Remove the wifi initialized flag
            rm -rf /tmp/wifi_initialized
            echo_t "RDKB_PROCESS_CRASHED : WIFI_process is not running, need restart"
            resetNeeded wifi CcspWifiSsp
        else
            radioenable=$(fast_dmcli eRT getv Device.WiFi.Radio.1.Enable)
            radioenable_timeout=$(echo "$radioenable" | grep "$CCSP_ERR_TIMEOUT")
            radioenable_notexist=$(echo "$radioenable" | grep "$CCSP_ERR_NOT_EXIST")
            if [ "$radioenable_timeout" != "" ] || [ "$radioenable_notexist" != "" ]; then
                wifi_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.wifi.Name)
                wifi_name_timeout=$(echo "$wifi_name" | grep "$CCSP_ERR_TIMEOUT")
                wifi_name_notexist=$(echo "$wifi_name" | grep "$CCSP_ERR_NOT_EXIST")
                if [ "$wifi_name_timeout" != "" ] || [ "$wifi_name_notexist" != "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : CcspWifiSsp process is hung , restarting it"
                    t2CountNotify "WIFI_SH_CcspWifiHung_restart"
                    # Remove the wifi initialized flag
                    rm -rf /tmp/wifi_initialized

                    #TCCBR-4286 Workaround for wifi process hung
                    sh $TAD_PATH/oemhooks.sh "CCSP_WIFI_HUNG"

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
        CM_PID=$(get_pid CcspCMAgentSsp)
        if [ "$CM_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : CM_process is not running, need restart"
            resetNeeded cm CcspCMAgentSsp
        fi

        # Checking WEBController's PID
        #   WEBC_PID=$(busybox pidof CcspWecbController)
        #   if [ "$WEBC_PID" = "" ]; then
        #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : WECBController_process is not running, restarting it"
        #       echo_t "RDKB_PROCESS_CRASHED : WECBController_process is not running, need restart"
        #       resetNeeded wecb CcspWecbController
        #   fi

        # Checking RebootManager's PID
        #   Rm_PID=$(busybox pidof CcspRmSsp)
        #   if [ "$Rm_PID" = "" ]; then
        #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : RebootManager_process is not running, restarting it"
        #       echo "[$(getDateTime)] RDKB_PROCESS_CRASHED : RebootManager_process is not running, need restart"
        #       resetNeeded "rm" CcspRmSsp

        #   fi

        # Checking Test adn Daignostic's PID
        TandD_PID=$(get_pid CcspTandDSsp)
        if [ "$TandD_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : TandD_process is not running, need restart"
            resetNeeded tad CcspTandDSsp
        fi

        # Checking Lan Manager PID
        LM_PID=$(get_pid CcspLMLite)
        if [ "$LM_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : LanManager_process is not running, need restart"
            t2CountNotify "SYS_SH_LM_restart"
            resetNeeded lm CcspLMLite

        fi

        # Checking XdnsSsp PID
        XDNS_PID=$(get_pid CcspXdnsSsp)
        if [ "$XDNS_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : CcspXdnsSsp_process is not running, need restart"
            resetNeeded xdns CcspXdnsSsp

        fi

        # Checking CcspEthAgent PID
        ETHAGENT_PID=$(get_pid CcspEthAgent)
        if [ "$ETHAGENT_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : CcspEthAgent_process is not running, need restart"
            resetNeeded ethagent CcspEthAgent

        fi

        # Checking snmp subagent PID
        if [ -f "/etc/SNMP_PA_ENABLE" ]; then
            SNMP_PID=$(get_pid snmp_subagent)
            if [ "$SNMP_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : snmp process is not running, need restart"
                t2CountNotify "SYS_SH_SNMP_NotRunning"
                resetNeeded snmp snmp_subagent
            fi
        fi

        # Checking harvester PID
        HARVESTER_PID=$(get_pid harvester)
        if [ "$HARVESTER_PID" = "" ]; then
            echo_t "RDKB_PROCESS_CRASHED : harvester is not running, need restart"
            resetNeeded harvester harvester
        fi
    ;;
    "SYSTEMD")
    ;;
esac

if [ "$(fast_psmcli get dmsb.hotspot.enable)" = "1" ]; then
    HOTSPOT_ENABLE="true"
else
    HOTSPOT_ENABLE="false"
fi

if [ "$BOX_TYPE" = "MV1" ] && [ "$HOTSPOT_ENABLE" = "true" ]; then
    count=0
    for i in 5 6; do
        APENABLE=$(get_ssid_enable $i)
        if [ "$APENABLE" = "true" ]; then
            # Accounting is enabled always hence, double the expected relay process count
            count=$((count+2))
        fi
    done

    RELAYPID=$(get_pid radius_relay)
    relay_count=0
    for j in $RELAYPID; do
        relay_count=$((relay_count+1))
    done

    echo "radius process: actual count=$relay_count expected count=$count"

    if [ "$count" -gt "$relay_count" ] ; then
        echo_t "RDKB_SELFHEAL : radius relay not running for all the instance, needs a restart"
        sysevent set radiusrelay-restart
    fi
fi

if [ "$thisWAN_TYPE" != "EPON" ] && [ "$HOTSPOT_ENABLE" = "true" ]; then
    DHCP_ARP_PID=$(get_pid hotspot_arpd)
    if [ "$DHCP_ARP_PID" = "" ] && [ -f /tmp/hotspot_arpd_up ] && [ ! -f /tmp/tunnel_destroy_flag ] ; then
        echo_t "RDKB_PROCESS_CRASHED : DhcpArp_process is not running, need restart"
        t2CountNotify "SYS_SH_DhcpArpProcess_restart"
        resetNeeded "" hotspot_arpd
    fi

    HOTSPOT_PID=$(get_pid CcspHotspot)
    if [ "$HOTSPOT_PID" = "" ]; then
        if [ ! -f /tmp/tunnel_destroy_flag ] ; then

            primary=$(sysevent get hotspotfd-primary)
            secondary=$(sysevent get hotspotfd-secondary)
            PRIMARY_EP=$(dmcli eRT retv Device.X_COMCAST-COM_GRE.Tunnel.1.PrimaryRemoteEndpoint)
            SECOND_EP=$(dmcli eRT retv Device.X_COMCAST-COM_GRE.Tunnel.1.SecondaryRemoteEndpoint)
            if [ "$primary" = "" ] ; then
               echo_t "Primary endpoint is empty. Restoring it"
               sysevent set hotspotfd-primary $PRIMARY_EP
            fi

            if [ "$secondary" = "" ] ; then
               echo_t "Secondary endpoint is empty. Restoring it"
               sysevent set hotspotfd-secondary $SECOND_EP
            fi
            resetNeeded "" CcspHotspot
        fi
    fi
fi

case $SELFHEAL_TYPE in
    "BASE")
        if [ "$WAN_TYPE" != "EPON" ] && [ "$HOTSPOT_ENABLE" = "true" ]; then
            rcount=0
            OPEN_24=$(get_ssid_enable 5)
            OPEN_5=$(get_ssid_enable 6)

            # ----------------------------------------------------------------
            if [ "$BOX_TYPE" = "MV1" ]; then
            # ----------------------------------------------------------------

                l2net_24_VLANID=`fast_psmcli get dmsb.l2net.3.Port.2.Pvid`
                l2net_5_VLANID=`fast_psmcli get dmsb.l2net.3.Port.3.Pvid`
                GRE_VLANID=`fast_psmcli get dmsb.l2net.3.Port.4.Pvid`

                Interface=`fast_psmcli get dmsb.l2net.3.Members.WiFi`
                if [ "$Interface" = "" ]; then
                    echo_t "PSM value(wifi if) is missing for $l2sd0Prefix.$l2net_24_VLANID and $l2sd0Prefix.$l2net_24_VLANID"
                    psmcli set dmsb.l2net.3.Members.WiFi "cei02 wdev0ap2"
                fi
                if [ "0" = "$GRE_VLANID" ]; then
                    GREIF="$grePrefix"
                else
                    GREIF="$grePrefix.$GRE_VLANID"
                fi

                if [ "$OPEN_24" = "true" ] || [ "$OPEN_5" = "true" ]; then

                    grePresent=$(ifconfig -a | grep "$GREIF")
                    if [ "$grePresent" = "" ]; then
                        #l2sd0.102 case
                        ifconfig | grep "$l2sd0Prefix\.$l2net_24_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "ComunityWifi is enabled, but $l2sd0Prefix.$l2net_24_VLANID interface is not created try creating it"

                            sysevent set multinet_3-status stopped
                            $UTOPIA_PATH/service_multinet_exec multinet-start 3
                            ifconfig $l2sd0Prefix.$l2net_24_VLANID up
                            ifconfig | grep "$l2sd0Prefix\.$l2net_24_VLANID"
                            if [ $? -eq 1 ]; then
                                echo_t "$l2sd0Prefix.$l2net_24_VLANID is not created at First Retry, try again after 2 sec"
                                sleep 2
                                sysevent set multinet_3-status stopped
                                $UTOPIA_PATH/service_multinet_exec multinet-start 3
                                ifconfig $l2sd0Prefix.$l2net_24_VLANID up
                                ifconfig | grep "$l2sd0Prefix\.$l2net_24_VLANID"
                                if [ $? -eq 1 ]; then
                                    echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$l2net_24_VLANID is not created after Second Retry, no more retries !!!"
                                fi
                            else
                                echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$l2net_24_VLANID created at First Retry itself"
                            fi
                        else
                            echo_t "[RDKB_PLATFORM_ERROR] :CommunityWifi:  SSID 2.4GHz is enabled but gre tunnels not present, restoring it"
                            t2CountNotify "SYS_ERROR_GRETunnel_restored"
                            rcount=1
                        fi

                        #l2sd0.103 case
                        ifconfig | grep "$l2sd0Prefix\.$l2net_5_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "XfinityWifi is enabled, but $l2sd0Prefix.$l2net_5_VLANID interface is not created try creatig it"

                            sysevent set multinet_4-status stopped
                            $UTOPIA_PATH/service_multinet_exec multinet-start 3
                            ifconfig $l2sd0Prefix.$l2net_5_VLANID up
                            ifconfig | grep "$l2sd0Prefix\.$l2net_5_VLANID"
                            if [ $? -eq 1 ]; then
                                echo_t "$l2sd0Prefix.$l2net_5_VLANID is not created at First Retry, try again after 2 sec"
                                sleep 2
                                sysevent set multinet_4-status stopped
                                $UTOPIA_PATH/service_multinet_exec multinet-start 3
                                ifconfig $l2sd0Prefix.$l2net_5_VLANID up
                                ifconfig | grep "$l2sd0Prefix\.$l2net_5_VLANID"
                                if [ $? -eq 1 ]; then
                                    echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$l2net_5_VLANID is not created after Second Retry, no more retries !!!"
                                fi
                            else
                                echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$l2net_5_VLANID created at First Retry itself"
                            fi
                        else
                            echo_t "[RDKB_PLATFORM_ERROR] :CommunityWifi:  SSID 5 GHz is enabled but gre tunnels not present, restoring it"
                            t2CountNotify "SYS_ERROR_GRETunnel_restored"
                            rcount=1
                        fi
                    fi
                fi # [ "$OPEN_24" = "true" ] || [ "$OPEN_5" = "true" ]

                #We need to make sure Community hotspot Vlan IDs are attached to the bridges
                #if found not attached , then add the device to bridges

                grePresent=$(ifconfig -a | grep "$GREIF")
                if [ -n "$grePresent" ]; then
                    vlanAdded=$(brctl show brlan2 | grep "$GREIF")
                    if [ "$vlanAdded" = "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Vlan not added $GREIF"
                        brctl addif brlan2 $GREIF
                    fi
                fi

                l2netPresent=$(ifconfig -a | grep "$l2sd0Prefix\.$l2net_24_VLANID")
                if [ -n "$l2netPresent" ]; then
                    vlanAdded=$(brctl show brlan2 | grep "$l2sd0Prefix\.$l2net_24_VLANID")
                    if [ "$vlanAdded" = "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Vlan not added $l2sd0Prefix.$l2net_24_VLANID"
                        brctl addif brlan2 $l2sd0Prefix.$l2net_24_VLANID
                    fi
                fi
                l2netPresent=$(ifconfig -a | grep "$l2sd0Prefix\.$l2net_5_VLANID")
                if [ -n "$l2netPresent" ]; then
                    vlanAdded=$(brctl show brlan2 | grep "$l2sd0Prefix\.$l2net_5_VLANID")
                    if [ "$vlanAdded" = "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Vlan not added $l2sd0Prefix.$l2net_5_VLANID"
                        brctl addif brlan2 $l2sd0Prefix.$l2net_5_VLANID
                    fi
                fi

            # ----------------------------------------------------------------
            elif [ "$BOX_TYPE" = "MV2PLUS" ]; then 
            # ----------------------------------------------------------------
          
                GRE_VLANID=`fast_psmcli get dmsb.hotspot.tunnel.1.interface.1.VLANID`

                if [ "0" = "$GRE_VLANID" ]; then
                    GREIF="$grePrefix"
                else
                    GREIF="$grePrefix.$GRE_VLANID"
                fi

                if [ "$OPEN_24" = "true" ] || [ "$OPEN_5" = "true" ]; then
                    #We need to make sure Community hotspot Vlan IDs are attached to the bridges
                    #if found not attached , then add the device to bridges

                    grePresent=$(ifconfig -a | grep "$GREIF")
                    grePresentWithoutVlan=$(ifconfig -a | grep "$grePrefix")

                    if [ -n "$grePresent" ]; then
                        tunnelAdded=$(brctl show brlan2 | grep "$GREIF")
                        if [ "$tunnelAdded" = "" ]; then
                            tunnelAddedWithoutVlan=$(brctl show brlan2 | grep "$grePrefix")
                            if [ -n "$tunnelAddedWithoutVlan" ] && [ -n "$GRE_VLANID" ]; then
                                echo_t "[RDKB_PLATFORM_ERROR] : GRE tunnel is created without vlan"
                                sysevent set update-vlanID 1
                            else
                                echo_t "[RDKB_PLATFORM_ERROR] : Vlan not added $GREIF"
                                brctl addif brlan2 $GREIF
                            fi
                        fi
                    elif [ -n "$grePresentWithoutVlan" ] && [ -n "$GRE_VLANID" ]; then
                          echo_t "[RDKB_PLATFORM_ERROR] : GRE Tunnel is present without VLAN"
                          sysevent set update-vlanID 1
                    else
                          HOTSPOT_PROCESS_ID=$(get_pid CcspHotspot)
                          primary=$(sysevent get hotspotfd-primary)
                          if [ -n "$HOTSPOT_PROCESS_ID" ] && [ -n "$primary" ] ; then
                              echo_t " [RDKB_PLATFORM_ERROR] : Hotspot process is up but tunnel got destroyed " 
                              sysevent set gre-forceRestart 1
                          fi 
                    fi		       
                fi
                if [ "$OPEN_24" = "true" ]; then
                    wifiInterface24=$(get_ssid_name 5)
                    if [ -n "$wifiInterface24" ]; then
                        wl02Present=$(brctl show brlan2 | grep "$wifiInterface24")
                        if [ "$wl02Present" = "" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : WiFi interface not added to hotspot bridge $wifiInterface24 "
                            brctl addif brlan2 $wifiInterface24
                        fi
                    fi
                fi
                if [ "$OPEN_5" = "true" ]; then
                    wifiInterface5=$(get_ssid_name 6)
                    if [ -n "$wifiInterface5" ]; then
                        wl12Present=$(brctl show brlan2 | grep "$wifiInterface5")
                        if [ "$wl12Present" = "" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : WiFi interface is not added to hotspot bridge $wifiInterface5"
                            brctl addif brlan2 $wifiInterface5
                        fi
                    fi
                fi
                
            # ----------------------------------------------------------------
            else
            # ----------------------------------------------------------------

            #When Xfinitywifi is enabled, l2sd0.102 and l2sd0.103 should be present.
            #If they are not present below code shall re-create them
            #l2sd0.102 case , also adding a strict rule that they are up, since some
            #devices we observed l2sd0 not up

            if [ "$MODEL_NUM" = "DPC3939B" ]; then
		Xfinity_Open_24_VLANID="2312"
		Xfinity_Open_5_VLANID="2315"
		Xfinity_Secure_24_VLANID="2311"
		Xfinity_Secure_5_VLANID="2314"
		Xfinity_Public_5_VLANID="2316"
	    elif [ "$MODEL_NUM" = "DPC3941B" ]; then
		Xfinity_Open_24_VLANID="2322"
		Xfinity_Open_5_VLANID="2325"
		Xfinity_Secure_24_VLANID="2321"
		Xfinity_Secure_5_VLANID="2324"
		Xfinity_Public_5_VLANID="2326"
	    else
		Xfinity_Open_24_VLANID="102"
		Xfinity_Open_5_VLANID="103"
		Xfinity_Secure_24_VLANID="104"
		Xfinity_Secure_5_VLANID="105"
	    fi
        if [ "$OPEN_24" = "true" ]; then
            grePresent=$(ifconfig -a | grep "$grePrefix\.$Xfinity_Open_24_VLANID")
			if [ "x$ovs_enable" = "xtrue" ];then
				brPresent=$(ovs-vsctl show | grep "brlan2")
			else
				brPresent=$(brctl show | grep "brlan2")
			fi
            
            if [ "$grePresent" = "" ] || [ "$brPresent" = "" ] ; then
                ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_24_VLANID"
                if [ $? -eq 1 ]; then
                    echo_t "XfinityWifi is enabled, but $l2sd0Prefix.$Xfinity_Open_24_VLANID interface is not created try creating it"

                    Interface=$(fast_psmcli get dmsb.l2net.3.Members.WiFi)
                    if [ "$Interface" = "" ]; then
                        echo_t "PSM value(ath4) is missing for $l2sd0Prefix.$Xfinity_Open_24_VLANID"
                        psmcli set dmsb.l2net.3.Members.WiFi ath4
                    fi

                    sysevent set multinet_3-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 3
                    ifconfig $l2sd0Prefix.$Xfinity_Open_24_VLANID up
                    ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_24_VLANID"
                    if [ $? -eq 1 ]; then
                        echo_t "$l2sd0Prefix.$Xfinity_Open_24_VLANID is not created at First Retry, try again after 2 sec"
                        sleep 2
                        sysevent set multinet_3-status stopped
                        $UTOPIA_PATH/service_multinet_exec multinet-start 3
                        ifconfig $l2sd0Prefix.$Xfinity_Open_24_VLANID up
                        ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_24_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Open_24_VLANID is not created after Second Retry, no more retries !!!"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Open_24_VLANID created at First Retry itself"
                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] :XfinityWifi:  SSID 2.4GHz is enabled but gre tunnels not present, restoring it"
                    t2CountNotify "SYS_ERROR_GRETunnel_restored"
                    rcount=1
                fi
            fi
            fi

            #l2sd0.103 case
            if [ "$OPEN_5" = "true" ]; then
            grePresent=$(ifconfig -a | grep "$grePrefix\.$Xfinity_Open_5_VLANID")
			if [ "x$ovs_enable" = "xtrue" ];then
				brPresent=$(ovs-vsctl show | grep "brlan3")
			else
				brPresent=$(brctl show | grep "brlan3")
			fi
            
            if [ "$grePresent" = "" ] || [ "$brPresent" = "" ]; then
                ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_5_VLANID"
                if [ $? -eq 1 ]; then
                    echo_t "XfinityWifi is enabled, but $l2sd0Prefix.$Xfinity_Open_5_VLANID interface is not created try creatig it"

                    Interface=$(fast_psmcli get dmsb.l2net.4.Members.WiFi)
                    if [ "$Interface" = "" ]; then
                        echo_t "PSM value(ath5) is missing for $l2sd0Prefix.$Xfinity_Open_5_VLANID"
                        psmcli set dmsb.l2net.4.Members.WiFi ath5
                    fi

                    sysevent set multinet_4-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 4
                    ifconfig $l2sd0Prefix.$Xfinity_Open_5_VLANID up
                    ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_5_VLANID"
                    if [ $? -eq 1 ]; then
                        echo_t "$l2sd0Prefix.$Xfinity_Open_5_VLANID is not created at First Retry, try again after 2 sec"
                        sleep 2
                        sysevent set multinet_4-status stopped
                        $UTOPIA_PATH/service_multinet_exec multinet-start 4
                        ifconfig $l2sd0Prefix.$Xfinity_Open_5_VLANID up
                        ifconfig | grep "$l2sd0Prefix\.$Xfinity_Open_5_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Open_5_VLANID is not created after Second Retry, no more retries !!!"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Open_5_VLANID created at First Retry itself"
                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] :XfinityWifi:  SSID 5 GHz is enabled but gre tunnels not present, restoring it"
                    t2CountNotify "SYS_ERROR_GRETunnel_restored"
                    rcount=1
                fi
            fi
            fi
            #RDKB-16889: We need to make sure Xfinity hotspot Vlan IDs are attached to the bridges
            #if found not attached , then add the device to bridges
            for index in 2 3 4 5
              do
                grePresent=$(ifconfig -a | grep "$grePrefix.10$index")
                if [ -n "$grePresent" ]; then
                    if [ "x$ovs_enable" = "xtrue" ];then
                    	vlanAdded=$(ovs-vsctl show $brlanPrefix$index | grep "$l2sd0Prefix.10$index")
                    else
                    	vlanAdded=$(brctl show $brlanPrefix$index | grep "$l2sd0Prefix.10$index")
                    fi
                    if [ "$vlanAdded" = "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Vlan not added $l2sd0Prefix.10$index"
                        if [ "x$bridgeUtilEnable" = "xtrue" || "x$ovs_enable" = "xtrue" ];then
                        	/usr/bin/bridgeUtils add-port $brlanPrefix$index $l2sd0Prefix.10$index
                        else
                        	brctl addif $brlanPrefix$index $l2sd0Prefix.10$index
                        fi
                    fi
                fi
              done

            SECURED_24=$(get_ssid_enable 9)
            SECURED_5=$(get_ssid_enable 10)

            #Check for Secured Xfinity hotspot briges and associate them properly if
            #not proper
            #l2sd0.103 case
            
            #Secured Xfinity 2.4
            if [ "$SECURED_24" = "true" ]; then
            grePresent=$(ifconfig -a | grep "$grePrefix\.$Xfinity_Secure_24_VLANID")
			if [ "x$ovs_enable" = "xtrue" ];then
				brPresent=$(ovs-vsctl show | grep "brlan4")
			else
				brPresent=$(brctl show | grep "brlan4")
            fi
            if [ "$grePresent" = "" ] || [ "$brPresent" = "" ]; then
                ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_24_VLANID"
                if [ $? -eq 1 ]; then
                    echo_t "XfinityWifi is enabled Secured gre created, but $l2sd0Prefix.$Xfinity_Secure_24_VLANID interface is not created try creatig it"
                    sysevent set multinet_7-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 7
                    ifconfig $l2sd0Prefix.$Xfinity_Secure_24_VLANID up
                    ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_24_VLANID"
                    if [ $? -eq 1 ]; then
                        echo_t "$l2sd0Prefix.$Xfinity_Secure_24_VLANID is not created at First Retry, try again after 2 sec"
                        sleep 2
                        sysevent set multinet_7-status stopped
                        $UTOPIA_PATH/service_multinet_exec multinet-start 7
                        ifconfig $l2sd0Prefix.$Xfinity_Secure_24_VLANID up
                        ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_24_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Secure_24_VLANID is not created after Second Retry, no more retries !!!"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Secure_24_VLANID created at First Retry itself"
                    fi
                else
                #RDKB-17221: In some rare devices we found though Xfinity secured ssid enabled, but it did'nt create the gre tunnels
                #but all secured SSIDs Vaps were up and system remained in this state for long not allowing clients to
                #connect
                    echo_t "[RDKB_PLATFORM_ERROR] :XfinityWifi: Secured SSID 2.4 is enabled but gre tunnels not present, restoring it"
                    t2CountNotify "SYS_ERROR_GRETunnel_restored"
                    rcount=1
                fi
            fi
            fi

            #Secured Xfinity 5
            if [ "$SECURED_5" = "true" ]; then
            grePresent=$(ifconfig -a | grep "$grePrefix\.$Xfinity_Secure_5_VLANID")
			if [ "x$ovs_enable" = "xtrue" ];then
				brPresent=$(ovs-vsctl show | grep "brlan5")
			else
				brPresent=$(brctl show | grep "brlan5")
			fi
            
            if [ "$grePresent" = "" ] || [ "$brPresent" = "" ]; then
                ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_5_VLANID"
                if [ $? -eq 1 ]; then
                    echo_t "XfinityWifi is enabled Secured gre created, but $l2sd0Prefix.$Xfinity_Secure_5_VLANID interface is not created try creatig it"
                    sysevent set multinet_8-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 8
                    ifconfig $l2sd0Prefix.$Xfinity_Secure_5_VLANID up
                    ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_5_VLANID"
                    if [ $? -eq 1 ]; then
                        echo_t "$l2sd0Prefix.$Xfinity_Secure_5_VLANID is not created at First Retry, try again after 2 sec"
                        sleep 2
                        sysevent set multinet_8-status stopped
                        $UTOPIA_PATH/service_multinet_exec multinet-start 8
                        ifconfig $l2sd0Prefix.$Xfinity_Secure_5_VLANID up
                        ifconfig | grep "$l2sd0Prefix\.$Xfinity_Secure_5_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Secure_5_VLANID is not created after Second Retry, no more retries !!!"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Secure_5_VLANID created at First Retry itself"
                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] :XfinityWifi: Secured SSID 5GHz is enabled but gre tunnels not present, restoring it"
                    t2CountNotify "SYS_ERROR_GRETunnel_restored"
                    rcount=1
                fi
            fi
            fi

            #New Public hotspot
            PUBLIC_5=$(get_ssid_enable 16)
            if [ "$PUBLIC_5" = "true" ]; then
            grePresent=$(ifconfig -a | grep "$grePrefix\.$Xfinity_Public_5_VLANID")
			if [ "x$ovs_enable" = "xtrue" ];then
				brPresent=$(ovs-vsctl show | grep "brpub")
			else
				brPresent=$(brctl show | grep "brpub")
			fi
            
            if [ "$grePresent" = "" ] || [ "$brPresent" = "" ]; then
                ifconfig | grep "$l2sd0Prefix\.$Xfinity_Public_5_VLANID"
                if [ $? -eq 1 ]; then
                    echo_t "XfinityWifi is enabled, but $l2sd0Prefix.$Xfinity_Public_5_VLANID interface is not created try creating it"

                    Interface=$(fast_psmcli get dmsb.l2net.11.Members.WiFi)
                    if [ "$Interface" = "" ]; then
                        echo_t "PSM value(ath15) is missing for $l2sd0Prefix.$Xfinity_Public_5_VLANID"
                        psmcli set dmsb.l2net.11.Members.WiFi ath15
                    fi

                    sysevent set multinet_11-status stopped
                    $UTOPIA_PATH/service_multinet_exec multinet-start 11
                    ifconfig $l2sd0Prefix.$Xfinity_Public_5_VLANID up
                    ifconfig | grep "$l2sd0Prefix\.$Xfinity_Public_5_VLANID"
                    if [ $? -eq 1 ]; then
                        echo_t "$l2sd0Prefix.$Xfinity_Public_5_VLANID is not created at First Retry, try again after 2 sec"
                        sleep 2
                        sysevent set multinet_11-status stopped
                        $UTOPIA_PATH/service_multinet_exec multinet-start 11
                        ifconfig $l2sd0Prefix.$Xfinity_Public_5_VLANID up
                        ifconfig | grep "$l2sd0Prefix\.$Xfinity_Public_5_VLANID"
                        if [ $? -eq 1 ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Public_5_VLANID is not created after Second Retry, no more retries !!!"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : $l2sd0Prefix.$Xfinity_Public_5_VLANID created at First Retry itself"
                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] :Public XfinityWifi:  SSID 5GHz is enabled but gre tunnels not present, restoring it"
                    t2CountNotify "SYS_ERROR_GRETunnel_restored"
                    rcount=1
                fi
              fi
              fi

            # ----------------------------------------------------------------
            fi
            # ----------------------------------------------------------------

            if [ $rcount -eq 1 ] ; then
                sh $UTOPIA_PATH/service_multinet/handle_gre.sh hotspotfd-tunnelEP recover
            fi
        fi  # [ "$WAN_TYPE" != "EPON" ] && [ "$HOTSPOT_ENABLE" = "true" ]
    ;;
    "TCCBR")
          if [ "$HOTSPOT_ENABLE" = "true" ]; then                                                                                      
                XOPEN_24=$(get_ssid_enable 5)
                XOPEN_5=$(get_ssid_enable 6)
                XSEC_24=$(get_ssid_enable 9)
                XSEC_5=$(get_ssid_enable 10)
                XOPEN_16=$(get_ssid_enable 16)
                
                open2=`wlctl -i wl0.2 bss`
                open5=`wlctl -i wl1.2 bss`
                sec2=`wlctl -i wl0.4 bss`
                sec5=`wlctl -i wl1.4 bss`
                open16=`wlctl -i wl1.7 bss`
                xcount=0 
                if [ "$XOPEN_24" = "true" ]; then
                      if [ "$open2" = "down" ]; then
                           wlctl -i wl0.2 bss up
                           echo_t "[RDKB_PLATFORM_INFO] :XfinityWifi:  TCBR  SSID:5 2.4GHz restoring"
                           xcount=1
                      fi
                fi
                
                if [ "$XOPEN_5" = "true" ]; then
                      if [ "$open5" = "down" ]; then
                           wlctl -i wl1.2 bss up
                           echo_t "[RDKB_PLATFORM_INFO] :XfinityWifi:  TCBR SSID:6 5GHz restoring"
                           xcount=1
                      fi
                fi
                
                if [ "$XSEC_24" = "true" ]; then
                      if [ "$sec2" = "down" ]; then
                           wlctl -i wl0.4 bss up
                           echo_t "[RDKB_PLATFORM_INFO] :XfinityWifi:  TCBR SSID:9 2.4GHz restoring"
                           xcount=1
                      fi
                fi

                if [ "$XSEC_5" = "true" ]; then
                      if [ "$sec5" = "down" ]; then
                           wlctl -i wl1.4 bss up
                           echo_t "[RDKB_PLATFORM_INFO] :XfinityWifi:  TCBR SSID:10 5GHz restoring"
                           xcount=1
                      fi
                fi

                if [ "$XOPEN_16" = "true" ]; then
                      if [ "$open16" = "down" ]; then
                           wlctl -i wl1.7 bss up
                           echo_t "[RDKB_PLATFORM_INFO] :XfinityWifi:  TCBR SSID:16 5GHz restoring"
                           xcount=1
                      fi
                fi
                
                if [ $xcount -eq 1 ] ; then
                      echo_t "apply settings"
                      dmcli eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool true
                      dmcli eRT setv Device.WiFi.Radio.2.X_CISCO_COM_ApplySetting bool true
                fi
         fi                                                                                  
    ;;
    "SYSTEMD")
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        # TODO: move DROPBEAR BASE code with TCCBR,SYSTEMD code!
    ;;
    "TCCBR")
        #Check dropbear is alive to do rsync/scp to/fro ATOM
        if [ "$ARM_INTERFACE_IP" != "" ]; then
            DROPBEAR_ENABLE=$(grep "dropbear" $PROCESSES_CACHE | grep "$ARM_INTERFACE_IP")
            if [ "$DROPBEAR_ENABLE" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : rsync_dropbear_process is not running, need restart"
                t2CountNotify "SYS_SH_Dropbear_restart"
                dropbear -E -s -p $ARM_INTERFACE_IP:22 > /dev/null 2>&1
            fi
        fi
    ;;
    "SYSTEMD")
        if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then
            #Checking dropbear PID
            DROPBEAR_PID=$(get_pid dropbear)
            if [ "$DROPBEAR_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : dropbear_process is not running, restarting it"
                t2CountNotify "SYS_SH_Dropbear_restart"
                sh /etc/utopia/service.d/service_sshd.sh sshd-restart &
            fi
            if [ -f "/nvram/ETHWAN_ENABLE" ]; then
                NUMPROC=$(netstat -tulpn | grep ":22 " | grep -c "dropbear")
                if [ $NUMPROC -lt 2 ] ; then
                    echo_t "EWAN mode : Dropbear not listening on ipv6 address, restarting dropbear "
                    sh /etc/utopia/service.d/service_sshd.sh sshd-restart &
                fi
            fi
        fi
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        # TODO: move LIGHTTPD_PID BASE code with TCCBR,SYSTEMD code!
    ;;
    "TCCBR"|"SYSTEMD")
	#Ignoring for XLE and Ashlene model. lighttpd and webgui.sh are not avaialble in these boxes.
	if [ "$MODEL_NUM" = "WNXL11BWL" ] || [ "$MODEL_NUM" = "SE501" ]; then
            : # Do nothing
        else
            # Checking lighttpd PID
            LIGHTTPD_PID=$(get_pid lighttpd)
            WEBGUI_PID=$(get_pid webgui.sh)
            if [ "$LIGHTTPD_PID" = "" ]; then
                if [ "$WEBGUI_PID" != "" ]; then
                    if [ -f /tmp/WEBGUI_"$WEBGUI_PID" ]; then
                        echo_t "WEBGUI is in hung state, restarting it"
                        kill -9 "$WEBGUI_PID"
                        rm /tmp/WEBGUI_*

                        isPortKilled=$(netstat -anp | grep "21515")
                        if [ "$isPortKilled" != "" ]; then
                            echo_t "Port 21515 is still alive. Killing processes associated to 21515"
                            fuser -k 21515/tcp
                        fi
                        rm /tmp/webgui.lock
                        sh /etc/webgui.sh
                    else
                        for f in /tmp/WEBGUI_*
                          do
                            if [ -f "$f" ]; then  #TODO: file test not needed since we just got list of filenames from shell?
                                rm "$f"
                            fi
                          done
                        touch /tmp/WEBGUI_"$WEBGUI_PID"
                        echo_t "WEBGUI is running with pid $WEBGUI_PID"
                    fi
                else
                    isPortKilled=$(netstat -anp | grep "21515")
                    if [ "$isPortKilled" != "" ]; then
                        echo_t "Port 21515 is still alive. Killing processes associated to 21515"
                        fuser -k 21515/tcp
                    fi
                    if [ -f "/tmp/wifi_initialized" ]; then
                        echo_t "RDKB_PROCESS_CRASHED : lighttpd is not running, restarting it"
                        t2CountNotify "SYS_SH_lighttpdCrash"
                        #lighttpd -f $LIGHTTPD_CONF
                    else
                        #if wifi is not initialized, still starting lighttpd to have gui access. Not a crash.
                        echo_t "WiFi is not initialized yet. Starting lighttpd for GUI access."
                    fi
                    rm /tmp/webgui.lock
                    sh /etc/webgui.sh
                fi
            fi
        fi
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        _start_parodus_()
        {
            echo_t "RDKB_PROCESS_CRASHED : parodus process is not running, need restart"
            t2CountNotify "WIFI_SH_Parodus_restart"
            echo_t "Check parodusCmd.cmd in /tmp"
            if [ -e /tmp/parodusCmd.cmd ]; then
                echo_t "parodusCmd.cmd exists in tmp, but deleting it to recreate and fetch new values"
                rm -rf /tmp/parodusCmd.cmd
                #start parodus
                /usr/bin/parodusStart &
                echo_t "Started parodusStart in background"
            else
                echo_t "parodusCmd.cmd does not exist in tmp, trying to start parodus"
                /usr/bin/parodusStart &
            fi
        }
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

if [ "$MODEL_NUM" = "" ] || [ "$FIRMWARE_TYPE" = "OFW" ]; then
	echo_t "[RDKB_SELFHEAL] : Disabling parodus"
else
# Checking for parodus connection stuck issue
# Checking parodus PID
PARODUS_PID=$(get_pid parodus)
case $SELFHEAL_TYPE in
    "BASE")
        PARODUSSTART_PID=$(get_pid parodusStart)
        if [ "$PARODUS_PID" = "" ] && [ "$PARODUSSTART_PID" = "" ]; then
            _start_parodus_
            thisPARODUS_PID=""    # avoid executing 'already-running' code below
        fi
    ;;
    "TCCBR"|"SYSTEMD")
        thisPARODUS_PID="$PARODUS_PID"
    ;;
esac
if [ "$thisPARODUS_PID" != "" ]; then
    # parodus process is running,
    kill_parodus_msg=""
    # check if parodus is stuck in connecting
    if [ "$kill_parodus_msg" = "" ] && [ -f $PARCONNHEALTH_PATH ]; then
        wan_status=$(sysevent get wan-status)
        if [ "$wan_status" = "started" ]; then
            time_line=$(awk '/^\{START=[0-9\,]+\}$/' $PARCONNHEALTH_PATH)
        else
            time_line=""
        fi
        health_time_pair=$(echo "$time_line" | tr -d "}" | cut -d"=" -f2)
        if [ "$health_time_pair" != "" ]; then
            conn_start_time=$(echo "$health_time_pair" | cut -d"," -f1)
            conn_retry_time=$(echo "$health_time_pair" | cut -d"," -f2)
            echo_t "Parodus connecting time '$health_time_pair'"
            time_now=$(date +%s)
            time_now_val=$((time_now+0))
            time_limit=$((conn_retry_time+1800))
            if [ $time_now_val -ge $time_limit ]; then
                # parodus connection health file has a recorded
                # time stamp that is > 30 minutes old, seems parodus conn is stuck
                kill_parodus_msg="Parodus Connection TimeStamp Expired."
                t2CountNotify "SYS_ERROR_parodus_TimeStampExpired"
            fi
            time_limit=$((conn_start_time+900))
            if [ $time_now_val -ge $time_limit ]; then
                echo_t "Parodus connection lost since 15 minutes"
            fi
        fi
    fi
    if [ "$kill_parodus_msg" != "" ]; then
        case $SELFHEAL_TYPE in
            "BASE")
                ppid=$(get_pid parodus)
                if [ "$ppid" != "" ]; then
                    echo_t "$kill_parodus_msg Killing parodus process."
                    t2CountNotify "SYS_SH_Parodus_Killed"
                    # want to generate minidump for further analysis hence using signal 11
                    kill -11 "$ppid"
                    sleep 1
                fi
                _start_parodus_
            ;;
            "TCCBR"|"SYSTEMD")
                ppid=$(get_pid parodus)
                if [ "$ppid" != "" ]; then
                    echo "[$(getDateTime)] $kill_parodus_msg Killing parodus process."
                    t2CountNotify "SYS_SH_Parodus_Killed"
                    # want to generate minidump for further analysis hence using signal 11
                    systemctl kill --signal=11 parodus.service
                fi
            ;;
        esac
    fi
fi
fi

#Implement selfheal mechanism for aker to restart aker process in selfheal window  in XB3 devices
case $SELFHEAL_TYPE in
    "BASE")
	# Checking Aker PID
	AKER_PID=$(get_pid aker)
	if [ -f "/etc/AKER_ENABLE" ] &&  [ "$AKER_PID" = "" ]; then
		echo_t "[RDKB_PROCESS_CRASHED] : aker process is not running need restart"
		t2CountNotify "SYS_SH_akerCrash"
		if [ ! -f  "/tmp/aker_cmd.cmd" ] ; then
			echo_t "aker_cmd.cmd don't exist in tmp, creating it."
			echo "/usr/bin/aker -p $PARODUS_URL -c $AKER_URL -w parcon -d /nvram/pcs.bin -f /nvram/pcs.bin.md5" > /tmp/aker_cmd.cmd
		fi
		aker_cmd=`cat /tmp/aker_cmd.cmd`
		$aker_cmd &
	fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        # TODO: move DROPBEAR BASE code with TCCBR,SYSTEMD code!
        #Check dropbear is alive to do rsync/scp to/fro ATOM
        if [ "$ARM_INTERFACE_IP" != "" ]; then
            DROPBEAR_ENABLE=$(grep "dropbear" $PROCESSES_CACHE | grep "$ARM_INTERFACE_IP")
            if [ "$DROPBEAR_ENABLE" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : rsync_dropbear_process is not running, need restart"
                t2CountNotify "SYS_SH_Dropbear_restart"	
                DROPBEAR_PARAMS_1="/tmp/.dropbear/dropcfg1_tdt2"
                DROPBEAR_PARAMS_2="/tmp/.dropbear/dropcfg2_tdt2"
                if [ ! -d '/tmp/.dropbear' ]; then
                    mkdir -p /tmp/.dropbear
                fi
                if [ ! -f $DROPBEAR_PARAMS_1 ]; then
                    getConfigFile $DROPBEAR_PARAMS_1
                fi
                if [ ! -f $DROPBEAR_PARAMS_2 ]; then
                    getConfigFile $DROPBEAR_PARAMS_2
                fi
                dropbear -r $DROPBEAR_PARAMS_1 -r $DROPBEAR_PARAMS_2 -E -s -p $ARM_INTERFACE_IP:22 -P /var/run/dropbear_ipc.pid > /dev/null 2>&1
            fi
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        # TODO: move LIGHTTPD_PID BASE code with TCCBR,SYSTEMD code!
        # Checking lighttpd PID
        LIGHTTPD_PID=$(get_pid lighttpd)
        WEBGUI_PID=$(get_pid webgui.sh)
        if [ "$LIGHTTPD_PID" = "" ]; then
            if [ "$WEBGUI_PID" != "" ]; then
                if [ -f /tmp/WEBGUI_"$WEBGUI_PID" ]; then
                    echo_t "WEBGUI is in hung state, restarting it"
                    kill -9 "$WEBGUI_PID"
                    rm /tmp/WEBGUI_*

                    isPortKilled=$(netstat -anp | grep "21515")
                    if [ "$isPortKilled" != "" ]; then
                        echo_t "Port 21515 is still alive. Killing processes associated to 21515"
                        fuser -k 21515/tcp
                    fi
                    rm /tmp/webgui.lock
                    sh /etc/webgui.sh
                else
                    for f in /tmp/WEBGUI_*
                      do
                        if [ -f "$f" ]; then  #TODO: file test not needed since we just got list of filenames from shell?
                            rm "$f"
                        fi
                      done
                    touch /tmp/WEBGUI_"$WEBGUI_PID"
                    echo_t "WEBGUI is running with pid $WEBGUI_PID"
                fi
            else
                isPortKilled=$(netstat -anp | grep "21515")
                if [ "$isPortKilled" != "" ]; then
                    echo_t "Port 21515 is still alive. Killing processes associated to 21515"
                    fuser -k 21515/tcp
                fi

                echo_t "RDKB_PROCESS_CRASHED : lighttpd is not running, restarting it"
                t2CountNotify "SYS_SH_lighttpdCrash"
                #lighttpd -f $LIGHTTPD_CONF
                rm /tmp/webgui.lock
                sh /etc/webgui.sh
            fi
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

if [ "$MODEL_NUM" = "PX5001" ] || [ "$MODEL_NUM" = "PX5001B" ] || [ "$MODEL_NUM" = "CGA4131COM" ]; then
    #Checking if acsd is running and whether acsd core is generated or not
    #Check the status if 2.4GHz Wifi Radio
    RADIO_DISBLD_2G=-1
    Radio_1=$(fast_dmcli eRT getv Device.WiFi.Radio.1.Enable)
    RadioExecution_1=$(echo "$Radio_1" | grep "Execution succeed")
    if [ "$RadioExecution_1" != "" ]; then
        isDisabled_1=$(echo "$Radio_1" | grep "false")
        if [ "$isDisabled_1" != "" ]; then
            RADIO_DISBLD_2G=1
        fi
    fi
    RADIO_DISBLD_5G=-1
    Radio_2=$(fast_dmcli eRT getv Device.WiFi.Radio.2.Enable)
    RadioExecution_2=$(echo "$Radio_2" | grep "Execution succeed")
    if [ "$RadioExecution_2" != "" ]; then
        isDisabled_2=$(echo "$Radio_2" | grep "false")
        if [ "$isDisabled_2" != "" ]; then
            RADIO_DISBLD_5G=1
        fi
    fi
    RADIO_STATUS_2G=-1
    Radio_sts_1=$(dmcli eRT getv Device.WiFi.Radio.1.Status)
    RadioExecution_sts_1=$(echo "$Radio_sts_1" | grep "Execution succeed")
    if [ "$RadioExecution_sts_1" != "" ]; then
        isDisabled_3=$(echo "$Radio_sts_1" | grep "Down")
        if [ "$isDisabled_3" != "" ]; then
            RADIO_STATUS_2G=1
        fi
    fi
    RADIO_STATUS_5G=-1
    Radio_sts_2=$(dmcli eRT getv Device.WiFi.Radio.2.Status)
    RadioExecution_sts_2=$(echo "$Radio_sts_2" | grep "Execution succeed")
    if [ "$RadioExecution_sts_2" != "" ]; then
        isDisabled_4=$(echo "$Radio_sts_2" | grep "Down")
        if [ "$isDisabled_4" != "" ]; then
            RADIO_STATUS_5G=1
        fi
    fi
    if [ $RADIO_DISBLD_2G -eq 1 ] && [ $RADIO_DISBLD_5G -eq 1 ]; then
        echo_t "[RDKB_SELFHEAL] : Radio's disabled, Skipping ACSD check"
    elif [ $RADIO_STATUS_2G -eq 1 ] && [ $RADIO_STATUS_5G -eq 1 ]; then
        echo_t "[RDKB_SELFHEAL] : Radio's status is down, Skipping ACSD check"
    else
        ACSD_PID=$(get_pid acsd)
        if [ "$ACSD_PID" = ""  ]; then
            echo_t "[ACSD_CRASH/RESTART] : ACSD is not running "
        fi
        ACSD_CORE=$(ls /tmp | grep "core.prog_acsd")
        if [ "$ACSD_CORE" != "" ]; then
            echo_t "[ACSD_CRASH/RESTART] : ACSD core has been generated inside /tmp :  $ACSD_CORE"
            ACSD_CORE_COUNT=$(ls /tmp | grep -c "core.prog_acsd")
            echo_t "[ACSD_CRASH/RESTART] : Number of ACSD cores created inside /tmp  are : $ACSD_CORE_COUNT"
        fi
    fi
fi

#Checking Wheteher any core is generated inside /tmp folder
CORE_TMP=$(ls /tmp | grep "core.")
if [ "$CORE_TMP" != "" ]; then
    echo_t "[PROCESS_CRASH] : core has been generated inside /tmp :  $CORE_TMP"
    if [ "$CORE_TMP" = "snmp_subagent.core.gz" ]; then
        t2CountNotify "SYS_ERROR_snmpSubagentcrash"
    fi
    CORE_COUNT=$(ls /tmp | grep -c "core.")
    echo_t "[PROCESS_CRASH] : Number of cores created inside /tmp are : $CORE_COUNT"
fi


# Checking syseventd PID
SYSEVENT_PID=$(get_pid syseventd)
if [ "$SYSEVENT_PID" = "" ]; then
    #Needs to avoid false alarm
    rebootCounter=$(get_from_syscfg_cache X_RDKCENTRAL-COM_LastRebootCounter)
    echo_t "[syseventd] Previous rebootCounter:$rebootCounter"

    if [ ! -f "$SyseventdCrashed"  ] && [ "$rebootCounter" != "1" ] ; then
        echo_t "[RDKB_PROCESS_CRASHED] : syseventd is crashed, need to reboot the device in maintanance window."
        t2CountNotify "SYS_ERROR_syseventdCrashed"
        touch $SyseventdCrashed
        case $SELFHEAL_TYPE in
            "BASE"|"SYSTEMD")
                echo_t "Setting Last reboot reason"
                dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string Syseventd_crash
                dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1
            ;;
            "TCCBR")
            ;;
        esac
    fi
    rebootDeviceNeeded=1
fi


case $SELFHEAL_TYPE in
    "BASE")
        # Checking snmp master PID
        if [ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "MV1" ]; then
            SNMP_MASTER_PID=$(get_pid snmp_agent_cm)
            if [ "$SNMP_MASTER_PID" = "" ] && [  ! -f "$SNMPMASTERCRASHED"  ]; then
                echo_t "[RDKB_PROCESS_CRASHED] : snmp_agent_cm process crashed"
                touch $SNMPMASTERCRASHED
            fi
        fi

        if [ -e /tmp/atom_ro ]; then
            reboot_needed_atom_ro=1
            rebootDeviceNeeded=1
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")

        #Checking Wheteher any core is generated inside /tmp folder
        CORE_TMP=$(ls /tmp | grep "core.")
        if [ "$CORE_TMP" != "" ]; then
            echo_t "[PROCESS_CRASH] : core has been generated inside /tmp :  $CORE_TMP"
            if [ "$CORE_TMP" = "snmp_subagent.core.gz" ]; then
                t2CountNotify "SYS_ERROR_snmpSubagentcrash"
            fi
            CORE_COUNT=$(ls /tmp | grep -c "core.")
            echo_t "[PROCESS_CRASH] : Number of cores created inside /tmp are : $CORE_COUNT"
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

# ARRIS XB6 => MODEL_NUM=TG3482G
# Tech CBR  => MODEL_NUM=CGA4131COM
# Tech xb6  => MODEL_NUM=CGM4140COM
# Tech XB7  => MODEL_NUM=CGM4331COM
# CMX  XB7  => MODEL_NUM=TG4482A
# Tech CBR2 => MODEL_NUM=CGA4332COM
# Tech XB8  => MODEL_NUM=CGM4981COM
# This critical processes checking is handled in selfheal_aggressive.sh for above platforms
# Ref: RDKB-25546
if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
       [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] && [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ] && [ "$MODEL_NUM" != "brcm93390" ]
then
case $SELFHEAL_TYPE in
    "BASE")
        # Checking whether brlan0 and l2sd0.100 are created properly , if not recreate it

        if [ "$WAN_TYPE" != "EPON" ] && [ "$BOX_TYPE" != "MV3" ]; then
            if [ ! -f /tmp/.router_reboot ]; then
                if [ "$bridgeMode" != "" ]; then
                    check_device_in_router_mode=$(echo "$bridgeMode" | grep "router")
                    if [ "$check_device_in_router_mode" != "" ]; then
                        check_if_brlan0_created=$(ifconfig | grep "brlan0")
                        check_if_brlan0_up=$(ifconfig brlan0 | grep "UP")
                        check_if_brlan0_hasip=$(ifconfig brlan0 | grep "inet addr")
                        check_if_l2sd0_100_created=$(ifconfig | grep "l2sd0.100")
                        check_if_l2sd0_100_up=$(ifconfig l2sd0.100 | grep "UP" )
                        if [ "$check_if_brlan0_created" = "" ] || [ "$check_if_brlan0_up" = "" ] || [ "$check_if_brlan0_hasip" = "" ] || [ "$check_if_l2sd0_100_created" = "" ] || [ "$check_if_l2sd0_100_up" = "" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : Either brlan0 or l2sd0.100 is not completely up, setting event to recreate vlan and brlan0 interface"
                            echo_t "[RDKB_SELFHEAL_BOOTUP] : brlan0 and l2sd0.100 o/p "
                            ifconfig brlan0;ifconfig l2sd0.100;							
                            if [ "x$ovs_enable" = "xtrue" ];then
                            	ovs-vsctl show
                            else
                            	brctl show
                            fi
                            logNetworkInfo

                            ipv4_status=$(sysevent get ipv4_4-status)
                            lan_status=$(sysevent get lan-status)

                            if [ "$lan_status" != "started" ]; then
                                if [ "$ipv4_status" = "" ] || [ "$ipv4_status" = "down" ]; then
                                    echo_t "[RDKB_SELFHEAL] : ipv4_4-status is not set or lan is not started, setting lan-start event"
                                    sysevent set lan-start
                                    sleep 60
				else
				    if [ "$check_if_brlan0_created" = "" ] && [ "$check_if_l2sd0_100_created" = "" ]; then
					/etc/utopia/registration.d/02_multinet restart
				    fi

				    sysevent set multinet-down 1
				    sleep 5
				    sysevent set multinet-up 1
				    sleep 30
                                fi
                            else
				if [ "$check_if_brlan0_created" = "" ] && [ "$check_if_l2sd0_100_created" = "" ]; then
				    /etc/utopia/registration.d/02_multinet restart
				fi

				sysevent set multinet-down 1
				sleep 5
				sysevent set multinet-up 1
				sleep 30
			    fi
                        fi

                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while fetching device mode "
                    t2CountNotify "SYS_ERROR_Error_fetching_devicemode"
                fi
            else
                rm -rf /tmp/.router_reboot
            fi

            #TODO: Need to revisit this after enabling CcspHomeSecurity
            # Checking whether brlan1 and l2sd0.101 interface are created properly
            if [ "$thisIS_BCI" != "yes" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
                check_if_brlan1_created=$(ifconfig | grep "brlan1")
                check_if_brlan1_up=$(ifconfig brlan1 | grep "UP")
                check_if_brlan1_hasip=$(ifconfig brlan1 | grep "inet addr")
                check_if_l2sd0_101_created=$(ifconfig | grep "l2sd0.101")
                check_if_l2sd0_101_up=$(ifconfig l2sd0.101 | grep "UP" )

                if [ "$check_if_brlan1_created" = "" ] || [ "$check_if_brlan1_up" = "" ] || [ "$check_if_brlan1_hasip" = "" ] || [ "$check_if_l2sd0_101_created" = "" ] || [ "$check_if_l2sd0_101_up" = "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : Either brlan1 or l2sd0.101 is not completely up, setting event to recreate vlan and brlan1 interface"
                    echo_t "[RDKB_SELFHEAL_BOOTUP] : brlan1 and l2sd0.101 o/p "
                    ifconfig brlan1;ifconfig l2sd0.101; 
                    if [ "x$ovs_enable" = "xtrue" ];then
                    	ovs-vsctl show
                    else
                    	brctl show
                    fi
					
                    ipv5_status=$(sysevent get ipv4_5-status)
                    lan_l3net=$(sysevent get homesecurity_lan_l3net)

                    if [ "$lan_l3net" != "" ]; then
                        if [ "$ipv5_status" = "" ] || [ "$ipv5_status" = "down" ]; then
                            echo_t "[RDKB_SELFHEAL] : ipv5_4-status is not set , setting event to create homesecurity lan"
                            sysevent set ipv4-up $lan_l3net
                            sleep 60
			else
			    if [ "$check_if_brlan1_created" = "" ] && [ "$check_if_l2sd0_101_created" = "" ] ; then
				/etc/utopia/registration.d/02_multinet restart
			    fi
			    sysevent set multinet-down 2
			    sleep 5
			    sysevent set multinet-up 2
			    sleep 10
                        fi
                    else
			if [ "$check_if_brlan1_created" = "" ] && [ "$check_if_l2sd0_101_created" = "" ] ; then
			    /etc/utopia/registration.d/02_multinet restart
			fi

			sysevent set multinet-down 2
			sleep 5
			sysevent set multinet-up 2
			sleep 10
		    fi
                fi
            fi
        fi
    ;;
    "TCCBR")
        # Checking whether brlan0 created properly , if not recreate it
        lanSelfheal=$(sysevent get lan_selfheal)
        echo_t "[RDKB_SELFHEAL] : Value of lanSelfheal : $lanSelfheal"
        if [ ! -f /tmp/.router_reboot ]; then
            if [ "$lanSelfheal" != "done" ]; then
                check_param_get_succeed=$(echo "$bridgeMode" | grep "Execution succeed")
                if [ "$check_param_get_succeed" != "" ]; then
                    check_device_in_router_mode=$(echo "$bridgeMode" | grep "router")
                    if [ "$check_device_in_router_mode" != "" ]; then
                        check_if_brlan0_created=$(ifconfig | grep "brlan0")
                        check_if_brlan0_up=$(ifconfig brlan0 | grep "UP")
                        check_if_brlan0_hasip=$(ifconfig brlan0 | grep "inet addr")
                        if [ "$check_if_brlan0_created" = "" ] || [ "$check_if_brlan0_up" = "" ] || [ "$check_if_brlan0_hasip" = "" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : brlan0 is not completely up, setting event to recreate brlan0 interface"
                            t2CountNotify "SYS_ERROR_brlan0_not_created"
                            logNetworkInfo

                            ipv4_status=$(sysevent get ipv4_4-status)
                            lan_status=$(sysevent get lan-status)

                            if [ "$lan_status" != "started" ]; then
                                if [ "$ipv4_status" = "" ] || [ "$ipv4_status" = "down" ]; then
                                    echo_t "[RDKB_SELFHEAL] : ipv4_4-status is not set or lan is not started, setting lan-start event"
                                    sysevent set lan-start
                                    sleep 30
				else
				    if [ "$check_if_brlan0_created" = "" ]; then
					/etc/utopia/registration.d/02_multinet restart
				    fi

				    sysevent set multinet-down 1
				    sleep 5
				    sysevent set multinet-up 1
				    sleep 30
                                fi
                            else

				if [ "$check_if_brlan0_created" = "" ]; then
				    /etc/utopia/registration.d/02_multinet restart
				fi

				sysevent set multinet-down 1
				sleep 5
				sysevent set multinet-up 1
				sleep 30
			    fi
                            sysevent set lan_selfheal "done"
                        fi

                    fi
                else
                    echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while fetching device mode "
                    t2CountNotify "SYS_ERROR_Error_fetching_devicemode"
                fi
            else
                echo_t "[RDKB_SELFHEAL] : brlan0 already restarted. Not restarting again"
                t2CountNotify "SYS_SH_brlan0_restarted"
                sysevent set lan_selfheal ""
            fi
        else
            rm -rf /tmp/.router_reboot
        fi
    ;;
    "SYSTEMD")
        if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then
            # Checking whether brlan0 is created properly , if not recreate it
            lanSelfheal=$(sysevent get lan_selfheal)
            echo_t "[RDKB_SELFHEAL] : Value of lanSelfheal : $lanSelfheal"
            if [ ! -f /tmp/.router_reboot ]; then
                if [ "$lanSelfheal" != "done" ]; then
                    # Check device is in router mode
                    # Get from syscfg instead of dmcli for performance reasons
                    check_device_in_bridge_mode=$(get_from_syscfg_cache bridge_mode)
                    if [ "$check_device_in_bridge_mode" = "0" ]; then
                        check_if_brlan0_created=$(ifconfig | grep "brlan0")
                        check_if_brlan0_up=$(ifconfig brlan0 | grep "UP")
                        check_if_brlan0_hasip=$(ifconfig brlan0 | grep "inet addr")
                        if [ "$check_if_brlan0_created" = "" ] || [ "$check_if_brlan0_up" = "" ] || [ "$check_if_brlan0_hasip" = "" ]; then
                            echo_t "[RDKB_PLATFORM_ERROR] : brlan0 is not completely up, setting event to recreate vlan and brlan0 interface"
                            t2CountNotify "SYS_ERROR_brlan0_not_created"
                            logNetworkInfo

                            ipv4_status=$(sysevent get ipv4_4-status)
                            lan_status=$(sysevent get lan-status)

                            if [ "$lan_status" != "started" ]; then
                                if [ "$ipv4_status" = "" ] || [ "$ipv4_status" = "down" ]; then
                                    echo_t "[RDKB_SELFHEAL] : ipv4_4-status is not set or lan is not started, setting lan-start event"
                                    sysevent set lan-start
                                    sleep 30
				else
				    if [ "$check_if_brlan0_created" = "" ]; then
					/etc/utopia/registration.d/02_multinet restart
				    fi

				    sysevent set multinet-down 1
				    sleep 5
				    sysevent set multinet-up 1
				    sleep 30
                                fi
                            else

				if [ "$check_if_brlan0_created" = "" ]; then
				    /etc/utopia/registration.d/02_multinet restart
				fi

				sysevent set multinet-down 1
				sleep 5
				sysevent set multinet-up 1
				sleep 30
			    fi
                            sysevent set lan_selfheal "done"
                        fi

                    fi
                else
                    echo_t "[RDKB_SELFHEAL] : brlan0 already restarted. Not restarting again"
                    t2CountNotify "SYS_SH_brlan0_restarted"
                    sysevent set lan_selfheal ""
                fi
            else
                rm -rf /tmp/.router_reboot
            fi
            # Checking whether brlan1 interface is created properly

            l3netRestart=$(sysevent get l3net_selfheal)
            echo_t "[RDKB_SELFHEAL] : Value of l3net_selfheal : $l3netRestart"

            if [ "$l3netRestart" != "done" ]; then

                check_if_brlan1_created=$(ifconfig | grep "brlan1")
                check_if_brlan1_up=$(ifconfig brlan1 | grep "UP")
                check_if_brlan1_hasip=$(ifconfig brlan1 | grep "inet addr")

                if [ "$check_if_brlan1_created" = "" ] || [ "$check_if_brlan1_up" = "" ] || [ "$check_if_brlan1_hasip" = "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : brlan1 is not completely up, setting event to recreate vlan and brlan1 interface"

                    ipv5_status=$(sysevent get ipv4_5-status)
                    lan_l3net=$(sysevent get homesecurity_lan_l3net)

                    if [ "$lan_l3net" != "" ]; then
                        if [ "$ipv5_status" = "" ] || [ "$ipv5_status" = "down" ]; then
                            echo_t "[RDKB_SELFHEAL] : ipv5_4-status is not set , setting event to create homesecurity lan"
                            sysevent set ipv4-up $lan_l3net
                            sleep 30
			else
			    if [ "$check_if_brlan1_created" = "" ]; then
				/etc/utopia/registration.d/02_multinet restart
			    fi

			    sysevent set multinet-down 2
			    sleep 5
			    sysevent set multinet-up 2
			    sleep 10
                        fi
                    else

			if [ "$check_if_brlan1_created" = "" ]; then
			    /etc/utopia/registration.d/02_multinet restart
			fi

			sysevent set multinet-down 2
			sleep 5
			sysevent set multinet-up 2
			sleep 10
		    fi
                    sysevent set l3net_selfheal "done"
                fi
            else
                echo_t "[RDKB_SELFHEAL] : brlan1 already restarted. Not restarting again"
            fi

            # Test to make sure that if mesh is enabled the backhaul tunnels are attached to the bridges
            MESH_ENABLE=$(dmcli eRT retv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Enable)
            if [ "$MESH_ENABLE" = "true" ]; then
                echo_t "[RDKB_SELFHEAL] : Mesh is enabled, test if tunnels are attached to bridges"
                t2CountNotify "WIFI_INFO_mesh_enabled"

                # Fetch mesh tunnels from the brlan0 bridge if they exist
                if [ "x$ovs_enable" = "xtrue" ];then
                    brctl0_ifaces=$(ovs-vsctl list-ifaces brlan0 | egrep "pgd")
                else
                    brctl0_ifaces=$(brctl show brlan0 | egrep "pgd")
                fi
                br0_ifaces=$(ifconfig | egrep "^pgd" | egrep "\.100" | awk '{print $1}')

                for ifn in $br0_ifaces
                  do
                    brFound="false"

                    for br in $brctl0_ifaces
                      do
                        if [ "$br" = "$ifn" ]; then
                            brFound="true"
                        fi
                      done
                    if [ "$brFound" = "false" ]; then
                        echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan0"
                            if [ "x$bridgeUtilEnable" = "xtrue" || "x$ovs_enable" = "xtrue" ];then
                                echo_t "RDKB_SELFHEAL : Ovs is enabled, calling bridgeUtils to  add $ifn to brlan0  :"
                                /usr/bin/bridgeUtils add-port brlan0 $ifn;
                            else
                                brctl addif brlan0 $ifn;
                            fi 

                    fi
                  done

                # Fetch mesh tunnels from the brlan1 bridge if they exist
                if [ "$thisIS_BCI" != "yes" ]; then
                    if [ "x$ovs_enable" = "xtrue" ];then
                    	brctl1_ifaces=$(ovs-vsctl list-ifaces brlan1 | egrep "pgd")
                    else
                    	brctl1_ifaces=$(brctl show brlan1 | egrep "pgd")
                    fi
                    br1_ifaces=$(ifconfig | egrep "^pgd" | egrep "\.101" | awk '{print $1}')

                    for ifn in $br1_ifaces
                      do
                        brFound="false"

                        for br in $brctl1_ifaces
                          do
                            if [ "$br" = "$ifn" ]; then
                                brFound="true"
                            fi
                          done
                        if [ "$brFound" = "false" ]; then
                            echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan1"
                            if [ "x$bridgeUtilEnable" = "xtrue" || "x$ovs_enable" = "xtrue" ];then
                                echo_t "RDKB_SELFHEAL : Ovs is enabled, calling bridgeUtils to  add $ifn to brlan1  :"
                                /usr/bin/bridgeUtils add-port brlan1 $ifn;
                            else
                                brctl addif brlan1 $ifn;
                            fi 
                        fi
                      done
                fi
            fi
        fi #Not HUB4 && SR300 && SE501 && SR213 && WNXL11BWL
    ;;
esac
fi

# RDKB-41671 - Check Radio Enable/disable and status(up/Down) too while checking SSID Status
Radio_5G_Enable_Check()
{
    radioenable_5=$(dmcli eRT getv Device.WiFi.Radio.2.Enable)
    isRadioExecutionSucceed_5=$(echo "$radioenable_5" | grep "Execution succeed")
    if [ "$isRadioExecutionSucceed_5" != "" ]; then
        isRadioEnabled_5=$(echo "$radioenable_5" | grep "false")
        if [ "$isRadioEnabled_5" != "" ]; then
            echo_t "[RDKB_SELFHEAL] : Both 5G Radio(Radio 2) and 5G Private SSID are in DISABLED state"
        else
            echo_t "[RDKB_SELFHEAL] : 5G Radio(Radio 2) is Enabled, only 5G Private SSID is DISABLED"
            fi
    else
        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Radio status."
        echo "$radioenable_5"
    fi
}



#!!! TODO: merge this $SELFHEAL_TYPE block !!!
case $SELFHEAL_TYPE in
    "BASE")
        SSID_DISABLED=0
        isEnabled=$(get_ssid_enable 2)
        if is_ssid_execution_succeed; then
            if [ "$isEnabled" != "true" ]; then
                Radio_5G_Enable_Check
                SSID_DISABLED=1
                echo_t "[RDKB_SELFHEAL] : SSID 5GHZ is disabled"
                t2CountNotify "WIFI_INFO_5G_DISABLED"
            fi
        else
            destinationError=$(get_from_ssid_cache "Can't find destination component")
            if [ "$destinationError" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : Parameter cannot be found on WiFi subsystem"
                t2CountNotify "WIFI_ERROR_WifiDmCliError"
            else
                echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Enable"
                sed -n 1,4p "$SSIDS_CACHE"
            fi
        fi
    ;;
    "TCCBR")
        #Selfheal will run after 15mins of bootup, then by now the WIFI initialization must have
        #completed, so if still wifi_initilization not done, we have to recover the WIFI
        #Restart the WIFI if initialization is not done with in 15mins of poweron.
        if [ "$WiFi_Flag" = "false" ]; then
            SSID_DISABLED=0
            if [ -f "/tmp/wifi_initialized" ]; then
                echo_t "[RDKB_SELFHEAL] : WiFi Initialization done"
                ssidEnable=$(fast_dmcli eRT getv Device.WiFi.SSID.2.Enable)
                ssidExecution=$(echo "$ssidEnable" | grep "Execution succeed")
                if [ "$ssidExecution" != "" ]; then
                    isEnabled=$(echo "$ssidEnable" | grep "false")
                    if [ "$isEnabled" != "" ]; then
                        Radio_5G_Enable_Check
                        SSID_DISABLED=1
                        echo_t "[RDKB_SELFHEAL] : SSID 5GHZ is disabled"
                        t2CountNotify "WIFI_INFO_5G_DISABLED"
                    fi
                else
                    destinationError=$(echo "$ssidEnable" | grep "Can't find destination component")
                    if [ "$destinationError" != "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Parameter cannot be found on WiFi subsystem"
                        t2CountNotify "WIFI_ERROR_WifiDmCliError"
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Enable"
                        echo "$ssidEnable"
                    fi
                fi
            else
                echo_t  "[RDKB_PLATFORM_ERROR] : WiFi initialization not done"
                if [ -f "$thisREADYFILE" ]; then
                    echo_t  "[RDKB_PLATFORM_ERROR] : restarting the CcspWifiSsp"
                    killall CcspWifiSsp
                    resetNeeded wifi CcspWifiSsp
                fi
            fi
        fi
    ;;
    "SYSTEMD")
        #Selfheal will run after 15mins of bootup, then by now the WIFI initialization must have
        #completed, so if still wifi_initilization not done, we have to recover the WIFI
        #Restart the WIFI if initialization is not done with in 15mins of poweron.
        if [ "$WiFi_Flag" = "false" ]; then
            SSID_DISABLED=0
            if [ -f "/tmp/wifi_initialized" ]; then
                echo_t "[RDKB_SELFHEAL] : WiFi Initialization done"
                ssidEnable=$(fast_dmcli eRT getv Device.WiFi.SSID.2.Enable)
                ssidExecution=$(echo "$ssidEnable" | grep "Execution succeed")
                if [ "$ssidExecution" != "" ]; then
                    isEnabled=$(echo "$ssidEnable" | grep "false")
                    if [ "$isEnabled" != "" ]; then
                        Radio_5G_Enable_Check
                        SSID_DISABLED=1
                        echo_t "[RDKB_SELFHEAL] : SSID 5GHZ is disabled"
			t2CountNotify "WIFI_INFO_5G_DISABLED"
                    fi
                else
                    destinationError=$(echo "$ssidEnable" | grep "Can't find destination component")
                    if [ "$destinationError" != "" ]; then
                        echo_t "[RDKB_PLATFORM_ERROR] : Parameter cannot be found on WiFi subsystem"
                        t2CountNotify "WIFI_ERROR_WifiDmCliError"
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Enable"
                        echo "$ssidEnable"
                    fi
                fi
            else
                echo_t  "[RDKB_PLATFORM_ERROR] : WiFi initialization not done"
                if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Technicolor" ]; then
                    if [ -f "$thisREADYFILE" ]; then
                        echo_t  "[RDKB_PLATFORM_ERROR] : restarting the CcspWifiSsp"
                        systemctl stop ccspwifiagent
                        systemctl start ccspwifiagent
                    fi
                fi
            fi
        fi
    ;;
esac

case $SELFHEAL_TYPE in
    "BASE")
        #check for PandM response
        bridgeSucceed=$(echo "$bridgeMode" | grep "Execution succeed")
        if [ "$bridgeSucceed" = "" ]; then
            echo_t "[RDKB_SELFHEAL_DEBUG] : bridge mode = $bridgeMode"
            if [ serialNumber == "" ]; then
                serialNumber=$(dmcli eRT retv Device.DeviceInfo.SerialNumber)
            fi
            echo_t "[RDKB_SELFHEAL_DEBUG] : SerialNumber = $serialNumber"
            if [ modelName == "" ]; then
                modelName=$(dmcli eRT retv Device.DeviceInfo.ModelName)
            fi
            echo_t "[RDKB_SELFHEAL_DEBUG] : modelName = $modelName"

            pandm_timeout=$(echo "$bridgeMode" | grep "CCSP_ERR_TIMEOUT")
            pandm_notexist=$(echo "$bridgeMode" | grep "CCSP_ERR_NOT_EXIST")
            pandm_notconnect=$(echo "$bridgeMode" | grep "CCSP_ERR_NOT_CONNECT")
            if [ "$pandm_timeout" != "" ] || [ "$pandm_notexist" != "" ] || [ "$pandm_notconnect" != "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR] : pandm parameter timed out or failed to return"

                pam_name=$(fast_dmcli eRT getv com.cisco.spvtg.ccsp.pam.Name)
                cr_timeout=$(echo "$pam_name" | grep "CCSP_ERR_TIMEOUT")
                cr_pam_notexist=$(echo "$pam_name" | grep "CCSP_ERR_NOT_EXIST")
                cr_pam_notconnect=$(echo "$pam_name" | grep "CCSP_ERR_NOT_CONNECT")
                if [ "$cr_timeout" != "" ] || [ "$cr_pam_notexist" != "" ] || [ "$cr_pam_notconnect" != "" ]; then
                    echo_t "[RDKB_PLATFORM_ERROR] : pandm process is not responding. Restarting it"
                    t2CountNotify "SYS_ERROR_PnM_Not_Responding"
                    PANDM_PID=$(busybox pidof CcspPandMSsp)
                    if [ "$PANDM_PID" != "" ]; then
                        kill -9 "$PANDM_PID"
                    fi
                    rm -rf /tmp/pam_initialized
                    resetNeeded pam CcspPandMSsp
                fi
            fi
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
    ;;
esac

if [ $RADIO_COUNT -eq 0 ]; then
    RADIO_COUNT=$(dmcli eRT retv Device.WiFi.RadioNumberOfEntries)
    if [ "$RADIO_COUNT" = "" ]; then
        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking number of radios"
        echo "$isnumofRadiosExec"
    fi
fi

if [ $RADIO_COUNT -eq 3 ]; then
    SSID_DISABLED_6G=0
    isEnabled_6=$(get_ssid_enable 17)
    if is_ssid_execution_succeed; then
        if [ "$isEnabled_6" != "true" ]; then
            radioenable_6=$(dmcli eRT getv Device.WiFi.Radio.3.Enable)
            isRadioExecutionSucceed_6=$(echo "$radioenable_6" | grep "Execution succeed")
            if [ "$isRadioExecutionSucceed_6" != "" ]; then
                isRadioEnabled_6=$(echo "$radioenable_6" | grep "false")
                if [ "$isRadioEnabled_6" != "" ]; then
                    echo_t "[RDKB_SELFHEAL] : Both 6G Radio(Radio 3) and 6G Private SSID are in DISABLED state"
                else
                    echo_t "[RDKB_SELFHEAL] : 6G Radio(Radio 3) is Enabled, only 6G private SSID is DISABLED"
                fi
            else
                echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 6G Radio status."
                echo "$radioenable_6"
            fi
            SSID_DISABLED_6G=1
            echo_t "[RDKB_SELFHEAL] : SSID 6GHZ is disabled"
            t2CountNotify "WIFI_INFO_6G_DISABLED"
        fi
    else
        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 6G Enable"
        sed -n 1,4p "$SSIDS_CACHE"
    fi
fi

if [ "$SELFHEAL_TYPE" = "BASE" ] || [ "$WiFi_Flag" = "false" ]; then
    # If bridge mode is not set and WiFI is not disabled by user,
    # check the status of SSID
    if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED -eq 0 ]; then
        ssidStatus_5=$(get_ssid_status 2)
        if is_ssid_execution_succeed; then
            if [ "$ssidStatus_5" != "Up" ]; then
                # We need to verify if it was a dmcli crash or is WiFi really down
                if [ "$ssidStatus_5" = "Down" ]; then
                    radioStatus_5=$(dmcli eRT getv Device.WiFi.Radio.2.Status)
                    isRadioExecutionSucceed_5=$(echo "$radioStatus_5" | grep "Execution succeed")
                    if [ "$isRadioExecutionSucceed_5" != "" ]; then
                        isRadioDown_5=$(echo "$radioStatus_5" | grep "Down")
                        if [ "$isRadioDown_5" != "" ]; then
                            echo_t "[RDKB_SELFHEAL] : Both 5G Radio(Radio 2) and 5G Private SSID are in DOWN state"
                        else
                            echo_t "[RDKB_SELFHEAL] : 5G Radio(Radio 2) is in up state, only 5G Private SSID is in DOWN state"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G Radio status."
                        echo "$radioStatus_5"
                    fi
                    echo_t "[RDKB_PLATFORM_ERROR] : 5G private SSID (ath1) is off."
                    t2CountNotify "WIFI_INFO_5GPrivateSSID_OFF"
                else
                    echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G status."
                    sed -n 1,4p "$SSIDS_CACHE"
                fi
            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 5G status."
            t2CountNotify "WIFI_ERROR_DMCLI_crash_5G_Status"
            sed -n 1,4p "$SSIDS_CACHE"
        fi
    fi

    # Check the status if 2.4GHz Wifi SSID
    SSID_DISABLED_2G=0
    isEnabled_2=$(get_ssid_enable 1)
    if is_ssid_execution_succeed; then
        if [ "$isEnabled_2" != "true" ]; then
            radioEnable_2=$(dmcli eRT getv Device.WiFi.Radio.1.Enable)
            radioExecution_2=$(echo "$radioEnable_2" | grep "Execution succeed")
            if [ "$radioExecution_2" != "" ]; then
                isRadioEnabled_2=$(echo "$radioEnable_2" | grep "false")
                if [ "$isRadioEnabled_2" != "" ]; then
                    echo_t "[RDKB_SELFHEAL] : Both 2G Radio(Radio 1) and 2G Private SSID are in DISABLED state"
                else
                    echo_t "[RDKB_SELFHEAL] : 2G Radio(Radio 1) is Enabled, only 2G Private SSID is DISABLED"
                fi
             else
                echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G Radio Enable"
                echo $radioEnable_2
            fi
            SSID_DISABLED_2G=1
            echo_t "[RDKB_SELFHEAL] : SSID 2.4GHZ is disabled"
            t2CountNotify "WIFI_INFO_2G_DISABLED"
        fi
    else
        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G Enable"
        sed -n 1,4p "$SSIDS_CACHE"
    fi

    # If bridge mode is not set and WiFI is not disabled by user,
    # check the status of SSID
    if [ $BR_MODE -eq 0 ] && [ $SSID_DISABLED_2G -eq 0 ]; then
        ssidStatus_2=$(get_ssid_status 1)
        if is_ssid_execution_succeed; then
            if [ "$ssidStatus_2" != "Up" ]; then
                # We need to verify if it was a dmcli crash or is WiFi really down
                if [ "$ssidStatus_2" = "Down" ]; then
                    radioStatus_2=$(dmcli eRT getv Device.WiFi.Radio.1.Status)
                    isRadioExecutionSucceed_2=$(echo "$radioStatus_2" | grep "Execution succeed")
                    if [ "$isRadioExecutionSucceed_2" != "" ]; then
                        isRadioDown_2=$(echo "$radioStatus_2" | grep "Down")
                        if [ "$isRadioDown_2" != "" ]; then
                            echo_t "[RDKB_SELFHEAL] : Both 2G Radio(Radio 1) and 2G Private SSID are in DOWN state"
                        else
                            echo_t "[RDKB_SELFHEAL] : 2G Radio(Radio 1) is in up state, only 2G Private SSID is in DOWN state"
                        fi
                    else
                        echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2G Radio status."
                        echo "$radioStatus_2"
                    fi
                    echo_t "[RDKB_PLATFORM_ERROR] : 2.4G private SSID (ath0) is off."
                    t2CountNotify "WIFI_INFO_2GPrivateSSID_OFF"
                else
                    echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G status."
                    echo "$ssidStatus_2"
                fi
            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : dmcli crashed or something went wrong while checking 2.4G status."
            t2CountNotify "WIFI_ERROR_DMCLI_crash_2G_Status"
            sed -n 1,4p "$SSIDS_CACHE"
        fi
    fi
fi

FIREWALL_ENABLED=$(get_from_syscfg_cache firewall_enabled)

echo_t "[RDKB_SELFHEAL] : BRIDGE_MODE is $BR_MODE"
if [ $BR_MODE -eq 1 ]; then
    t2CountNotify "SYS_INFO_BridgeMode"
fi
echo_t "[RDKB_SELFHEAL] : FIREWALL_ENABLED is $FIREWALL_ENABLED"

#Check whether private SSID's are broadcasting during bridge-mode or not
#if broadcasting then we need to disable that SSID's for pseduo mode(2)
#if device is in full bridge-mode(3) then we need to disable both radio and SSID's
if [ $BR_MODE -eq 1 ]; then

    isBridging=$(get_from_syscfg_cache bridge_mode)
    echo_t "[RDKB_SELFHEAL] : BR_MODE:$isBridging"

    #full bridge-mode(3)
    if [ "$isBridging" = "3" ]; then
        # Check the status if 2.4GHz Wifi Radio
        RADIO_ENABLED_2G=0
        RadioEnable_2=$(fast_dmcli eRT getv Device.WiFi.Radio.1.Enable)
        RadioExecution_2=$(echo "$RadioEnable_2" | grep "Execution succeed")

        if [ "$RadioExecution_2" != "" ]; then
            isEnabled_2=$(echo "$RadioEnable_2" | grep "true")
            if [ "$isEnabled_2" != "" ]; then
                RADIO_ENABLED_2G=1
                echo_t "[RDKB_SELFHEAL] : Radio 2.4GHZ is Enabled"
            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 2.4G radio Enable"
            echo "$RadioEnable_2"
        fi

        # Check the status if 5GHz Wifi Radio
        RADIO_ENABLED_5G=0
        RadioEnable_5=$(fast_dmcli eRT getv Device.WiFi.Radio.2.Enable)
        RadioExecution_5=$(echo "$RadioEnable_5" | grep "Execution succeed")

        if [ "$RadioExecution_5" != "" ]; then
            isEnabled_5=$(echo "$RadioEnable_5" | grep "true")
            if [ "$isEnabled_5" != "" ]; then
                RADIO_ENABLED_5G=1
                echo_t "[RDKB_SELFHEAL] : Radio 5GHZ is Enabled"
            fi
        else
            echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 5G radio Enable"
            echo "$RadioEnable_5"
        fi

        if [ $RADIO_COUNT -eq 3 ]; then
            # Check the status if 6GHz Wifi Radio
            RADIO_ENABLED_6G=0
            RadioEnable_6=$(fast_dmcli eRT getv Device.WiFi.Radio.3.Enable)
            RadioExecution_6=$(echo "$RadioEnable_5" | grep "Execution succeed")

            if [ "$RadioExecution_6" != "" ]; then
                isEnabled_6=$(echo "$RadioEnable_6" | grep "true")
                if [ "$isEnabled_6" != "" ]; then
                    RADIO_ENABLED_6G=1
                    echo_t "[RDKB_SELFHEAL] : Radio 6GHZ is Enabled"
                fi
            else
                echo_t "[RDKB_PLATFORM_ERROR] : Something went wrong while checking 6G radio Enable"
                echo "$RadioEnable_6"
            fi
        fi
        if [ $RADIO_COUNT -eq 3 ]; then
            if [ $RADIO_ENABLED_5G -eq 1 ] || [ $RADIO_ENABLED_2G -eq 1 ] || [ $RADIO_ENABLED_6G -eq 1 ]; then
                dmcli eRT setv Device.WiFi.Radio.1.Enable bool false
                sleep 2
                dmcli eRT setv Device.WiFi.Radio.2.Enable bool false
                sleep 2
                dmcli eRT setv Device.WiFi.Radio.3.Enable bool false
                sleep 2
                dmcli eRT setv Device.WiFi.SSID.3.Enable bool false
                sleep 2
                IsNeedtoDoApplySetting=1
            fi
        else
            if [ $RADIO_ENABLED_5G -eq 1 ] || [ $RADIO_ENABLED_2G -eq 1 ]; then
                dmcli eRT setv Device.WiFi.Radio.1.Enable bool false
                sleep 2
                dmcli eRT setv Device.WiFi.Radio.2.Enable bool false
                sleep 2
                dmcli eRT setv Device.WiFi.SSID.3.Enable bool false
                sleep 2
                IsNeedtoDoApplySetting=1
            fi
        fi
    fi

    if [ $RADIO_COUNT -eq 3 ]; then
        if [ $SSID_DISABLED_2G -eq 0 ] || [ $SSID_DISABLED -eq 0 ] || [ $SSID_DISABLED_6G -eq 0 ]; then
            dmcli eRT setv Device.WiFi.SSID.1.Enable bool false
            sleep 2
            dmcli eRT setv Device.WiFi.SSID.2.Enable bool false
            sleep 2
            dmcli eRT setv Device.WiFi.SSID.17.Enable bool false
            sleep 2
            IsNeedtoDoApplySetting=1
        fi
    else
        if [ $SSID_DISABLED_2G -eq 0 ] || [ $SSID_DISABLED -eq 0 ]; then
            dmcli eRT setv Device.WiFi.SSID.1.Enable bool false
            sleep 2
            dmcli eRT setv Device.WiFi.SSID.2.Enable bool false
            sleep 2
            IsNeedtoDoApplySetting=1
        fi
    fi

    if [ $RADIO_COUNT -eq 3 ]; then
        if [ "$IsNeedtoDoApplySetting" = "1" ]; then
            dmcli eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool true
            sleep 3
            dmcli eRT setv Device.WiFi.Radio.2.X_CISCO_COM_ApplySetting bool true
            sleep 3
            dmcli eRT setv Device.WiFi.Radio.3.X_CISCO_COM_ApplySetting bool true
            sleep 3
            dmcli eRT setv Device.WiFi.X_CISCO_COM_ResetRadios bool true
        fi
    else
        if [ "$IsNeedtoDoApplySetting" = "1" ]; then
            dmcli eRT setv Device.WiFi.Radio.1.X_CISCO_COM_ApplySetting bool true
            sleep 3
            dmcli eRT setv Device.WiFi.Radio.2.X_CISCO_COM_ApplySetting bool true
            sleep 3
            dmcli eRT setv Device.WiFi.X_CISCO_COM_ResetRadios bool true
        fi
    fi
fi

if [ $BR_MODE -eq 0 ]; then
    iptables-save -t nat | grep "A PREROUTING -i"
    if [ $? -eq 1 ]; then
        echo_t "[RDKB_PLATFORM_ERROR] : iptable corrupted."
        t2CountNotify "SYS_ERROR_iptable_corruption"
        #sysevent set firewall-restart
    fi
fi

if [ -s /tmp/.ipv4table_error ] || [ -s /tmp/.ipv6table_error ];then
	firewall_selfheal_count=$(sysevent get firewall_selfheal_count)
	if [ "$firewall_selfheal_count" = "" ];then
		firewall_selfheal_count=0
	fi
	if [ $firewall_selfheal_count -lt 3 ];then
		echo_t "[RDKB_SELFHEAL] : iptables error , restarting firewall"
		echo ">>>> /tmp/.ipv4table_error <<<<"
		cat /tmp/.ipv4table_error
		echo ">>>> /tmp/.ipv6table_error <<<<"
		cat /tmp/.ipv6table_error
		sysevent set firewall-restart
		firewall_selfheal_count=$((firewall_selfheal_count + 1))
		sysevent set firewall_selfheal_count $firewall_selfheal_count
		echo_t "[RDKB_SELFHEAL] : firewall_selfheal_count is $firewall_selfheal_count"
	else
		echo_t "[RDKB_SELFHEAL] : max firewall_selfheal_count reached, not restarting firewall"
	fi
fi

case $SELFHEAL_TYPE in
    "BASE"|"SYSTEMD")
        if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ] && [ "$thisIS_BCI" != "yes" ] && [ $BR_MODE -eq 0 ] && [ ! -f "$brlan1_firewall" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
            firewall_rules=$(iptables-save)
            check_if_brlan1=$(echo "$firewall_rules" | grep "brlan1")
            if [ "$check_if_brlan1" = "" ]; then
                echo_t "[RDKB_PLATFORM_ERROR]:brlan1_firewall_rules_missing,restarting firewall"
                sysevent set firewall-restart
            fi
            touch $brlan1_firewall
        fi
    ;;
    "TCCBR")
    ;;
esac

#Logging to check the DHCP range corruption
lan_ipaddr=$(get_from_syscfg_cache lan_ipaddr)
lan_netmask=$(get_from_syscfg_cache lan_netmask)
echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : lan_ipaddr = $lan_ipaddr lan_netmask = $lan_netmask"

lost_and_found_enable=$(get_from_syscfg_cache lost_and_found_enable)
echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] :  lost_and_found_enable = $lost_and_found_enable"
if [ "$lost_and_found_enable" = "true" ]; then
    iot_ifname=$(get_from_syscfg_cache iot_ifname)
    if [ "$iot_ifname" = "l2sd0.106" ]; then
        iot_ifname=$(get_from_syscfg_cache iot_brname)
    fi
    iot_dhcp_start=$(get_from_syscfg_cache iot_dhcp_start)
    iot_dhcp_end=$(get_from_syscfg_cache iot_dhcp_end)
    iot_netmask=$(get_from_syscfg_cache iot_netmask)
    echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : DHCP server configuring for IOT iot_ifname = $iot_ifname "
    echo_t "[RDKB_SELFHEAL] [DHCPCORRUPT_TRACE] : iot_dhcp_start = $iot_dhcp_start iot_dhcp_end=$iot_dhcp_end iot_netmask=$iot_netmask"
fi

#Checking whether dnsmasq is running or not and if zombie for XF3
if [ "$thisWAN_TYPE" = "EPON" ]; then
    DNS_PID=$(get_pid dnsmasq)
    if [ "$DNS_PID" = "" ]; then
        InterfaceInConf=""
        Bridge_Mode_t=$(get_from_syscfg_cache bridge_mode)
        InterfaceInConf=$(grep "interface=" /var/dnsmasq.conf)
        if [ "$InterfaceInConf" = "" ] && [ "0" != "$Bridge_Mode_t" ] ; then
            if [ ! -f /tmp/dnsmaq_noiface ]; then
                echo_t "[RDKB_SELFHEAL] : Unit in bridge mode,interface info not available in dnsmasq.conf"
                touch /tmp/dnsmaq_noiface
            fi
        else
            echo_t "[RDKB_SELFHEAL] : dnsmasq is not running"
            t2CountNotify "SYS_SH_dnsmasq_restart"
        fi
    else
        if [ -f /tmp/dnsmaq_noiface ]; then
            rm -rf /tmp/dnsmaq_noiface
        fi
    fi
    # ARRIS XB6 => MODEL_NUM=TG3482G
    # Tech CBR  => MODEL_NUM=CGA4131COM
    # Tech xb6  => MODEL_NUM=CGM4140COM
    # Tech XB7  => MODEL_NUM=CGM4331COM
    # CMX  XB7  => MODEL_NUM=TG4482A
    # Tech CBR2 => MODEL_NUM=CGA4332COM
    # Tech XB8  => MODEL_NUM=CGM4981COM
    # This critical processes checking is handled in selfheal_aggressive.sh for above platforms
    # Ref: RDKB-25546
    if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
	   [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] && [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ]
    then
    checkIfDnsmasqIsZombie=$(grep "dnsmasq" $PROCESSES_CACHE | grep "Z" | awk '{ print $1 }')
    if [ "$checkIfDnsmasqIsZombie" != "" ] ; then
        for zombiepid in $checkIfDnsmasqIsZombie
          do
            confirmZombie=$(grep "State:" /proc/$zombiepid/status | grep -i "zombie")
            if [ "$confirmZombie" != "" ] ; then
                case $SELFHEAL_TYPE in
                    "BASE")
                    ;;
                    "TCCBR")
                    ;;
                    "SYSTEMD")
                        echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, stopping CcspXdns"
                        t2CountNotify "SYS_ERROR_Zombie_dnsmasq"
                        systemctl stop CcspXdnsSsp.service
                    ;;
                esac
                echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, restarting dnsmasq"
                t2CountNotify "SYS_ERROR_Zombie_dnsmasq"
                kill -9 "$(get_pid dnsmasq)"
                systemctl stop dnsmasq
                systemctl start dnsmasq
                case $SELFHEAL_TYPE in
                    "BASE")
                    ;;
                    "TCCBR")
                    ;;
                    "SYSTEMD")
                        echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, restarting CcspXdns"
                        t2CountNotify "SYS_ERROR_Zombie_dnsmasq"
                        systemctl start CcspXdnsSsp.service
                    ;;
                esac
                break
            fi
          done
    fi
    fi
fi

#Checking whether dnsmasq is running or not
if [ "$thisWAN_TYPE" != "EPON" ]; then
    DNS_PID=$(get_pid dnsmasq)
    if [ "$DNS_PID" = "" ]; then
        InterfaceInConf=""
        Bridge_Mode_t=$(get_from_syscfg_cache bridge_mode)
        InterfaceInConf=$(grep "interface=" /var/dnsmasq.conf)
        if [ "$InterfaceInConf" = "" ] && [ "0" != "$Bridge_Mode_t" ] ; then
            if [ ! -f /tmp/dnsmaq_noiface ]; then
                echo_t "[RDKB_SELFHEAL] : Unit in bridge mode,interface info not available in dnsmasq.conf"
                touch /tmp/dnsmaq_noiface
            fi
        else
            if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ] ; then
                echo_t "[RDKB_SELFHEAL] : dnsmasq is not running"
                t2CountNotify "SYS_SH_dnsmasq_restart"
            fi
        fi
    else
        brlan0up=$(grep "brlan0" /var/dnsmasq.conf)
        brlan1up="NA"
        infup="NA"

        #TODO: Need to revisit this after enabling CcspHomeSecurity and LnF network

#       case $SELFHEAL_TYPE in
#           "BASE")
#               brlan1up=$(grep "brlan1" /var/dnsmasq.conf)
#               lnf_ifname=$(syscfg get iot_ifname)
#               if [ "$lnf_ifname" = "l2sd0.106" ]; then
#                   lnf_ifname=$(syscfg get iot_brname)
#               fi
#               if [ "$lnf_ifname" != "" ]; then
#                   echo_t "[RDKB_SELFHEAL] : LnF interface is: $lnf_ifname"
#                   infup=$(grep "$lnf_ifname" /var/dnsmasq.conf)
#               else
#                   echo_t "[RDKB_SELFHEAL] : LnF interface not available in DB"
#                   #Set some value so that dnsmasq won't restart
#                   infup="NA"
#               fi
#           ;;
#           "TCCBR")
#           ;;
#           "SYSTEMD")
#               brlan1up=$(grep "brlan1" /var/dnsmasq.conf)
#           ;;
#       esac

        if [ -f /tmp/dnsmaq_noiface ]; then
            rm -rf /tmp/dnsmaq_noiface
        fi
        IsAnyOneInfFailtoUp=0

        if [ $BR_MODE -eq 0 ]; then
            if [ "$brlan0up" = "" ]; then
                echo_t "[RDKB_SELFHEAL] : brlan0 info is not availble in dnsmasq.conf"
                IsAnyOneInfFailtoUp=1
            fi
        fi

        case $SELFHEAL_TYPE in
            "BASE"|"SYSTEMD")
                if [ "$thisIS_BCI" != "yes" ] && [ "$brlan1up" = "" ] && [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
                    echo_t "[RDKB_SELFHEAL] : brlan1 info is not availble in dnsmasq.conf"
                    IsAnyOneInfFailtoUp=1
                fi
            ;;
            "TCCBR")
            ;;
        esac

        case $SELFHEAL_TYPE in
            "BASE")
                if [ "$infup" = "" ]; then
                    echo_t "[RDKB_SELFHEAL] : $lnf_ifname info is not availble in dnsmasq.conf"
                    IsAnyOneInfFailtoUp=1
                fi
            ;;
            "TCCBR")
            ;;
            "SYSTEMD")
            ;;
        esac

        if [ ! -f /tmp/dnsmasq_restarted_via_selfheal ]; then
            if [ $IsAnyOneInfFailtoUp -eq 1 ]; then
                touch /tmp/dnsmasq_restarted_via_selfheal

                echo_t "[RDKB_SELFHEAL] : dnsmasq.conf is."
                cat /var/dnsmasq.conf

                echo_t "[RDKB_SELFHEAL] : Setting an event to restart dnsmasq"
                sysevent set dhcp_server-restart
            fi
        fi

        case $SELFHEAL_TYPE in
            "BASE"|"SYSTEMD"|"TCCBR")
		# ARRIS XB6 => MODEL_NUM=TG3482G
		# Tech CBR  => MODEL_NUM=CGA4131COM
		# Tech xb6  => MODEL_NUM=CGM4140COM
		# Tech XB7  => MODEL_NUM=CGM4331COM
		# Tech CBR2 => MODEL_NUM=CGA4332COM
		# CMX  XB7  => MODEL_NUM=TG4482A
                # Tech XB8  => MODEL_NUM=CGM4981COM
		# This critical processes checking is handled in selfheal_aggressive.sh for above platforms
		# Ref: RDKB-25546
		if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
		       [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] && [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ]
		then
                checkIfDnsmasqIsZombie=$(awk '/Z.*dnsmasq/{ print $1 }' $PROCESSES_CACHE)
                if [ "$checkIfDnsmasqIsZombie" != "" ] ; then
                    for zombiepid in $checkIfDnsmasqIsZombie
                      do
                        confirmZombie=$(grep "State:" /proc/$zombiepid/status | grep -i "zombie")
                        if [ "$confirmZombie" != "" ] ; then
                            echo_t "[RDKB_SELFHEAL] : Zombie instance of dnsmasq is present, restarting dnsmasq"
                            t2CountNotify "SYS_ERROR_Zombie_dnsmasq"
                            kill -9 "$(get_pid dnsmasq)"
                            sysevent set dhcp_server-restart
                            break
                        fi
                      done
                fi
		fi
            ;;
        esac
    fi   # [ "$DNS_PID" = "" ]
fi  # [ "$thisWAN_TYPE" != "EPON" ]

case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        #Checking ipv6 dad failure and restart dibbler client [TCXB6-5169]
        CHKIPV6_DAD_FAILED=$(ip -6 addr show dev erouter0 | grep "scope link tentative dadfailed")
        if [ "$CHKIPV6_DAD_FAILED" != "" ]; then
            echo_t "link Local DAD failed"
            t2CountNotify "SYS_ERROR_linkLocalDad_failed"
            if [ "$BOX_TYPE" = "XB6" -a "$SOC_TYPE" = "Broadcom" ] ; then
                partner_id=$(get_from_syscfg_cache PartnerID)
                if [ "$partner_id" != "comcast" ]; then
                    dibbler-client stop
                    sysctl -w net.ipv6.conf.erouter0.disable_ipv6=1
                    sysctl -w net.ipv6.conf.erouter0.accept_dad=0
                    sysctl -w net.ipv6.conf.erouter0.disable_ipv6=0
                    sysctl -w net.ipv6.conf.erouter0.accept_dad=1
                    dibbler-client start
                    echo_t "IPV6_DAD_FAILURE : successfully recovered for partner id $partner_id"
                    t2ValNotify "dadrecoverypartner_split" "$partner_id"
                fi
            fi
        fi
    ;;
esac

# ARRIS XB6 => MODEL_NUM=TG3482G
# Tech CBR  => MODEL_NUM=CGA4131COM
# Tech xb6  => MODEL_NUM=CGM4140COM
# Tech XB7  => MODEL_NUM=CGM4331COM
# Tech CBR2 => MODEL_NUM=CGA4332COM
# CMX  XB7  => MODEL_NUM=TG4482A
# Tech XB8  => MODEL_NUM=CGM4981COM
# This critical processes checking is handled in selfheal_aggressive.sh for above platforms
# Ref: RDKB-25546
if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
       [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] && [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ]
then
#Checking dibbler server is running or not RDKB_10683
DIBBLER_PID=$(get_pid dibbler-server)
if [ "$DIBBLER_PID" = "" ]; then
#   IPV6_STATUS=`sysevent get ipv6-status`
    DHCPV6C_ENABLED=$(sysevent get dhcpv6c_enabled)
    routerMode="`get_from_syscfg_cache last_erouter_mode`"

    if [ "$BOX_TYPE" = "HUB4" ]; then
        #Since dibbler client not supported for hub4
        DHCPV6C_ENABLED="1"
    fi

    if [ "$BR_MODE" = "0" ] && [ "$DHCPV6C_ENABLED" = "1" ]; then
        Sizeof_ServerConf=`stat -c %s $DIBBLER_SERVER_CONF`
        DHCPv6_ServerType="`get_from_syscfg_cache dhcpv6s00::servertype`"
        case $SELFHEAL_TYPE in
            "BASE"|"TCCBR")
                DHCPv6EnableStatus=$(get_from_syscfg_cache dhcpv6s00::serverenable)
                if [ "$IS_BCI" = "yes" ] && [ "0" = "$DHCPv6EnableStatus" ]; then
                    echo_t "DHCPv6 Disabled. Restart of Dibbler process not Required"
                elif [ "$routerMode" = "1" ] || [ "$routerMode" = "" ] || [ "$Unit_Activated" = "0" ]; then
                        #TCCBR-4398 erouter0 not getting IPV6 prefix address from CMTS so as brlan0 also not getting IPV6 address.So unable to start dibbler service.
                        echo_t "DIBBLER : Non IPv6 mode dibbler server.conf file not present"
                else
                    if [ "$DHCPv6_ServerType" -ne 2 ] || [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ];then
                        echo_t "RDKB_PROCESS_CRASHED : Dibbler is not running, restarting the dibbler"
                        t2CountNotify "SYS_SH_Dibbler_restart"
                    fi
                    if [ -f "/etc/dibbler/server.conf" ]; then
                        BRLAN_CHKIPV6_DAD_FAILED=$(ip -6 addr show dev $PRIVATE_LAN | grep "scope link tentative dadfailed")
                        if [ "$BRLAN_CHKIPV6_DAD_FAILED" != "" ]; then
                            echo "DADFAILED : BRLAN0_DADFAILED"
                            t2CountNotify "SYS_ERROR_Dibbler_DAD_failed"


                            if [ "$BOX_TYPE" = "TCCBR" ]; then
                                echo "DADFAILED : Recovering device from DADFAILED state"
                                echo "1" > /proc/sys/net/ipv6/conf/$PRIVATE_LAN/disable_ipv6
                                sleep 1
                                echo "0" > /proc/sys/net/ipv6/conf/$PRIVATE_LAN/disable_ipv6
                                sleep 1
                                Dhcpv6_Client_restart "dibbler-client" "Idle"
                            fi
                        elif [ $Sizeof_ServerConf -le 1 ]; then
                            Dhcpv6_Client_restart "$DHCPv6_TYPE" "restart_for_dibbler-server"
                            ret_val=`echo $?`
                            if [ "$ret_val" = "1" ];then
                                echo "DIBBLER : Dibbler Server Config is empty"
                                t2CountNotify "SYS_ERROR_DibblerServer_emptyconf"
                            fi
                        elif [ "$DHCPv6_ServerType" -eq 2 ] && [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ]  && [ "$BOX_TYPE" != "WNXL11BWL" ];then
                            #if servertype is stateless(1-stateful,2-stateless),the ip assignment will be done through zebra process.Hence dibbler-server won't required.
                            echo_t "DHCPv6 servertype is stateless,dibbler-server restart not required"
                        else
                            dibbler-server stop
                            sleep 2
                            dibbler-server start
                        fi
                    else
                        echo_t "RDKB_PROCESS_CRASHED : dibbler server.conf file not present"
                        Dhcpv6_Client_restart "$DHCPv6_TYPE" "restart_for_dibbler-server"
                        ret_val=`echo $?`
                        if [ "$ret_val" = "2" ];then
                            echo_t "DIBBLER : Restart of dibbler failed with reason 2"
                        fi
                    fi
                fi
            ;;
            "SYSTEMD")
                #ARRISXB6-7776 .. check if IANAEnable is set to 0
                IANAEnable=$(syscfg show | grep "dhcpv6spool00::IANAEnable" | cut -d"=" -f2)
                if [ "$IANAEnable" = "0" ] ; then
                    echo "[$(getDateTime)] IANAEnable disabled, enable and restart dhcp6 client and dibbler"
                    syscfg set dhcpv6spool00::IANAEnable 1
                    syscfg commit
                    sleep 2
                    #need to restart dhcp client to generate dibbler conf
                    Dhcpv6_Client_restart "ti_dhcp6c" "Idle"
                elif [ "$routerMode" = "1" ] || [ "$routerMode" = "" ] || [ "$Unit_Activated" = "0" ]; then
                    #TCCBR-4398 erouter0 not getting IPV6 prefix address from CMTS so as brlan0 also not getting IPV6 address.So unable to start dibbler service.
                    echo_t "DIBBLER : Non IPv6 mode dibbler server.conf file not present"
                else
                    if [ "$DHCPv6_ServerType" -ne 2 ] || [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR300" ];then
                        echo_t "RDKB_PROCESS_CRASHED : Dibbler is not running, restarting the dibbler"
                        t2CountNotify "SYS_SH_Dibbler_restart"
                    fi
                    if [ -f "/etc/dibbler/server.conf" ]; then
                        BRLAN_CHKIPV6_DAD_FAILED=$(ip -6 addr show dev $PRIVATE_LAN | grep "scope link tentative dadfailed")
                        if [ "$BRLAN_CHKIPV6_DAD_FAILED" != "" ]; then
                            echo "DADFAILED : BRLAN0_DADFAILED"
                            t2CountNotify "SYS_ERROR_Dibbler_DAD_failed"

                            if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Technicolor" ] ; then
                                echo "DADFAILED : Recovering device from DADFAILED state"
                                # save global ipv6 address before disable it
                                v6addr=$(ip -6 addr show dev $PRIVATE_LAN | grep -i "global" | awk '{print $2}')
                                echo "1" > /proc/sys/net/ipv6/conf/$PRIVATE_LAN/disable_ipv6
                                sleep 1
                                echo "0" > /proc/sys/net/ipv6/conf/$PRIVATE_LAN/disable_ipv6
                                # re-add global ipv6 address after enabled it
                                ip -6 addr add $v6addr dev $PRIVATE_LAN
                                sleep 1

                                dibbler-server stop
                                sleep 1
                                dibbler-server start
                                sleep 5
                            elif [ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "TG4482A" ]; then
                                echo "DADFAILED : Recovering device from DADFAILED state"
                                # save global ipv6 address before disable it
                                v6addr=$(ip -6 addr show dev $PRIVATE_LAN | grep -i "global" | awk '{print $2}')
                                sh $DHCPV6_HANDLER disable
                                sysctl -w net.ipv6.conf.$PRIVATE_LAN.disable_ipv6=1
                                sysctl -w net.ipv6.conf.$PRIVATE_LAN.accept_dad=0
                                sleep 1
                                sysctl -w net.ipv6.conf.$PRIVATE_LAN.disable_ipv6=0
                                sysctl -w net.ipv6.conf.$PRIVATE_LAN.accept_dad=1
                                sleep 1
                                sh $DHCPV6_HANDLER enable
                                # re-add global ipv6 address after enabled it
                                ip -6 addr add $v6addr dev $PRIVATE_LAN
                                sleep 5
                            fi
                        elif [ $Sizeof_ServerConf -le 1 ]; then
                            if [ "$BOX_TYPE" = "HUB4" ]; then
                                echo "DIBBLER : Dibbler Server Config is empty"
                                echo "DIBBLER : Setting 'sysevent dibblerServer-restart restart' to trigger PAM for dibbler-server file creation"
                                sysevent set dibblerServer-restart restart
                            else
                                Dhcpv6_Client_restart "$DHCPv6_TYPE" "restart_for_dibbler-server"
                                ret_val=`echo $?`
                                if [ "$ret_val" = "1" ];then
                                    echo "DIBBLER : Dibbler Server Config is empty"
                                    t2CountNotify "SYS_ERROR_DibblerServer_emptyconf"
                                fi
                            fi
                        elif [ "$DHCPv6_ServerType" -eq 2 ] && [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "WNXL11BWL" ];then
                            #if servertype is stateless(1-stateful,2-stateless),the ip assignment will be done through zebra process.Hence dibbler-server won't required.
                            echo_t "DHCPv6 servertype is stateless,dibbler-server restart not required"
                        else
                            dibbler-server stop
                            sleep 2
                            dibbler-server start
                        fi
                    else
                        echo_t "RDKB_PROCESS_CRASHED : dibbler server.conf file not present"
                        Dhcpv6_Client_restart "$DHCPv6_TYPE" "restart_for_dibbler-server"
                        ret_val=`echo $?`
                        if [ "$ret_val" = "2" ];then
                           echo_t "DIBBLER : Restart of dibbler failed with reason 2"
                        fi
                    fi
                fi
            ;;
        esac
    fi
fi
fi

#Checking the zebra is running or not
WAN_STATUS=$(sysevent get wan-status)
ZEBRA_PID=$(get_pid zebra)
if [ "$ZEBRA_PID" = "" ] && [ "$WAN_STATUS" = "started" ]; then
    if [ "$BR_MODE" = "0" ]; then

        echo_t "RDKB_PROCESS_CRASHED : zebra is not running, restarting the zebra"
        t2CountNotify "SYS_SH_Zebra_restart"
        /etc/utopia/registration.d/20_routing restart
        sysevent set zebra-restart
    fi
fi

case $SELFHEAL_TYPE in
    "BASE")
        #Checking the ntpd is running or not
        if [ "$WAN_TYPE" != "EPON" ] && [ "$BOX_TYPE" = "MV1" ]; then
	    #TODO: Revisit when NTP daemon is enabled
#           NTPD_PID=$(busybox pidof ntpd)
#           if [ "$NTPD_PID" = "" ]; then
#               echo_t "RDKB_PROCESS_CRASHED : NTPD is not running, restarting the NTPD"
#               sysevent set ntpd-restart
#           fi


            #Checking if rpcserver is running
            RPCSERVER_PID=$(get_pid rpcserver)
            if [ "$RPCSERVER_PID" = "" ] && [ -f /usr/bin/rpcserver ]; then
                echo_t "RDKB_PROCESS_CRASHED : RPCSERVER is not running on ARM side, restarting "
                /usr/bin/rpcserver &
            fi
        fi
    ;;
    "TCCBR")
        #Checking the ntpd is running or not for TCCBR
        if [ "$WAN_TYPE" != "EPON" ]; then
            NTPD_PID=$(get_pid ntpd)
            if [ "$NTPD_PID" = "" ]; then
                echo_t "RDKB_PROCESS_CRASHED : NTPD is not running, restarting the NTPD"
                sysevent set ntpd-restart
            fi
        fi
    ;;
    "SYSTEMD")
        #All CCSP Processes Now running on Single Processor. Add those Processes to Test & Diagnostic
    ;;
esac

# ARRIS XB6 => MODEL_NUM=TG3482G
# Tech CBR  => MODEL_NUM=CGA4131COM
# Tech xb6  => MODEL_NUM=CGM4140COM
# Tech XB7  => MODEL_NUM=CGM4331COM
# Tech CBR2 => MODEL_NUM=CGA4332COM
# CMX  XB7  => MODEL_NUM=TG4482A
# Tech XB8  => MODEL_NUM=CGM4981COM
# This critical processes checking is handled in selfheal_aggressive.sh for above platforms
# Ref: RDKB-25546
if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
       [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] && [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ]
then
# Checking for WAN_INTERFACE ipv6 address
DHCPV6_ERROR_FILE="/tmp/.dhcpv6SolicitLoopError"
WAN_STATUS=$(sysevent get wan-status)
WAN_IPv4_Addr=$(ifconfig $WAN_INTERFACE | grep "inet" | grep -v "inet6")
#DHCPV6_HANDLER="/etc/utopia/service.d/service_dhcpv6_client.sh"

case $SELFHEAL_TYPE in
    "BASE"|"SYSTEMD")
        if [ "$WAN_STATUS" != "started" ]; then
            echo_t "WAN_STATUS : wan-status is $WAN_STATUS"
            if [ "$WAN_STATUS" = "stopped" ]; then
                t2CountNotify "RF_ERROR_WAN_stopped"
            fi
        fi
    ;;
    "TCCBR")
    ;;
esac

if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ]  && [ "$BOX_TYPE" != "SR213" ]  && [ "$BOX_TYPE" != "WNXL11BWL" ] && [ -f "$DHCPV6_ERROR_FILE" ] && [ "$WAN_STATUS" = "started" ] && [ "$WAN_IPv4_Addr" != "" ]; then
    isIPv6=$(ifconfig $WAN_INTERFACE | grep "inet6" | grep "Scope:Global")
    echo_t "isIPv6 = $isIPv6"
    if [ "$isIPv6" = "" ] && [ "$Unit_Activated" != "0" ]; then
        case $SELFHEAL_TYPE in
            "BASE"|"SYSTEMD")
                echo_t "[RDKB_SELFHEAL] : $DHCPV6_ERROR_FILE file present and $WAN_INTERFACE ipv6 address is empty, restarting dhcpv6 client"
            ;;
            "TCCBR")
                echo_t "[RDKB_SELFHEAL] : $DHCPV6_ERROR_FILE file present and $WAN_INTERFACE ipv6 address is empty, restarting ti_dhcp6c"
            ;;
        esac
        rm -rf $DHCPV6_ERROR_FILE
	Dhcpv6_Client_restart "ti_dhcp6c" "Idle"
    fi
fi
#Logic added in reference to RDKB-25714
#to check erouter0 has Global IPv6 or not and accordingly kill the process
#responsible for the same. The killed processes will get restarted
#by the later stages of this script.
erouter0_up_check=$(ifconfig $WAN_INTERFACE | grep "UP")
erouter0_globalv6_test=$(ifconfig $WAN_INTERFACE | grep "inet6" | grep "Scope:Global" | awk '{print $(NF-1)}' | cut -f1 -d":")
erouter_mode_check=$(get_from_syscfg_cache last_erouter_mode) #Check given for non IPv6 bootfiles RDKB-27963
IPV6_STATUS_CHECK_GIPV6=$(sysevent get ipv6-status) #Check given for non IPv6 bootfiles RDKB-27963
if [ "$erouter0_globalv6_test" = "" ] && [ "$WAN_STATUS" = "started" ] && [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then
    case $SELFHEAL_TYPE in
        "SYSTEMD")
            if [ "$erouter0_up_check" = "" ]; then
                echo_t "[RDKB_SELFHEAL] : erouter0 is DOWN, making it UP"
                ifconfig $WAN_INTERFACE up
            fi
            if ( [ "x$IPV6_STATUS_CHECK_GIPV6" != "x" ] || [ "x$IPV6_STATUS_CHECK_GIPV6" != "xstopped" ] ) && [ "$erouter_mode_check" -ne 1 ] && [ "$Unit_Activated" != "0" ]; then
            if [ "$SOC_TYPE" = "Broadcom" ] && [ "$BOX_TYPE" = "XB6" ]; then
                echo_t "[RDKB_SELFHEAL] : Killing dibbler as Global IPv6 not attached"
                /usr/sbin/dibbler-client stop
            elif [ "$BOX_TYPE" = "XB6" ]; then
                echo_t "DHCP_CLIENT : Killing DHCP Client for v6 as Global IPv6 not attached"
                sh $DHCPV6_HANDLER disable
            fi
            fi
        ;;
        "BASE")
            if ( [ "x$IPV6_STATUS_CHECK_GIPV6" != "x" ] || [ "x$IPV6_STATUS_CHECK_GIPV6" != "xstopped" ] ) && [ "$erouter_mode_check" -ne 1 ] && [ "$Unit_Activated" != "0" ]; then
            task_to_be_killed=$(awk '/dhcp6c.*erouter0/{ print $1 }' $PROCESSES_CACHE)
            if [ "$erouter0_up_check" = "" ]; then
                echo_t "[RDKB_SELFHEAL] : erouter0 is DOWN, making it UP"
                ifconfig $WAN_INTERFACE up
                #Adding to kill ipv4 process to solve RDKB-27177
                task_to_kill=$(awk '/udhcpc.*erouter/{ print $1 }' $PROCESSES_CACHE)
                if [ "x$task_to_kill" != "x" ]; then
                    kill $task_to_kill
                fi
                #RDKB-27177 fix ends here
            fi
            if [ "$task_to_be_killed" != "" ]; then
                echo_t "DHCP_CLIENT : Killing DHCP Client for v6 as Global IPv6 not attached"
                kill "$task_to_be_killed"
                sleep 3
            fi
            fi
        ;;
        "TCCBR")
            if [ "$erouter0_up_check" = "" ]; then
                echo_t "[RDKB_SELFHEAL] : erouter0 is DOWN, making it UP"
                ifconfig $WAN_INTERFACE up
            fi
            if ( [ "x$IPV6_STATUS_CHECK_GIPV6" != "x" ] || [ "x$IPV6_STATUS_CHECK_GIPV6" != "xstopped" ] ) && [ "$erouter_mode_check" -ne 1 ] && [ "$Unit_Activated" != "0" ]; then
            echo_t "[RDKB_SELFHEAL] : Killing dibbler as Global IPv6 not attached"
            /usr/sbin/dibbler-client stop
            fi
        ;;
    esac
else
    echo_t "[RDKB_SELFHEAL] : Global IPv6 is present"
fi
#Logic ends here for RDKB-25714
wan_dhcp_client_v4=1
wan_dhcp_client_v6=1
if [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ]  && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ] && [ "$WAN_STATUS" = "started" ]; then
    wan_dhcp_client_v4=1
    wan_dhcp_client_v6=1

    #Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    LAST_EROUTER_MODE=`get_from_syscfg_cache last_erouter_mode`

    case $SELFHEAL_TYPE in
        "BASE"|"SYSTEMD")
            UDHCPC_Enable=$(get_from_syscfg_cache UDHCPEnable)

            if ( [ "$SOC_TYPE" = "Broadcom" ] && [ "$BOX_TYPE" != "XB3" ] ) || [ "$WAN_TYPE" = "EPON" ]; then
                check_wan_dhcp_client_v4=$(grep -E 'udhcpc.*erouter' $PROCESSES_CACHE)
                check_wan_dhcp_client_v6=$(grep "[d]ibbler-client" $PROCESSES_CACHE)
            else
                if [ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "TG4482A" ] || [ "$SELFHEAL_TYPE" = "BASE" -a "$BOX_TYPE" = "XB3" ]; then
                    dhcp_cli_output=$(grep -E 'ti_.*erouter0' $PROCESSES_CACHE)
                    if [ "$MAPT_CONFIG" != "set" ]; then
                    if [ "$UDHCPC_Enable" = "true" ]; then
                        check_wan_dhcp_client_v4=$(grep -E 'sbin/udhcpc.*erouter' $PROCESSES_CACHE)
                    else
                        check_wan_dhcp_client_v4=$(echo "$dhcp_cli_output" | grep "ti_udhcpc")
                    fi
                    fi
                    dibbler_client_enable=$(get_from_syscfg_cache dibbler_client_enable)
                    if [ "$dibbler_client_enable" = "true" ]; then
                        check_wan_dhcp_client_v6=$(grep "[d]ibbler-client" $PROCESSES_CACHE)
                    else
                        check_wan_dhcp_client_v6=$(echo "$dhcp_cli_output" | grep "ti_dhcp6c")
                    fi
                else
                    if [ "$FIRMWARE_TYPE" = "OFW" ]; then
                        check_wan_dhcp_client_v4=$(grep -E 'udhcpc.*erouter' $PROCESSES_CACHE)
                        check_wan_dhcp_client_v6=$(grep "[d]ibbler-client" $PROCESSES_CACHE)
                    else
                        dhcp_cli_output=$(cgrep -E 'ti_.*erouter0' $PROCESSES_CACHE)
                        check_wan_dhcp_client_v4=$(echo "$dhcp_cli_output" | grep "ti_udhcpc")
                        check_wan_dhcp_client_v6=$(echo "$dhcp_cli_output" | grep "ti_dhcp6c")
                    fi
                fi
            fi
        ;;
        "TCCBR")
            check_wan_dhcp_client_v4=$(grep -E 'udhcpc.*erouter' $PROCESSES_CACHE)
            check_wan_dhcp_client_v6=$(grep "[d]ibbler-client" $PROCESSES_CACHE)
        ;;
    esac

    case $SELFHEAL_TYPE in
        "BASE")
            if [ "$BOX_TYPE" = "XB3" ] || [ "$BOX_TYPE" = "MV1" ]; then

                if [ "$check_wan_dhcp_client_v4" != "" ] && [ "$check_wan_dhcp_client_v6" != "" ]; then
                    if [ "$(cat /proc/net/dbrctl/mode)"  = "standbay" ]; then
                        echo_t "RDKB_SELFHEAL : dbrctl mode is standbay, changing mode to registered"
                        echo "registered" > /proc/net/dbrctl/mode
                    fi
                fi
            fi

        ;;
        "TCCBR")
        ;;
        "SYSTEMD")
        ;;
    esac

    if [ "$(sysevent get wan-status)" = "started" ] && [ -n "$(ifconfig $WAN_INTERFACE | grep 'UP')" ]
    then
        LAST_EROUTER_MODE=$(get_from_syscfg_cache last_erouter_mode)
        if [ "$LAST_EROUTER_MODE" != "2" ] ; then
            WAN_IPv4_Addr=$(ifconfig $WAN_INTERFACE | grep "inet" | grep -v "inet6")
            if [ "$WAN_IPv4_Addr" = "" ] ; then
                echo_t "erouter ipv4 address is empty"
                t2CountNotify "RF_ERROR_erouter_ipv4_loss"
                sysevent set dhcp_client-restart
            fi
        fi
        if [ "$LAST_EROUTER_MODE" != "1" ] ; then
            WAN_IPv6_Addr=$(ifconfig $WAN_INTERFACE | grep "inet6" | grep "Scope:Global")
            if [ "$WAN_IPv6_Addr" = "" ] ; then
                echo_t "erouter ipv6 address is empty"
                t2CountNotify "RF_ERROR_erouter_ipv6_loss"
                t2CountNotify "SYS_SH_ti_dhcp6c_restart"
                killall dibbler-client
                /lib/rdk/dibbler-init.sh
                /usr/sbin/dibbler-client start
            fi
        fi
    fi

    #Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    if [ "x$check_wan_dhcp_client_v4" = "x" ] && [ "x$LAST_EROUTER_MODE" != "x2" ] && [ "$MAPT_CONFIG" != "set" ]; then
          echo_t "RDKB_PROCESS_CRASHED : DHCP Client for v4 is not running, need restart "
          t2CountNotify "SYS_ERROR_DHCPV4Client_notrunning"
	  wan_dhcp_client_v4=0
    fi

    if [ "$thisWAN_TYPE" != "EPON" ]; then
                    
        #Intel Proposed RDKB Generic Bug Fix from XB6 SDK
	if [ "x$check_wan_dhcp_client_v6" = "x" ] && [ "x$LAST_EROUTER_MODE" != "x1" ] && [ "$Unit_Activated" != "0" ]; then
        echo_t "RDKB_PROCESS_CRASHED : DHCP Client for v6 is not running, need restart"
        t2CountNotify "SYS_ERROR_DHCPV6Client_notrunning"
		wan_dhcp_client_v6=0
	fi

        DHCP_STATUS=$(dmcli eRT retv Device.DHCPv4.Client.1.DHCPStatus)

        if [ "$DHCP_STATUS" != "" ] && [ "$DHCP_STATUS" != "Bound" ] ; then

            echo_t "DHCP_CLIENT : DHCPStatusValue is $DHCP_STATUS"
            if ([ $wan_dhcp_client_v4 -eq 0 ] && [ "$MAPT_CONFIG" != "set" ]) || [ $wan_dhcp_client_v6 -eq 0 ]; then
                case $SELFHEAL_TYPE in
                    "BASE"|"TCCBR")
                        echo_t "DHCP_CLIENT : DHCPStatus is not Bound, restarting WAN"
                    ;;
                    "SYSTEMD")
                        echo_t "DHCP_CLIENT : DHCPStatus is $DHCP_STATUS, restarting WAN"
                    ;;
                esac
                sh /etc/utopia/service.d/service_wan.sh wan-stop
                sh /etc/utopia/service.d/service_wan.sh wan-start
                wan_dhcp_client_v4=1
                wan_dhcp_client_v6=1
            fi
        fi
    fi

    case $SELFHEAL_TYPE in
        "BASE")
            if [ $wan_dhcp_client_v4 -eq 0 ] && [ "$MAPT_CONFIG" != "set" ]; then
                if [ "$SOC_TYPE" = "Broadcom" ] && [ "$BOX_TYPE" != "XB3" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
                    V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
                elif [ "$WAN_TYPE" = "EPON" ]; then
                    echo "Calling epon_utility.sh to restart udhcpc "
                    sh /usr/ccsp/epon_utility.sh
                else
                    if [ "$BOX_TYPE" = "XB6" -a "$MANUFACTURE" = "Arris" ] || [ "$BOX_TYPE" = "XB3" ]; then

                        if [ "$UDHCPC_Enable" = "true" ]; then
                            V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /usr/bin/service_udhcpc"
                        else
                            DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
                            V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 1"
                        fi
                    else
			if [ "$FIRMWARE_TYPE" = "OFW" ]; then
                            sysevent set dhcp_client-restart
                        else
                            DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
                            V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 1"
                        fi
                    fi
                fi
                echo_t "DHCP_CLIENT : Restarting DHCP Client for v4"
                eval "$V4_EXEC_CMD"
                sleep 5
                wan_dhcp_client_v4=1
            fi

            if [ $wan_dhcp_client_v6 -eq 0 ]; then
                echo_t "DHCP_CLIENT : Restarting DHCP Client for v6"
                if [ "$SOC_TYPE" = "Broadcom" ] && [ "$BOX_TYPE" != "XB3" ]; then
                    /lib/rdk/dibbler-init.sh
                    sleep 2
                    /usr/sbin/dibbler-client start
                elif [ "$BOX_TYPE" = "MV1" ]; then
                    /lib/rdk/dibbler-init.sh
                    sleep 2
                    /usr/sbin/dibbler-client start
                elif [ "$WAN_TYPE" = "EPON" ]; then
                    echo "Calling dibbler_starter.sh to restart dibbler-client "
                    sh /usr/ccsp/dibbler_starter.sh
                else
		    Dhcpv6_Client_restart "ti_dhcp6c" "Idle"
                fi
                wan_dhcp_client_v6=1
            fi
        ;;
        "TCCBR")
            if [ $wan_dhcp_client_v4 -eq 0 ] && [ "$MAPT_CONFIG" != "set" ]; then
                V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
                echo_t "DHCP_CLIENT : Restarting DHCP Client for v4"
                eval "$V4_EXEC_CMD"
                sleep 5
                wan_dhcp_client_v4=1
            fi

            if [ $wan_dhcp_client_v6 -eq 0 ]; then
                echo_t "DHCP_CLIENT : Restarting DHCP Client for v6"
                /lib/rdk/dibbler-init.sh
                sleep 2
                /usr/sbin/dibbler-client start
                wan_dhcp_client_v6=1
            fi
        ;;
        "SYSTEMD")
        ;;
    esac

fi # [ "$WAN_STATUS" = "started" ]

case $SELFHEAL_TYPE in
    "BASE")
        # Test to make sure that if mesh is enabled the backhaul tunnels are attached to the bridges
        MESH_ENABLE=$(dmcli eRT retv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Enable)
        if [ "$MESH_ENABLE" = "true" ]; then
            echo_t "[RDKB_SELFHEAL] : Mesh is enabled, test if tunnels are attached to bridges"
            t2CountNotify  "WIFI_INFO_mesh_enabled"

            # Fetch mesh tunnels from the brlan0 bridge if they exist
            if [ "x$ovs_enable" = "xtrue" ];then
            	brctl0_ifaces=$(ovs-vsctl list-ifaces brlan0 | egrep "pgd")
            else
            	brctl0_ifaces=$(brctl show brlan0 | egrep "pgd")
            fi
            br0_ifaces=$(ifconfig | egrep "^pgd" | egrep "\.100" | awk '{print $1}')

            for ifn in $br0_ifaces
              do
                brFound="false"

                for br in $brctl0_ifaces
                  do
                    if [ "$br" = "$ifn" ]; then
                        brFound="true"
                    fi
                  done
                if [ "$brFound" = "false" ]; then
                    echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan0"
                    if [ "x$bridgeUtilEnable" = "xtrue" || "x$ovs_enable" = "xtrue" ];then
                    	/usr/bin/bridgeUtils add-port brlan0 $ifn
                    else
                    	brctl addif brlan0 $ifn;
                    fi
                fi
              done

            # Fetch mesh tunnels from the brlan1 bridge if they exist
            if [ "$thisIS_BCI" != "yes" ] && [ "$FIRMWARE_TYPE" != "OFW" ]; then
                if [ "x$ovs_enable" = "xtrue" ];then
                    brctl1_ifaces=$(ovs-vsctl list-ifaces brlan1 | egrep "pgd")
                else
                    brctl1_ifaces=$(brctl show brlan1 | egrep "pgd")
                fi
                br1_ifaces=$(ifconfig | egrep "^pgd" | egrep "\.101" | awk '{print $1}')

                for ifn in $br1_ifaces
                  do
                    brFound="false"

                    for br in $brctl1_ifaces
                      do
                        if [ "$br" = "$ifn" ]; then
                            brFound="true"
                        fi
                      done
                    if [ "$brFound" = "false" ]; then
                        echo_t "[RDKB_SELFHEAL] : Mesh bridge $ifn missing, adding iface to brlan1"
                        if [ "x$bridgeUtilEnable" = "xtrue" || "x$ovs_enable" = "xtrue" ];then
                        	/usr/bin/bridgeUtils add-port brlan1 $ifn
                        else
                        	brctl addif brlan1 $ifn;
                        fi
                    fi
                  done
            fi
        fi
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        if [ "x$MAPT_CONFIG" != "xset" ] && [ "$BOX_TYPE" != "HUB4" ] && [ "$BOX_TYPE" != "SR300" ] && [ "$BOX_TYPE" != "SE501" ] && [ "$BOX_TYPE" != "SR213" ] && [ "$BOX_TYPE" != "WNXL11BWL" ]; then
            if [ $wan_dhcp_client_v4 -eq 0 ]; then
                if [ "$SOC_TYPE" = "Broadcom" ]; then
                    V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
                elif [ "$WAN_TYPE" = "EPON" ]; then
                    echo "Calling epon_utility.sh to restart udhcpc "
                    sh /usr/ccsp/epon_utility.sh
                else
                    if [ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "TG4482A" ]; then

                        if [ "$UDHCPC_Enable" = "true" ]; then
                            V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /usr/bin/service_udhcpc"
                        else
                            #For AXB6 b -4 option is added to avoid timeout.
                            DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
                            V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 4"
                        fi
                    else
 
                        DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
                        V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 1"
                    fi
                fi

                echo_t "DHCP_CLIENT : Restarting DHCP Client for v4"
                eval "$V4_EXEC_CMD"
                sleep 5
                wan_dhcp_client_v4=1
            fi

            #ARRISXB6-8319
            #check if interface is down or default route is missing.
            if ([ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "TG4482A" ]) && [ "$LAST_EROUTER_MODE" != "2" ] && [ "$MAPT_CONFIG" != "set" ]; then
                ip route show default | grep "default"
                if [ $? -ne 0 ] ; then
                    ifconfig $WAN_INTERFACE up
                    sleep 2



                    if [ "$UDHCPC_Enable" = "true" ]; then
                        echo_t "restart udhcp"
                        DHCPC_PID_FILE="/tmp/udhcpc.erouter0.pid"
                    else
                        echo_t "restart ti_udhcp"
                        DHCPC_PID_FILE="/var/run/eRT_ti_udhcpc.pid"
                    fi


                    if [ -f $DHCPC_PID_FILE ]; then
                        echo_t "SERVICE_DHCP : Killing $(cat $DHCPC_PID_FILE)"
                        kill -9 "$(cat $DHCPC_PID_FILE)"
                        rm -f $DHCPC_PID_FILE
                    fi


                    if [ "$UDHCPC_Enable" = "true" ]; then
                        V4_EXEC_CMD="/sbin/udhcpc -i erouter0 -p /tmp/udhcpc.erouter0.pid -s /etc/udhcpc.script"
                    else
                        #For AXB6 b -4 option is added to avoid timeout.
                        V4_EXEC_CMD="ti_udhcpc -plugin /lib/libert_dhcpv4_plugin.so -i $WAN_INTERFACE -H DocsisGateway -p $DHCPC_PID_FILE -B -b 4"
                    fi


                    echo_t "DHCP_CLIENT : Restarting DHCP Client for v4"
                    eval "$V4_EXEC_CMD"
                    sleep 5
                    wan_dhcp_client_v4=1
                fi
            fi

            if [ $wan_dhcp_client_v6 -eq 0 ]; then
                echo_t "DHCP_CLIENT : Restarting DHCP Client for v6"
                if [ "$SOC_TYPE" = "Broadcom" ] && [ "$BOX_TYPE" != "XB3" ]; then
                    /lib/rdk/dibbler-init.sh
                    sleep 2
                    /usr/sbin/dibbler-client start
                elif [ "$WAN_TYPE" = "EPON" ]; then
                    echo "Calling dibbler_starter.sh to restart dibbler-client "
                    sh /usr/ccsp/dibbler_starter.sh
                else
		    Dhcpv6_Client_restart "ti_dhcp6c" "Idle"
                fi
                wan_dhcp_client_v6=1
            fi
        fi #Not HUB4 && SR300 && SE501 && SR213 && WNXL11BWL
    ;;
esac
fi

# ARRIS XB6 => MODEL_NUM=TG3482G
# Tech CBR  => MODEL_NUM=CGA4131COM
# Tech xb6  => MODEL_NUM=CGM4140COM
# Tech XB7  => MODEL_NUM=CGM4331COM
# Tech CBR2 => MODEL_NUM=CGA4332COM
# CMX  XB7  => MODEL_NUM=TG4482A
# Tech XB8  => MODEL_NUM=CGM4981COM
# This critical processes checking is handled in selfheal_aggressive.sh for above platforms
# Ref: RDKB-25546
if [ "$MODEL_NUM" != "TG3482G" ] && [ "$MODEL_NUM" != "CGA4131COM" ] &&
       [ "$MODEL_NUM" != "CGM4140COM" ] && [ "$MODEL_NUM" != "CGM4331COM" ] && [ "$MODEL_NUM" != "CGM4981COM" ] &&  [ "$MODEL_NUM" != "TG4482A" ] && [ "$MODEL_NUM" != "CGA4332COM" ] 
then
case $SELFHEAL_TYPE in
    "BASE")
    ;;
    "TCCBR")
    ;;
    "SYSTEMD")
        if [ "$MULTI_CORE" = "yes" ]; then
            if [ -f $PING_PATH/ping_peer ]; then
                ## Check Peer ip is accessible
                loop=1
                while [ $loop -le 3 ]
                  do
                    PING_RES=$(ping_peer)
                    CHECK_PING_RES=$(echo "$PING_RES" | grep "packet loss" | cut -d"," -f3 | cut -d"%" -f1)

                    if [ "$CHECK_PING_RES" != "" ]; then
                        if [ $CHECK_PING_RES -ne 100 ]; then
                            ping_success=1
                            echo_t "RDKB_SELFHEAL : Ping to Peer IP is success"
                            break
                        else
                            ping_failed=1
                        fi
                    else
                        ping_failed=1
                    fi

                    if [ $ping_failed -eq 1 ] && [ $loop -lt 3 ]; then
                        echo_t "RDKB_SELFHEAL : Ping to Peer IP failed in iteration $loop"
                        t2CountNotify "SYS_SH_pingPeerIP_Failed"
                    else
                        echo_t "RDKB_SELFHEAL : Ping to Peer IP failed after iteration $loop also ,rebooting the device"
                        t2CountNotify "SYS_SH_pingPeerIP_Failed"
                        echo_t "RDKB_REBOOT : Peer is not up ,Rebooting device "
                        echo_t "Setting Last reboot reason Peer_down"
                        reason="Peer_down"
                        rebootCount=1
                        rebootNeeded RM "" $reason $rebootCount

                    fi
                    loop=$((loop+1))
                    sleep 5
                  done
            else
                echo_t "RDKB_SELFHEAL : ping_peer command not found"
            fi

            if [ -f $PING_PATH/arping_peer ]; then
                $PING_PATH/arping_peer
            else
                echo_t "RDKB_SELFHEAL : arping_peer command not found"
            fi
        fi
    ;;
esac
fi

if [ $rebootDeviceNeeded -eq 1 ]; then

    inMaintWindow=0
    doMaintReboot=1
    case $SELFHEAL_TYPE in
        "BASE"|"SYSTEMD")
            inMaintWindow=1
        ;;
        "TCCBR")
            inMaintWindow=1
        ;;
    esac
    if [ $inMaintWindow -eq 1 ]; then
            #Check if we have already flagged reboot is needed
            if [ ! -e $FLAG_REBOOT ]; then
                if [ "$SELFHEAL_TYPE" = "BASE" ] && [ $reboot_needed_atom_ro -eq 1 ]; then
                    echo_t "RDKB_REBOOT : atom is read only, rebooting the device."
                    dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string atom_read_only
                    sh /etc/calc_random_time_to_reboot_dev.sh "ATOM_RO" &
                elif [ "$SELFHEAL_TYPE" = "BASE" -o "$SELFHEAL_TYPE" = "SYSTEMD" ] && [ "$thisIS_BCI" != "yes" ] && [ $rebootNeededforbrlan1 -eq 1 ]; then
                    echo_t "rebootNeededforbrlan1"
                    echo_t "RDKB_REBOOT : brlan1 interface is not up, rebooting the device."
                    echo_t "Setting Last reboot reason"
                    dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootReason string brlan1_down
                    case $SELFHEAL_TYPE in
                        "BASE")
                            dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_LastRebootCounter int 1  #TBD: not in original DEVICE code
                        ;;
                        "TCCBR")
                        ;;
                        "SYSTEMD")
                        ;;
                    esac
                    echo_t "SET succeeded"
                    sh /etc/calc_random_time_to_reboot_dev.sh "" &
                else
                    echo_t "rebootDeviceNeeded"
                    sh /etc/calc_random_time_to_reboot_dev.sh "" &
                fi
                touch $FLAG_REBOOT
            else
                echo_t "Already waiting for reboot"
            fi
    fi  # [ $inMaintWindow -eq 1 ]
fi  # [ $rebootDeviceNeeded -eq 1 ]

isPeriodicFWCheckEnable=$(syscfg get PeriodicFWCheck_Enable)
if [ "$isPeriodicFWCheckEnable" = "false" ] || [ "$isPeriodicFWCheckEnable" = "" ]; then
    #check firmware download script is running.
    case $SELFHEAL_TYPE in
        "BASE")
            if [ "$BOX_TYPE" = "XB3" ]; then
                firmDwnldPid=$(ps w | grep -w "xb3_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
                if [ "$firmDwnldPid" = "" ]; then
                    echo_t "Restarting XB3 firmwareDwnld script"
                    exec  /etc/xb3_firmwareDwnld.sh &
                fi
            fi
        ;;
        "TCCBR")
            if [ "$BOX_TYPE" = "TCCBR" ]; then
                fDwnldPid=$(ps w | grep -w "cbr_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
                if [ "$fDwnldPid" = "" ]; then
                    echo_t "Restarting CBR firmwareDwnld script"
                    exec  /etc/cbr_firmwareDwnld.sh &
                fi
            fi
        ;;
        "SYSTEMD")
            if [ "$WAN_TYPE" = "EPON" ]; then
                fDwnldPid=$(ps w | grep -w "xf3_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            elif [ "$BOX_TYPE" = "HUB4" ]; then
                fDwnldPid=$(ps w | grep -w "hub4_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            elif [ "$BOX_TYPE" = "SR300" ]; then
                fDwnldPid=$(ps w | grep -w "sr300_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            elif [ "$BOX_TYPE" = "SE501" ]; then
                fDwnldPid=$(ps w | grep -w "se501_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            elif [ "$BOX_TYPE" = "WNXL11BWL" ]; then
                fDwnldPid=$(ps w | grep -w "wnxl11bwl_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            else
                fDwnldPid=$(ps w | grep -w "xb6_firmwareDwnld.sh" | grep -v "grep" | awk '{print $1}')
            fi

            if [ "$fDwnldPid" = "" ]; then
                echo_t "Restarting firmwareDwnld script"
                systemctl stop CcspXconf.service
                systemctl start CcspXconf.service
            fi
        ;;
    esac
fi

# Checking telemetry2_0 health and recovery
T2_0_BIN="/usr/bin/telemetry2_0"
T2_0_APP="telemetry2_0"
T2_ENABLE=$(get_from_syscfg_cache T2Enable)
if [ ! -f $T2_0_BIN ]; then
    T2_ENABLE="false"
fi
echo_t "Telemetry 2.0 feature is $T2_ENABLE"
if [ "$T2_ENABLE" = "true" ]; then
    T2_PID=$(get_pid $T2_0_APP)
    if [ "$T2_PID" = "" ]; then
        echo_t "RDKB_PROCESS_CRASHED : $T2_0_APP is not running, need restart"
        /usr/bin/telemetry2_0 &
    fi
fi

# Checking D process running or not
case $SELFHEAL_TYPE in
      "BASE"|"SYSTEMD"|"TCCBR")
      check_D_process=$(grep " [D]W " $PROCESSES_CACHE)
      if [ "$check_D_process" = "" ]; then
           echo_t "[RDKB_SELFHEAL] : There is no D process running in this device"
      else
           echo_t "[RDKB_SELFHEAL] : D process is running in this device"
      fi
      ;;
esac

#BWGRDK-1044 conntrack Flush monitoring
if [ "$MODEL_NUM" = "DPC3939B" ] || [ "$MODEL_NUM" = "DPC3941B" ]; then
    CONSOLE_LOGFILE=/rdklogs/logs/ArmConsolelog.txt.0;
    timestamp=$(date '+%d/%m/%Y %T')
    check_conntrack_D=`ps -w | grep -i "conntrack" | grep " D " | grep -v grep | wc -l`
    if [ $check_conntrack_D -gt 0 ]; then
        echo "$timestamp : Conntrack Log start " >> $CONSOLE_LOGFILE
        dmesg | grep conntrack >> $CONSOLE_LOGFILE
        echo "$timestamp : Conntrack Log End " >> $CONSOLE_LOGFILE
        cli system/pp/enable
    fi
fi

if [ "$MODEL_NUM" = "CGM4981COM" ] || [ "$MODEL_NUM" = "CGM4331COM" ]; then
        MESH_ENABLE=$(fast_dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Enable | sed -ne 's/[[:blank:]]*$//; s/.*value: //p')
        OPENSYNC_ENABLE=$(fast_dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Opensync | sed -ne 's/[[:blank:]]*$//; s/.*value: //p')
        if [ "$MESH_ENABLE" = "true" ] && [ "$OPENSYNC_ENABLE" = "true" ]; then
                echo_t "[RDKB_SELFHEAL] : Mesh is enabled, test if vlan tag is NULL "
                vlantag_wl0=$( /usr/opensync/tools/ovsh s Port -w name==wl0 | egrep "tag" | egrep 100)
                vlantag_wl1=$( /usr/opensync/tools/ovsh s Port -w name==wl1 | egrep "tag" | egrep 100)
                if [[ ! -z "$vlantag_wl0" ]] || [[ ! -z "$vlantag_wl1" ]]; then
                        echo_t "[RDKB_SELFHEAL] : Remove port vlan tag "
                        ovs-vsctl remove port wl0 tag 100
                        ovs-vsctl remove port wl1 tag 100
                fi
        fi
fi

# Run IGD process if not running
upnp_enabled=`get_from_syscfg_cache upnp_igd_enabled`
if [ "1" = "$upnp_enabled" ]; then
    IGD_PIDS=$(get_pid IGD)
    if [ "$IGD_PIDS" = "" ]; then
	    echo_t "[RDKB_SELFHEAL] : There is no IGD process running in this device"
	    sysevent set igd-restart
  fi
fi

if [ "$MODEL_NUM" = "TG3482G" ] || [ "$MODEL_NUM" = "CGM4140COM" ]; then
        MESH_ENABLE=$(dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Enable | grep "value" | cut -f3 -d":" | cut -f2 -d" ")
        OPENSYNC_ENABLE=$(dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.Mesh.Opensync | grep "value" | cut -f3 -d":" | cut -f2 -d" ")
        if [ "$MESH_ENABLE" = "true" ] && [ "$OPENSYNC_ENABLE" = "true" ]; then
                echo_t "[RDKB_SELFHEAL] : Mesh is enabled, test if vlan tag is NULL "
                vlantag_ath0=$( /usr/opensync/tools/ovsh s Port -w name==ath0 | egrep "tag" | egrep 100)
                vlantag_ath1=$( /usr/opensync/tools/ovsh s Port -w name==ath1 | egrep "tag" | egrep 100)
                if [[ ! -z "$vlantag_ath0" ]] || [[ ! -z "$vlantag_ath1" ]]; then
                        echo_t "[RDKB_SELFHEAL] : Remove port vlan tag "
                        ovs-vsctl remove port ath0 tag 100
                        ovs-vsctl remove port ath1 tag 100
                fi
        fi
fi

# ----------------------------------------------------------------------------
done
# ----------------------------------------------------------------------------
