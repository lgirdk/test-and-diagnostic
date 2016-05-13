#This script is used to get the rxtx counter since last facotry reset
#zhicheng_qiu@cable.comcast.com

#output format:
#$MAC|$rx_pkt|$rx_bytes|$tx_pkt|$tx_bytes
#example:
#11:67:20:8d:43:dc|39962|22981470|336868|2132250
#22:67:20:8d:43:dc|29981|21490735|268434|2076612444444
#33:67:20:8d:43:dc|79962|62981470|736868|6153225133332
#55:67:20:8d:43:dc|29981|21490735|268434|207661258888

#!/bin/sh

if [ ! -f "/tmp/rxtx_bak.txt" ]; then
	if [ -f "/nvram/rxtx_bak.txt" ]; then
		cp /nvram/rxtx_bak.txt /tmp/rxtx_bak.txt
	fi
fi
if [ -f "/tmp/rxtx_bak.txt" ]; then
	awk 'BEGIN{FS="|";OFS="|"}{a[$1]+=$2;b[$1]+=$3;c[$1]+=$4;d[$1]+=$5;}END{for (k in a) print k,a[k],b[k],c[k],d[k]}' /tmp/rxtx_cur.txt /tmp/rxtx_bak.txt > /tmp/rxtx_sum.txt
else
	cp /tmp/rxtx_cur.txt /tmp/rxtx_sum.txt
fi

