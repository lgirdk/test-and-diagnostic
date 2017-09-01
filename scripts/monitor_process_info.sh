#! /bin/sh
source /etc/device.properties
source /etc/utopia/service.d/log_capture_path.sh

#List Zombie process list
dt=`date "+%m-%d-%y-%I-%M%p"`
ps -l | grep '^Z' > /tmp/zombies.txt
count=`cat /tmp/zombies.txt | wc -l`
echo "************ ZOMBIE_COUNT $count at $dt ************"

if [ $count -ne 0 ];then
	echo "*************** List of Zombies ***************"
	cat /tmp/zombies.txt
	echo "***********************************************"
fi
rm /tmp/zombies.txt

#Display count of ATOM and ARM Process
  HOST_PROCESS_MAX_THRESHOLD=400
  PEER_PROCESS_MAX_THRESHOLD=200	

  HOST_PROCESS_COUNT=`ps | wc -l`
  echo_t "TOTAL_HOST_Processes:$HOST_PROCESS_COUNT" >> /rdklogs/logs/SelfHeal.txt.0

    if [ $HOST_PROCESS_COUNT -gt $HOST_PROCESS_MAX_THRESHOLD ]; then
    	echo_t "HOST process count above threshold" >> /rdklogs/logs/SelfHeal.txt.0
    fi

    if [ "$BOX_TYPE" = "XB3" ]; then
      PEER_PROCESS_COUNT=`rpcclient $ATOM_ARPING_IP "ps | wc -l" | sed '4q;d'`
      echo_t "TOTAL_PEER_Processes:$PEER_PROCESS_COUNT" >> /rdklogs/logs/SelfHeal.txt.0

        if [ $PEER_PROCESS_COUNT -gt $PEER_PROCESS_MAX_THRESHOLD ]; then
                echo_t "PEER process count above threshold" >> /rdklogs/logs/SelfHeal.txt.0
        fi
    fi

