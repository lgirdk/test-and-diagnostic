#!/bin/sh

source /etc/utopia/service.d/log_capture_path.sh

Min_Mem_Value=`syscfg get MinMemoryThreshold_Value`

free_mem=`free | awk 'FNR == 2 {print $4}'`
free_mem_in_mb=$(($free_mem/1024))
echo_t "RDKB_MEM_HEALTH : Min Memory Threshold Value is $Min_Mem_Value"
echo_t "RDKB_MEM_HEALTH : Free Memory $free_mem_in_mb MB"

if [ "$Min_Mem_Value" -ne 0 ] && [ "$free_mem_in_mb" -le "$Min_Mem_Value" ]
then
	# No need todo corrective action during box is in DiagnosticMode state
	DiagnosticMode=`syscfg get Selfheal_DiagnosticMode`
	if [ "$DiagnosticMode" == "true" ]
	then
		echo_t "RDKB_MEM_HEALTH : System free memory reached minimum threshold"
		echo_t "RDKB_MEM_HEALTH : Box is in diagnositic mode, so system not allow to clear the cache memory"				
	else
		echo_t "RDKB_MEM_HEALTH : System free memory reached minimum threshold , clearing the cache memory"
		sync
		echo 1 > /proc/sys/vm/drop_caches	
	fi
fi



