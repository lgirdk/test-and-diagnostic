#!/bin/sh

Min_Mem_Value=`syscfg get MinMemoryThreshold_Value`

free_mem=`free | awk 'FNR == 2 {print $4}'`
free_mem_in_mb=$(($free_mem/1024))
echo "RDKB_MEM_HEALTH : Min Memory Threshold Value is $Min_Mem_Value"
echo "RDKB_MEM_HEALTH : Free Memory $free_mem_in_mb MB"

if [ "$Min_Mem_Value" -ne 0 ] && [ "$free_mem_in_mb" -le "$Min_Mem_Value" ]
then
	echo "RDKB_MEM_HEALTH : System free memory reached minimum threshold , clearing the cache memory"
	sync
	echo 1 > /proc/sys/vm/drop_caches	
fi



