#!/bin/sh
while [ 1 ]
do
     totalMemSys=`free | awk 'FNR == 2 {print $2}'`
     usedMemSys=`free | awk 'FNR == 2 {print $3}'`
     freeMemSys=`free | awk 'FNR == 2 {print $4}'`

     AvgMemUsed=$(( ( $usedMemSys * 100 ) / $totalMemSys ))
      echo "total memory: $totalMemSys"
      echo "usedMemory: $usedMemSys"
      echo "free memory: $freeMemsys"
      echo "Average memroy used :$AvgMemUsed"
      sleep 1
done
