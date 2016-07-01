#Run rxtx_cur.sh 3 times per minute
#!/bin/sh
rxtx_enabled=`syscfg get rxtx_enabled`
if [ "$rxtx_enabled" != "1" ]; then
	if [ "$rxtx_enabled" -eq "0" ] && [ -e /tmp/eblist ]; then
		/usr/ccsp/tad/rxtx_res.sh
	fi
	exit 0;
fi
/usr/ccsp/tad/rxtx_cur.sh
sleep 19
/usr/ccsp/tad/rxtx_cur.sh
seep 19
/usr/ccsp/tad/rxtx_cur.sh

