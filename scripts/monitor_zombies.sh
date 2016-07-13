#! /bin/sh

source /etc/utopia/service.d/log_capture_path.sh

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
