#This is used to save the total rxtx counter before reboot
#zhicheng_qiu@cable.comcast.com
#!/bin/sh
rxtx_enabled=`syscfg get rxtx_enabled`
if [ "$rxtx_enabled" != "1" ]; then
	exit 0;
fi
/fss/gw/usr/ccsp/tad/rxtx_sum.sh 
cp /tmp/rxtx_sum.txt /nvram/rxtx_bak.txt

