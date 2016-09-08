#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "mta_hal.h"

#ifndef ULONG
#define ULONG unsigned long
#endif
  
int mtaBatteryGetPowerMode()
{
    char value[32] = {0};
    ULONG size = 0;
    if ( mta_hal_BatteryGetPowerStatus(value, &size) == 0 )
    {
    	if(strcmp(value,"Battery") == 0)
    	{
    		return 1;
    	}else {
    		return 0 ;
    	}
    } else {
    	return 0 ;
    }     
}

int main(int argc,char *argv[])
{
	int powermode_status = 0;

	if(argc < 2)
   	    return 0;
	if (strcmp(argv[1],"power_mode")==0)
	{
		mta_hal_InitDB();
		powermode_status= mtaBatteryGetPowerMode();
	}
	return powermode_status ;
}
