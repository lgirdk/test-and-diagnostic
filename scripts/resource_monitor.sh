#!/bin/sh

source /fss/gw/etc/utopia/service.d/log_env_var.sh
source /fss/gw/usr/ccsp/tad/corrective_action.sh
source /fss/gw/usr/ccsp/tad/task_health_monitor.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1

DELAY=30
threshold_reached=0
SELFHEAL_ENABLE=`syscfg get selfheal_enable`

while [ $SELFHEAL_ENABLE = "true" ]
do
	RESOURCE_MONITOR_INTERVAL=`syscfg get resource_monitor_interval`
	if [ "$RESOURCE_MONITOR_INTERVAL" = "" ]
	then
		RESOURCE_MONITOR_INTERVAL=15
	fi 
	RESOURCE_MONITOR_INTERVAL=$(($RESOURCE_MONITOR_INTERVAL*60))

	sleep $RESOURCE_MONITOR_INTERVAL
	
	totalMemSys=`free | awk 'FNR == 2 {print $2}'`
	usedMemSys=`free | awk 'FNR == 2 {print $3}'`
	freeMemSys=`free | awk 'FNR == 2 {print $4}'`

	timestamp=`getDate`

	# Memory info reading using free linux utility

	AvgMemUsed=$(( ( $usedMemSys * 100 ) / $totalMemSys ))

	MEM_THRESHOLD=`syscfg get avg_memory_threshold`

	if [ "$AvgMemUsed" -ge "$MEM_THRESHOLD" ]
	then

		echo "RDKB_SELFHEAL : Total memory in system is $totalMemSys at timestamp $timestamp"
		echo "RDKB_SELFHEAL : Used memory in system is $usedMemSys at timestamp $timestamp"
		echo "RDKB_SELFHEAL : Free memory in system is $freeMemSys at timestamp $timestamp"
		echo "RDKB_SELFHEAL : AvgMemUsed in % is  $AvgMemUsed"
		vendor=`getVendorName`
		modelName=`getModelName`
		CMMac=`getCMMac`
		timestamp=`getDate`

		echo "<$level>CABLEMODEM[$vendor]:<99000006><$timestamp><$CMMac><$modelName> RM Memory threshold reached"
		
		threshold_reached=1
		rebootNeeded RM MEM
	fi
	# Avg CPU usage reading from /proc/stat
#	AvgCpuUsed=`grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage }'`
#	AvgCpuUsed=`echo $AvgCpuUsed | cut -d "." -f1`
#	IdleCpuVal=`top -bn1  | head -n10 | grep "CPU:" | cut -c 34-35`

#	LOAD_AVG=`cat /proc/loadavg`
#	echo "RDKB_LOAD_AVERAGE : Load Average is $LOAD_AVG"

#	AvgCpuUsed=$((100 - $IdleCpuVal))
#	echo "RDKB_CPU_USAGE : CPU usage is $AvgCpuUsed"

#Record the start statistics

	STARTSTAT=$(getstat)
	
	user_ini=`echo $STARTSTAT | cut -d 'x' -f 1`
	system_ini=`echo $STARTSTAT | cut -d 'x' -f 3`
	idle_ini=`echo $STARTSTAT | cut -d 'x' -f 4`
	iowait_ini=`echo $STARTSTAT | cut -d 'x' -f 5`

#echo "RDKB_SELFHEAL : Initial CPU stats are"
#echo "user_ini: $user_ini system_ini: $system_ini idle_ini=$idle_ini iowait_ini=$iowait_ini"
	sleep $DELAY

#Record the end statistics
	ENDSTAT=$(getstat)

	user_end=`echo $ENDSTAT | cut -d 'x' -f 1`
	system_end=`echo $ENDSTAT | cut -d 'x' -f 3`
	idle_end=`echo $ENDSTAT | cut -d 'x' -f 4`
	iowait_end=`echo $ENDSTAT | cut -d 'x' -f 5`

#echo "RDKB_SELFHEAL : CPU stats after $DELAY sec are"
#echo "user_end: $user_end system_end: $system_end idle_end=$idle_end iowait_end=$iowait_end"
	
	user_diff=$(change 1)
	system_diff=$(change 3)
	idle_diff=$(change 4)
	iowait_diff=$(change 5)

