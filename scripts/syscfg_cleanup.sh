#!/bin/sh
source /etc/device.properties
source /etc/utopia/service.d/log_capture_path.sh

CRONTAB_DIR="/var/spool/cron/crontabs/"
CRON_FILE_BK="/tmp/crontab.txt"
SCRIPT_NAME="syscfg_cleanup"

if [ -f "/nvram/syscfg_clean" ]; then
    echo "Syscfg cleanup already done" 
    echo "Remove /nvram/syscfg_clean file to cleanup again"
    exit 1
fi

UPTIME=`cat /proc/uptime  | awk '{print $1}' | awk -F '.' '{print $1}'`
if [ "$UPTIME" -lt 1800 ]; then
    echo "Uptime is less than 30 mins, exiting the syscfg_cleanup"
    exit 0
fi

#Removing erouter0 "_inst_num" dynamic enteries from database
erouter_inst_num=`cat /nvram/syscfg.db | grep tr_erouter0 |grep "_inst_num" | cut -d "=" -f1`
for entry in $erouter_inst_num
do
        echo "$entry"
        syscfg unset $entry
done

#Removing erouter0 "_alias" dynamic enteries from database
erouter_alias=`cat /nvram/syscfg.db | grep tr_erouter0 |grep "_alias" | cut -d "=" -f1`

for entry in $erouter_alias
do
        echo "$entry"
        syscfg unset $entry
done

#Removing brlan0 "_inst_num" dynamic enteries from database
brlan_inst_num=`cat /nvram/syscfg.db | grep tr_brlan0 |grep "_inst_num" | cut -d "=" -f1`

for entry in $brlan_inst_num
do
        echo "$entry"
        syscfg unset $entry
done

#Removing brlan0 "_alias" dynamic enteries from database
brlan_alias=`cat /nvram/syscfg.db | grep tr_brlan0 |grep "_alias" | cut -d "=" -f1`

for entry in $brlan_alias
do
        echo "$entry"
        syscfg unset $entry
done

syscfg commit

check_cleanup_erouter_inst_num=`cat /nvram/syscfg.db | grep tr_erouter0 |grep "_inst_num" `
check_cleanup_erouter_alias=`cat /nvram/syscfg.db | grep tr_erouter0 |grep "_alias" `
check_cleanup_brlan_inst_num=`cat /nvram/syscfg.db | grep tr_brlan0 |grep "_inst_num" `
check_cleanup_brlan_alias=`cat /nvram/syscfg.db | grep tr_brlan0 |grep "_alias" `

#Check that cleanup is successful or not
if [ "$check_cleanup_erouter_inst_num" = "" ] && [ "$check_cleanup_erouter_alias" = "" ] && [ "$check_cleanup_brlan_inst_num" = "" ] && [ "$check_cleanup_brlan_alias" = "" ] ;then
	echo "Database clean up success"
        touch /nvram/syscfg_clean
else
	echo "Database clean up failed"
fi

#Clean the job from crontab
crontab -l -c $CRONTAB_DIR > $CRON_FILE_BK
sed -i "/$SCRIPT_NAME/d" $CRON_FILE_BK
crontab $CRON_FILE_BK -c $CRONTAB_DIR
rm -rf $CRON_FILE_BK

echo "Running apply system defaults"
apply_system_defaults

if [ "$BOX_TYPE" = "XB3" ];then
	echo "XB3 device, restaring PandM"
	cd /usr/ccsp/pam/
	kill -9 `pidof CcspPandMSsp`
	/usr/bin/CcspPandMSsp -subsys eRT.

	isPeriodicFWCheckEnable=`syscfg get PeriodicFWCheck_Enable`
	PID_XCONF=`pidof xb3_firmwareDwnld.sh`
	if [ "$isPeriodicFWCheckEnable" == "false" ] && [ "$PID_XCONF" == "" ] ;then
	        echo "XCONF SCRIPT : Calling XCONF Client"
	        /etc/xb3_firmwareDwnld.sh &
	fi
elif [ "$BOX_TYPE" = "XB6" ];then
		echo "XB6 device, restaring PandM"

	systemctl restart CcspPandMSsp.service
fi
