#!/bin/sh

source /etc/utopia/service.d/log_capture_path.sh

rotorLock=`dmcli eRT getv Device.Thermal.Fan.RotorLock | grep value | awk '{print $5}'`
if [ "$rotorLock" = "True" ]; then
    echo_t "THERMAL:Fan_Rotor_Lock"
fi
