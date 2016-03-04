#!/bin/sh

source /fss/gw/usr/ccsp/tad/corrective_action.sh
max_count=12

	timestamp=`getDate`

	totalMemSys=`free | awk 'FNR == 2 {print $2}'`
	usedMemSys=`free | awk 'FNR == 2 {print $3}'`
	freeMemSys=`free | awk 'FNR == 2 {print $4}'`

	echo "RDKB_SYS_MEM_INFO_SYS : Total memory in system is $totalMemSys at timestamp $timestamp"
	echo "RDKB_SYS_MEM_INFO_SYS : Used memory in system is $usedMemSys at timestamp $timestamp"
	echo "RDKB_SYS_MEM_INFO_SYS : Free memory in system is $freeMemSys at timestamp $timestamp"

	LOAD_AVG=`cat /proc/loadavg`

#Record the start statistics

	STARTSTAT=$(getstat)

	sleep $DELAY

#Record the end statistics
	ENDSTAT=$(getstat)

	USR=$(change 1)
	SYS=$(change 3)
	IDLE=$(change 4)
	IOW=$(change 5)


	ACTIVE=$(( $USR + $SYS + $IOW ))

	TOTAL=$(($ACTIVE + $IDLE))

	Curr_CPULoad=$(( $ACTIVE * 100 / $TOTAL ))

	echo "RDKB_LOAD_AVERAGE : Load Average is $LOAD_AVG at timestamp $timestamp"

	echo "RDKB_CPU_USAGE : CPU usage is $Curr_CPULoad at timestamp $timestamp"
        CPU_INFO=`mpstat | tail -1` 
	echo "RDKB_CPUINFO : Cpu Info is $CPU_INFO "


	count=`syscfg get process_memory_log_count`
	count=$((count + 1))
	echo "Count is $count"

	if [ "$count" -eq "$max_count" ]
	then
		echo "RDKB_PROC_MEM_LOG: Process Memory log at $timestamp is"
		echo ""
		top -m -b n 1
		syscfg set process_memory_log_count 0	
		syscfg commit
	
	else
		echo "RDKB_PROC_MEM_LOG: Ccsp Process Memory log at $timestamp is"
		echo ""
		top_cmd="top -m -b n 1 | grep -e Mem -e Swap -e PID -e CcspCrSsp -e PsmSsp -e CcspPandMSsp -e CcspWecbController -e CcspHomeSecurity -e CcspMtaAgentSsp -e CcspCMAgentSsp -e CcspTr069PaSsp -e CcspTandDSsp -e CcspLMLite -e webpa"
		eval $top_cmd
		syscfg set process_memory_log_count $count	
		syscfg commit
	fi

	echo "================================================================================"
	echo ""
	echo "RDKB_DISK_USAGE: Systems Disk Space Usage log at $timestamp is"
	echo ""
	disk_usage="df"
	eval $disk_usage
	count=$((count + 1))
