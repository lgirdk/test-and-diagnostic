#Run rxtx_cur.sh 3 times per minute
#!/bin/sh
if [ ! -e "/tmp/rxtx_enabled" ]; then
	exit 0;
fi
/usr/ccsp/tad/rxtx_cur.sh
sleep 19
/usr/ccsp/tad/rxtx_cur.sh
sleep 19
/usr/ccsp/tad/rxtx_cur.sh

