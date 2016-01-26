#!/bin/sh

source /fss/gw/etc/utopia/service.d/log_env_var.sh

exec 3>&1 4>&2 >>$SELFHEALFILE 2>&1
rand_num_old=""
source /fss/gw/usr/ccsp/tad/corrective_action.sh

# Generate random time to start 
WAN_INTERFACE=erouter0

calcRandom=1
ping4_server_num=0
ping6_server_num=0
ping4_success=0
ping6_success=0
ping4_failed=0
ping6_failed=0
calcRandTimetoStartPing()
{

    rand_min=0
    rand_sec=0

    # Calculate random min
    rand_min=`awk -v min=0 -v max=59 -v seed="$(date +%N)" 'BEGIN{srand(seed);print int(min+rand()*(max-min+1))}'`

    # Calculate random second
    rand_sec=`awk -v min=0 -v max=59 -v seed="$(date +%N)" 'BEGIN{srand(seed);print int(min+rand()*(max-min+1))}'`

    sec_to_sleep=$(($rand_min*60 + $rand_sec))
    sleep $sec_to_sleep; 
        
}

runPingTest()
{

	PINGCOUNT=`syscfg get ConnTest_NumPingsPerServer`

	if [ "$PINGCOUNT" = "" ] 
	then
		PINGCOUNT=3
	fi

	MINPINGSERVER=`syscfg get ConnTest_MinNumPingServer`

	if [ "$MINPINGSERVER" = "" ] 
	then
		MINPINGSERVER=1
	fi	

	RESWAITTIME=`syscfg get ConnTest_PingRespWaitTime`

	if [ "$RESWAITTIME" = "" ] 
	then
		RESWAITTIME=1000
	fi
	RESWAITTIME=$(($RESWAITTIME/1000))

	IPV4_SERVER_COUNT=`syscfg get Ipv4PingServer_Count`
	IPV6_SERVER_COUNT=`syscfg get Ipv6PingServer_Count`

	# Generate random number to find out which server to ping 
	while [ "$IPV4_SERVER_COUNT" -ne 0 ] || [ "$IPV6_SERVER_COUNT" -ne 0 ]
	do

		ping4_server_num=$((ping4_server_num+1))
		if [ "$ping4_success" -ne 1 ] && [ "$ping4_server_num" -le "$IPV4_SERVER_COUNT" ] && [ "$IPV4_SERVER_COUNT" -ne 0 ]
		then
			PING_SERVER_IS=`syscfg get Ipv4_PingServer_$ping4_server_num`
			if [ "$PING_SERVER_IS" != "" ] && [ "$PING_SERVER_IS" != "0.0.0.0" ]
			then
				PING_OUTPUT=`ping -I $WAN_INTERFACE -c $PINGCOUNT -w $RESWAITTIME $PING_SERVER_IS`
				CHECK_PACKET_RECEIVED=`echo $PING_OUTPUT | grep "packet loss" | cut -d"%" -f1 | awk '{print $NF}'`
				if [ "$CHECK_PACKET_RECEIVED" != "" ]
				then
					if [ "$CHECK_PACKET_RECEIVED" -ne 100 ] 
					then
						ping4_success=1
					else
						ping4_failed=1
					fi
				else
					ping4_failed=1
				fi
			fi
		  fi
		

		  ping6_server_num=$((ping6_server_num+1))
		  if [ "$ping6_success" -ne 1 ] && [ "$ping6_server_num" -le "$IPV6_SERVER_COUNT" ] && [ "$IPV6_SERVER_COUNT" -ne 0 ]
		  then
			PING_SERVER_IS=`syscfg get Ipv6_PingServer_$ping6_server_num`
			if [ "$PING_SERVER_IS" != "" ] && [ "$PING_SERVER_IS" != "0000::0000" ]
			then
				PING_OUTPUT=`ping -I $WAN_INTERFACE -c $PINGCOUNT -w $RESWAITTIME $PING_SERVER_IS`
				CHECK_PACKET_RECEIVED=`echo $PING_OUTPUT | grep "packet loss" | cut -d"%" -f1 | awk '{print $NF}'`
				if [ "$CHECK_PACKET_RECEIVED" != "" ]
				then
					if [ "$CHECK_PACKET_RECEIVED" -ne 100 ] 
					then
						ping6_success=1
					else
						ping6_failed=1
					fi
				else
					ping6_failed=1
				fi
			fi
		  fi


		  if [ "$ping4_success" -eq 1 ] && [ "$ping6_success" -eq 1 ]
		  then
		  	break
		  fi

		  if [ "$ping4_success" -eq 1 ] || [ "$ping6_success" -eq 1 ]
		  then
				if [ "$IPV4_SERVER_COUNT" -eq 0 ] || [ "$IPV6_SERVER_COUNT" -eq 0 ]
				then
					break
				fi

		  fi
		  
		  if [ "$ping4_server_num" -ge "$IPV4_SERVER_COUNT" ] && [ "$ping6_server_num" -ge "$IPV6_SERVER_COUNT" ]
		  then
		 	break
		  fi
		
	done


	if [ "$IPV4_SERVER_COUNT" -eq 0 ] ||  [ "$IPV6_SERVER_COUNT" -eq 0 ]
	then

			if [ "$IPV4_SERVER_COUNT" -eq 0 ] && [ "$IPV6_SERVER_COUNT" -eq 0 ]
			then
				echo "RDKB_SELFHEAL : Ping server lists are empty , not taking any corrective actions"				

			elif [ "$ping4_success" -ne 1 ] && [ "$IPV4_SERVER_COUNT" -ne 0 ]
			then
				echo "RDKB_SELFHEAL : Ping to IPv4 servers are failed, need to take corrective action"
				rebootNeeded PING
			elif [ "$ping6_success" -ne 1 ] && [ "$IPV6_SERVER_COUNT" -ne 0 ]
			then
				echo "RDKB_SELFHEAL : Ping to IPv6 servers are failed, need to take corrective action"
				rebootNeeded PING
			else
				echo "RDKB_SELFHEAL : One of the ping server list is empty, ping to the other list is successfull"
				echo "RDKB_SELFHEAL : Connectivity Test is Successfull"
			fi	

	elif [ "$ping4_success" -ne 1 ] &&  [ "$ping6_success" -ne 1 ]
	then
		echo "RDKB_SELFHEAL : Ping to both IPv4 and IPv6 servers are failed, need to take corrective action"
		rebootNeeded PING
	elif [ "$ping4_success" -ne 1 ]
	then
		echo "RDKB_SELFHEAL : Ping to IPv4 servers are failed, need to take corrective action"
		rebootNeeded PING
	elif [ "$ping6_success" -ne 1 ]
	then
		echo "RDKB_SELFHEAL : Ping to IPv6 servers are failed, need to take corrective action"
		rebootNeeded PING
	else
		echo "RDKB_SELFHEAL : Connectivity Test is Successfull"
	fi	

	ping4_success=0
	ping4_failed=0
	ping6_success=0
	ping6_failed=0
	ping4_server_num=0
	ping6_server_num=0

}

SELFHEAL_ENABLE=`syscfg get selfheal_enable`

while [ $SELFHEAL_ENABLE = "true" ]
do

	if [ "$calcRandom" -eq 1 ] 
	then

		calcRandTimetoStartPing
		calcRandom=0
	else
		INTERVAL=`syscfg get ConnTest_PingInterval`

		if [ "$INTERVAL" = "" ] 
		then
			INTERVAL=60
		fi
		INTERVAL=$(($INTERVAL*60))
		sleep $INTERVAL
	fi


	runPingTest

	SELFHEAL_ENABLE=`syscfg get selfheal_enable`
	# ping -I $WAN_INTERFACE -c $PINGCOUNT 
		
done
