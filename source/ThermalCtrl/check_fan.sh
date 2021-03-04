#!/bin/sh

source /etc/utopia/service.d/log_capture_path.sh

rotorLock_1=`dmcli eRT getv Device.Thermal.Fan.1.RotorLock | grep value | awk '{print $5}'`
rotorLock_2=`dmcli eRT getv Device.Thermal.Fan.2.RotorLock | grep value | awk '{print $5}'`
if [ "$rotorLock_1" = "True" ] || [ "$rotorLock_2" = "True" ]; then
    echo_t "THERMAL:Fan_Rotor_Lock"
fi