#echo "RDKB_SELFHEAL : CPU stats diff btw 2 intervals is"
#echo "user_diff= $user_diff system_diff=$system_diff and idle_diff=$idle_diff iowait_diff=$iowait_diff"

	active=$(( $user_diff + $system_diff + $iowait_diff))
	total=$(($active + $idle_diff))
	Curr_CPULoad=$(( $active * 100 / $total ))

	echo "RDKB_SELFHEAL : CPU usage is $Curr_CPULoad at timestamp $timestamp"
	CPU_THRESHOLD=`syscfg get avg_cpu_threshold`

	count_val=0
	if [ "$Curr_CPULoad" -ge "$CPU_THRESHOLD" ]
	then

		echo "RDKB_SELFHEAL : Monitoring CPU Load in a 5 minutes window"
	        Curr_CPULoad=0
		# Calculating CPU avg in 5 mins window		
		while [ "$count_val" -lt 10 ]
		do

			count_val=$(($count_val + 1))

			#Record the start statistics
			STARTSTAT=$(getstat)
	
			user_ini=`echo $STARTSTAT | cut -d 'x' -f 1`
			system_ini=`echo $STARTSTAT | cut -d 'x' -f 3`
			idle_ini=`echo $STARTSTAT | cut -d 'x' -f 4`
			iowait_ini=`echo $STARTSTAT | cut -d 'x' -f 5`

			echo "RDKB_SELFHEAL : Initial CPU stats are"
			echo "user_ini: $user_ini system_ini: $system_ini idle_ini=$idle_ini iowait_ini=$iowait_ini"

			sleep $DELAY

			#Record the end statistics
			ENDSTAT=$(getstat)

			user_end=`echo $ENDSTAT | cut -d 'x' -f 1`
			system_end=`echo $ENDSTAT | cut -d 'x' -f 3`
			idle_end=`echo $ENDSTAT | cut -d 'x' -f 4`
			iowait_end=`echo $ENDSTAT | cut -d 'x' -f 5`

			echo "RDKB_SELFHEAL : CPU stats after $DELAY sec are"
			echo "user_end: $user_end system_end: $system_end idle_end=$idle_end iowait_end=$iowait_end"
	
			user_diff=$(change 1)
			system_diff=$(change 3)
			idle_diff=$(change 4)
			iowait_diff=$(change 5)

			echo "RDKB_SELFHEAL : CPU stats diff btw 2 intervals is"
			echo "user_diff= $user_diff system_diff=$system_diff and idle_diff=$idle_diff iowait_diff=$iowait_diff"

			active=$(( $user_diff + $system_diff + $iowait_diff))
			total=$(($active + $idle_diff))
			Curr_CPULoad_calc=$(( $active * 100 / $total ))
			echo "RDKB_SELFHEAL : CPU load is $Curr_CPULoad_calc in iteration $count_val"
			Curr_CPULoad=$(($Curr_CPULoad + $Curr_CPULoad_calc))
			
		done

		Curr_CPULoad_Avg=$(( $Curr_CPULoad / 10 ))

		echo "RDKB_SELFHEAL : Avg CPU usage after 5 minutes of CPU Avg monitor window is $Curr_CPULoad_Avg"

		LOAD_AVG=`cat /proc/loadavg`
		echo "RDKB_SELFHEAL : LOAD_AVG is : $LOAD_AVG"
#		if [ "$Curr_CPULoad_Avg" -ge "$CPU_THRESHOLD" ]
#		then
#			vendor=`getVendorName`
#			modelName=`getModelName`
#			CMMac=`getCMMac`
#			timestamp=`getDate`
#
#			echo "<$level>CABLEMODEM[$vendor]:<99000005><$timestamp><$CMMac><$modelName> RM CPU threshold reached"
#		
#			threshold_reached=1
#			rebootNeeded RM CPU
#		fi
	fi

	sh /fss/gw/usr/ccsp/tad/task_health_monitor.sh

	SELFHEAL_ENABLE=`syscfg get selfheal_enable`
		
done
