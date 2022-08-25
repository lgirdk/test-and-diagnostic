#!/bin/sh
source /etc/log_timestamp.sh

tmp_file=$(find /tmp -name "jst_sess*")
for file in $tmp_file
do
        diff=$((($(date +%s) - $(stat $file -c %Y)) / 60 ))
        if [ "$diff" -ge 15 ]; then
           echo_t "[GUI] Deleted $file due to session inactivity" >> /rdklogs/logs/Consolelog.txt.0
           rm -f $file
        fi
done
