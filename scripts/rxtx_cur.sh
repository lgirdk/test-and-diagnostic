#This script is used to get the current rxtx counter after bootup
#zhicheng_qiu@cable.comcast.com

#!/bin/sh
ebtables -L --Lc | grep CONTINUE  | sort -k 2,2 | sed "N;s/\n/ /" | cut -d' ' -f2,8,12,20,24 | tr " " "|" > /tmp/rxtx_cur.txt
cat /tmp/rxtx_cur.txt | cut -d'|' -f1 > /tmp/eblist
cat /nvram/dnsmasq.leases | grep -v "\* \*" | grep -v "172.16.12." | cut -d' ' -f2 > /tmp/cli4
ip nei show | grep brlan0 | grep -v FAILED | cut -d' ' -f 5  > /tmp/cli46
cat /tmp/cli4 /tmp/cli46 | sort -u > /tmp/clilist
diff /tmp/eblist /tmp/clilist | grep "^+" | grep -v "+++" | cut -d'+' -f2 > /tmp/nclilist
for mac in $(cat /tmp/nclilist); do
  ebtables -A INPUT -s $mac
  ebtables -A OUTPUT -d $mac
done

