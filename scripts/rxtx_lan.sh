#Run rxtx_cur.sh 3 times per minute
#!/bin/sh
rxtx_enabled=`syscfg get rxtx_enabled`
if [ "$rxtx_enabled" != "1" ]; then
	if [ "$rxtx_enabled" -eq "0" ] && [ -e /tmp/eblist ]; then
		ebtables -F;
		rm /tmp/eblist;
	fi
	exit 0;
fi
/fss/gw/usr/ccsp/tad/rxtx_cur.sh
sleep 19
/fss/gw/usr/ccsp/tad/rxtx_cur.sh
seep 19
/fss/gw/usr/ccsp/tad/rxtx_cur.sh

