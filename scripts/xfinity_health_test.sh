#!/bin/sh
source /usr/ccsp/tad/corrective_action.sh
xfinitytestlogfile=/rdklogs/logs/xfinityTestAgent.log
xfinityenable=`psmcli get dmsb.hotspot.enable`
testenable=`syscfg get XfinityHealthCheckEnable`
testcadence=`syscfg get XfinityHealthCheckCadence`
daystotest=`syscfg get XfinityHealthCheckRemDays`
donetoday=`syscfg get XfinityHealthCheckDone`
grestatus="`ip link show gretap0 | grep DOWN`"

if [ "$xfinityenable" = "1" ] && [ "$testenable" = "true" ];then
    checkMaintenanceWindow
    if [ $reb_window -eq 1 ];then
        if [ "$donetoday" = "false" ];then
            if [ "$daystotest" = "0" ];then
                if [ "$grestatus" = "" ];then
                    /usr/bin/xfinitytest brTest 4091
                else
                    echo `date` ": HOTSPOT_HEALTHCHECK : GRE tunnel is down" >> $xfinitytestlogfile
                fi
                syscfg set XfinityHealthCheckRemDays $((--testcadence));
            else
                syscfg set XfinityHealthCheckRemDays $((--daystotest));
                echo `date` ": HOTSPOT_HEALTHCHECK :" $daystotest "days till Healthcheck" >> $xfinitytestlogfile
            fi
            syscfg set XfinityHealthCheckDone true;syscfg commit
        fi
    else
        echo `date` ": HOTSPOT_HEALTHCHECK : Not in Maintenance window" >> $xfinitytestlogfile
        syscfg set XfinityHealthCheckDone false;syscfg commit
    fi
else
    echo `date` ": HOTSPOT_HEALTHCHECK : Feature is disabled" >> $xfinitytestlogfile
fi

