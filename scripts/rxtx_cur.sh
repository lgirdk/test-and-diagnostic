#!/bin/sh
#######################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
#  Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################

#This script is used to get the current rxtx counter after bootup
#zhicheng_qiu@cable.comcast.com

source /etc/device.properties

if [ $BOX_TYPE = "XB3" ]; then
    MAC=`ifconfig l2sd0 | grep HWaddr | awk '{print $5}' | cut -c 1-14`
fi

#ebtables -L --Lc | grep CONTINUE  | sort -k 2,2 | sed "N;s/\n/ /" | cut -d' ' -f2,8,12,20,24 | awk 'BEGIN{FS="[: ]";}{ printf "%2s:%2s:%2s:%2s:%2s:%2s|%s|%s|%s|%s\n", $1,$2,$3,$4,$5,$6,$7,$8,$8,$10; }' | tr ' ' '0' | sort -u > /tmp/rxtx_cur.txt
if [ -z "$MAC" ]
then
    traffic_count -L | tr '[a-z]' '[A-Z]' > /tmp/rxtx_cur.txt
else
    traffic_count -L | grep -v $MAC | tr '[a-z]' '[A-Z]' > /tmp/rxtx_cur.txt
fi

ONETB=1099511627776
OIFS=$IFS
IFS='|'
while read mac rxpkts rxbytes txpkts txbytes other && [[ ! -z $mac ]]; do
    if [ $txbytes -ge $ONETB ];then
        high_upload_mac="${high_upload_mac}${high_upload_mac:+,}$mac"
        high_upload_bytes="${high_upload_bytes}${high_upload_bytes:+,}${txbytes%|}"
    fi
    if [ $rxbytes -ge $ONETB ];then
        high_download_mac="${high_download_mac}${high_download_mac:+,}$mac"
        high_download_bytes="${high_download_bytes}${high_download_bytes:+,}$rxbytes"
    fi
done < /tmp/rxtx_cur.txt
IFS=$OIFS
if [ ! -z "$high_upload_mac" ];then
    if [ $BOX_TYPE = "XB3" ]; then
        rpcclient $ATOM_ARPING_IP "echo "High_UploadData_Usage_Client_Bytes:$high_upload_bytes" >> /rdklogs/logs/Harvesterlog.txt.0"
        rpcclient $ATOM_ARPING_IP "echo "High_UploadData_Usage_Client_MAC:$high_upload_mac" >> /rdklogs/logs/Harvesterlog.txt.0"
    else
        echo "High_UploadData_Usage_Client_Bytes:$high_upload_bytes" >> /rdklogs/logs/Harvesterlog.txt.0
	echo "High_UploadData_Usage_Client_MAC:$high_upload_mac" >> /rdklogs/logs/Harvesterlog.txt.0
    fi
fi
if [ ! -z "$high_download_mac" ];then
    if [ $BOX_TYPE = "XB3" ]; then
        rpcclient $ATOM_ARPING_IP "echo "High_DownloadData_Usage_Client_Bytes:$high_download_bytes" >> /rdklogs/logs/Harvesterlog.txt.0"
        rpcclient $ATOM_ARPING_IP "echo "High_DownloadData_Usage_Client_MAC:$high_download_mac" >> /rdklogs/logs/Harvesterlog.txt.0"
    else
        echo "High_DownloadData_Usage_Client_Bytes:$high_download_bytes" >> /rdklogs/logs/Harvesterlog.txt.0
        echo "High_DownloadData_Usage_Client_MAC:$high_download_mac" >> /rdklogs/logs/Harvesterlog.txt.0
    fi
fi
cut -d'|' -f1 /tmp/rxtx_cur.txt | sort -u > /tmp/eblist
# Dump leases table - strip out mesh pods
grep -v "\* \*" /nvram/dnsmasq.leases | grep -v "172.16.12." | grep -v "58:90:43" | grep -v "60:b4:f7" | grep -v "b8: ee :0e" | grep -v "b8:d9:4d" | cut -d' ' -f2 > /tmp/cli4
ip nei show | grep brlan0 | grep -v FAILED | cut -d' ' -f 5  > /tmp/cli46
sort -u /tmp/cli4 /tmp/cli46 | tr '[a-z]' '[A-Z]' > /tmp/clilist
diff /tmp/eblist /tmp/clilist | grep "^+" | grep -v "+++" | cut -d'+' -f2 > /tmp/nclilist
for mac in $(cat /tmp/nclilist); do
  #ebtables -A INPUT -s $mac
  #ebtables -A OUTPUT -d $mac
  traffic_count -A $mac
done

