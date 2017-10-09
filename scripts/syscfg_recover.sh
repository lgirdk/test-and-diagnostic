#! /bin/sh

source /etc/device.properties

TAD_PATH="/usr/ccsp/tad"
UTOPIA_PATH="/etc/utopia/service.d"
SYSCFG_SHM_FILE="/tmp/syscfg.shmid"
SYSCFG_MOUNT=/nvram
SYSCFG_FILE=$SYSCFG_MOUNT/syscfg.db

source $UTOPIA_PATH/log_env_var.sh
source /etc/log_timestamp.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

#check whether syscfg is proper or not
syscfg get redirection_flag
if [ $? == 0 ]; then
	exit
fi

#check whether syscfg is proper or not once again
syscfg get selfheal_enable
if [ $? != 0 ]; then
	echo_t "RDKB_SELFHEAL : syscfg queries are failing"
	echo_t "RDKB_SELFHEAL : Memory Dump"
	echo_t "==========================="
	free -m
	df -h
	du -h $SYSCFG_MOUNT
	echo_t "==========================="

	echo_t "RDKB_SELFHEAL : Recreating syscfg DB again..."

	if [ -f $SYSCFG_SHM_FILE ]; then 
		rm -rf $SYSCFG_SHM_FILE		
	fi

	#Re-create syscfg create again
	syscfg_create -f $SYSCFG_FILE
	if [ $? == 0 ]; then
	   echo_t "RDKB_SELFHEAL : syscfg DB functional now"

		SELFHEAL_ENABLE=`syscfg get selfheal_enable`
		if [ "$SELFHEAL_ENABLE" == "true" ]; then
			SelfHealScript_PID=`pidof self_heal_connectivity_test.sh`
			if [ "$SelfHealScript_PID" == "" ]; then
				echo_t "Restarting selfheal connectivity script"
				$TAD_PATH/self_heal_connectivity_test.sh &
			fi

			SelfHealScript_PID=`pidof resource_monitor.sh`
			if [ "$SelfHealScript_PID" == "" ]; then
				echo_t "Restarting resource monitor script"
				$TAD_PATH/resource_monitor.sh & 
			fi
		fi
	else
	   echo_t "RDKB_SELFHEAL : syscfg DB creation failed"		
	fi
fi
