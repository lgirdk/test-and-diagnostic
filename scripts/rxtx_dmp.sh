#This is used to save the total rxtx counter before reboot
#zhicheng_qiu@cable.comcast.com
#!/bin/sh
/fss/gw/usr/ccsp/tad/rxtx_sum.sh 
cp /tmp/rxtx_sum.txt /nvram/rxtx_bak.txt

