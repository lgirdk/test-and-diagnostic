#! /bin/sh
syscfg set todays_reboot_count 0 
syscfg commit 
syscfg set todays_reset_count 0 

syscfg set lastActiontakentime 0 
syscfg set highloadavg_reboot_count 0
syscfg commit 

