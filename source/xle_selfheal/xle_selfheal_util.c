
/*********************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "sysevent/sysevent.h"
#include "safec_lib_common.h"
#include "ccsp_psm_helper.h"
#include <ccsp_base_api.h>
#include "ccsp_memory.h"
#include <syscfg/syscfg.h>


struct xle_attributes
{
  int devicemode;
  int is_lte_wan_up;
  int iswan_dhcp_client;
  int iswan_dhcp_server;
  int isdhcp_server_running;
  char mesh_wan_status[128];
  int is_ipv6present;
  int is_ipv4present;
  int is_ipv4_wan_route_table;
  int is_ipv6_wan_route_table;
  int is_ipv4_mesh_route_table;
  int is_ipv6_mesh_route_table;
  int is_ipv4_mesh_brWan_link;
  int is_ipv6_mesh_brWan_link;
  int is_default_route;
  int is_mesh_default_route;

}xle_attributes;

struct xle_attributes xle_params;
static char default_wan_ifname[50];
static char current_wan_ifname[50];
static int            sysevent_fd = -1;
static token_t        sysevent_token;
int retPsmGet = CCSP_SUCCESS;
char mesh_interface_name[128] = {0};
char comp_status_cmd[256] = {0};
char lte_wan_status[128] = {0};
char lte_backup_enable[128] = {0};

static char *g_Subsystem = "eRT." ;
/* CID 282121 fix */
char InterfaceStatus[256] = {0};
#define MESH_IFNAME        "br-home"
static void*    bus_handle = NULL;
#define CELLULAR_COMPONENT_NAME  "eRT.com.cisco.spvtg.ccsp.cellularmanager"
#define CELLULAR_DBUS_PATH  "/com/cisco/spvtg/ccsp/cellularmanager" 

FILE* logFp = NULL;

#define  xle_log( msg ...){ \
                ANSC_UNIVERSAL_TIME ut; \
                AnscGetLocalTime(&ut); \
                if ( logFp != NULL){ \
                fprintf(logFp, "%.2d%.2d%.2d-%.2d:%.2d:%.2d ", ut.Year,ut.Month,ut.DayOfMonth,ut.Hour,ut.Minute,ut.Second); \
                fprintf(logFp, msg);} \
}


void check_lte_provisioned(char* lte_wan,char* lte_backup_enable )
{
    char *paramNames[]= { "Device.Cellular.X_RDK_Status", "Device.Cellular.X_RDK_Enable" };
    parameterValStruct_t **retVal = NULL;
    int nval;
     int ret = CcspBaseIf_getParameterValues( 
		bus_handle,
        CELLULAR_COMPONENT_NAME,
        CELLULAR_DBUS_PATH,
        paramNames,
        2,
        &nval,
        &retVal);
    if (CCSP_SUCCESS == ret)
    {
        if (NULL != retVal[0]->parameterValue)
        {
            strncpy(lte_wan, retVal[0]->parameterValue, strlen(retVal[0]->parameterValue) + 1);
        }
        if(NULL != retVal[1]->parameterValue)
        {
            strncpy(lte_backup_enable, retVal[1]->parameterValue, strlen(retVal[1]->parameterValue) + 1);
        }

        if (retVal)
        {
            free_parameterValStruct_t(bus_handle, nval, retVal);
        }
    }
}
void GetInterfaceStatus( char* InterfaceStatus, char* comp_status_cmd, int size )
{
  FILE *fp;
	char path[256] = {0};
	fp = popen(comp_status_cmd,"r");
	if( fp != NULL )
	{
		if(fgets(path, sizeof(path)-1, fp) != NULL)
		{
			char *p;
			path[strlen(path) - 1] = '\0';
			 if ((p = strchr(path, '\n'))) {
 				*p = '\0';
			}
			strncpy(InterfaceStatus,path,size-1);
    }
     pclose(fp);
  }
}

void PopulateParameters()
{
    char mesh_status[128] = {0};
    sysevent_fd =  sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "xle_selfheal", &sysevent_token);
    sysevent_get(sysevent_fd, sysevent_token, "wan_ifname", default_wan_ifname, sizeof(default_wan_ifname));
    sysevent_get(sysevent_fd, sysevent_token, "current_wan_ifname", current_wan_ifname, sizeof(current_wan_ifname));
    sysevent_get(sysevent_fd, sysevent_token, "mesh_wan_linkstatus", mesh_status, sizeof(mesh_status));
    sysevent_close(sysevent_fd, sysevent_token);
    char *paramValue = NULL;
    char*  component_id = "ccsp.xle_self";
    CCSP_Message_Bus_Init(component_id,
                                CCSP_MSG_BUS_CFG,
                                &bus_handle,
                                (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback,
                                Ansc_FreeMemory_Callback);
    int retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, "dmsb.Mesh.WAN.Interface.Name", NULL, &paramValue);
    if (retPsmGet == CCSP_SUCCESS)
    {        strncpy(mesh_interface_name,paramValue,sizeof(mesh_interface_name)-1);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(paramValue);
    }
    check_lte_provisioned(lte_wan_status, lte_backup_enable );
    if(( 0 == strncmp( lte_wan_status, "CONNECTED", 9 )) && ( 0 == strncmp( lte_backup_enable, "true", 4 )))
    {
        sprintf(comp_status_cmd,"ifconfig %s | grep UP",current_wan_ifname);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_lte_wan_up = 1;
        }
        else
        {
            xle_params.is_lte_wan_up = 0;
        }
        
        if(  xle_params.is_lte_wan_up )
        {
            if( xle_params.devicemode )
            {
                memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
                sprintf(comp_status_cmd,"ps | grep dnsmasq | grep -v grep");
                memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
                GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
                if ( InterfaceStatus[0] != '\0' )
                {
                    xle_params.isdhcp_server_running = 1;
                    memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
                    sprintf(comp_status_cmd,"grep %s /var/dnsmasq.conf",mesh_interface_name);
                    memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
                    GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
                    if ( InterfaceStatus[0] != '\0' )
                    {
                        xle_params.iswan_dhcp_server = 1;
                    }
                    else
                    {
                        xle_params.iswan_dhcp_server = 0;
                    }
                }
                else
                {
                    xle_params.isdhcp_server_running=0;
                }
            }
            else
            {
                memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
                sprintf(comp_status_cmd,"ps w | grep udhcpc | grep %s | grep -v grep",mesh_interface_name);
                memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
                GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
                if ( InterfaceStatus[0] != '\0' )
                {
                    xle_params.iswan_dhcp_client = 1;
                }
                else
                {
                    xle_params.iswan_dhcp_client = 0;
                }
            }
            
           strcpy(xle_params.mesh_wan_status,  mesh_status);

            memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
            sprintf(comp_status_cmd,"ifconfig %s | grep inet6 | grep \"Scope:Global\"",default_wan_ifname);
            memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
            GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
            if ( InterfaceStatus[0] != '\0' )
            {
                xle_params.is_ipv6present = 1;
            }
            else
            {
                xle_params.is_ipv6present = 0;
            }
            memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
            sprintf(comp_status_cmd,"ifconfig %s | grep \"inet\" | grep -v \"inet6\"",default_wan_ifname);
            memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
            GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
            if ( InterfaceStatus[0] != '\0' )
            {
                xle_params.is_ipv4present = 1;
            }
            else
            {
                xle_params.is_ipv4present = 0;
            }
        }
    }
    else
    {
        xle_log("cannot check wan connectivity as it is not provisioned \n");

    }
    
    if( xle_params.devicemode )
    {

        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip route show table 12 | grep \"default via\" | grep %s",default_wan_ifname);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv4_wan_route_table = 1;
        }
        else
        {
            xle_params.is_ipv4_wan_route_table = 0;
        }

        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip -6 route show table 12 | grep \"default via\"  | grep %s",default_wan_ifname);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv6_wan_route_table= 1;
        }
        else
        {
            xle_params.is_ipv6_wan_route_table = 0;
        }

        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip route show table 11 | grep \"default via\" | grep %s",mesh_interface_name);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv4_mesh_route_table = 1;
        }
        else
        {
            xle_params.is_ipv4_mesh_route_table  = 0;
        }

        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip -6 route show table 11 | grep \"default via\" | grep %s",mesh_interface_name);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv6_mesh_route_table = 1;
        }
        else
        {
            xle_params.is_ipv6_mesh_route_table = 0;
        }


        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip route show | grep %s",mesh_interface_name);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv4_mesh_brWan_link = 1;
        }
        else
        {
            xle_params.is_ipv4_mesh_brWan_link = 0;
        }


        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip -6 route show | grep %s",mesh_interface_name);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_ipv6_mesh_brWan_link = 1;
        }
        else
        {
            xle_params.is_ipv6_mesh_brWan_link = 0;
        }
        
        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip route list | grep \"default via\" | grep %s",MESH_IFNAME);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_mesh_default_route = 1;
        }
        else
        {
            xle_params.is_mesh_default_route = 0;
        }
    }
    else
    {
        memset(comp_status_cmd, 0, sizeof(comp_status_cmd));
        sprintf(comp_status_cmd,"ip route list | grep \"default via\" | grep %s",current_wan_ifname);
        memset(InterfaceStatus, 0, sizeof(InterfaceStatus));
        GetInterfaceStatus( InterfaceStatus, comp_status_cmd, sizeof(comp_status_cmd) );
        if ( InterfaceStatus[0] != '\0' )
        {
            xle_params.is_default_route = 1;
        }
        else
        {
            xle_params.is_default_route = 0;
        }
    }
    
}

void isWan_up()
{
    if( xle_params.is_lte_wan_up )
    {
        xle_log("[xle_self_heal] current wan interface is up \n");
    }
    else
    {
        xle_log("[xle_self_heal] current wan interface is down \n");
    }
}

int Get_Device_Mode()
{
    int deviceMode = 0;
    char buf[8]= {0};
    memset(buf,0,sizeof(buf));
    if ( 0 == syscfg_get(NULL, "Device_Mode", buf, sizeof(buf)))
    {   
        if (buf[0] != '\0' && strlen(buf) != 0 )
            deviceMode = atoi(buf);
    }
    return deviceMode;

}
int main(int argc,char *argv[])
{
    if(argc<2){
        xle_log("Syntax: error - no of arguments\n");
        exit(1);
    }
    logFp = fopen("/rdklogs/logs/SelfHeal.txt.0","a+") ;
    xle_params.devicemode = atoi( argv[1]);
    PopulateParameters();
    if( xle_params.devicemode == Get_Device_Mode())
    {
        xle_log("[xle_self_heal] Device modes are same, printing the details\n");
        xle_log("[xle_self_heal] lte_wan_provisioned status :%s\n", lte_wan_status);
        if(( 0 == strncmp( lte_wan_status, "CONNECTED", 9 )) && ( 0 == strncmp( lte_backup_enable, "true", 4 )))
        {
            isWan_up();
            if( xle_params.is_lte_wan_up )
            {
                if(xle_params.devicemode)
                {
                    if( xle_params.isdhcp_server_running )
                    {
                        xle_log("[xle_self_heal] dhcp server running \n"); 
                        xle_log("[xle_self_heal] dhcp server running on mesh interface status:%d \n", xle_params.iswan_dhcp_server);
                    }
                    else
                    {
                         xle_log("[xle_self_heal] dhcp server is not running \n"); 
                    }
                }
                else
                {
                    xle_log("[xle_self_heal] dhcp client running on mesh interface status:%d \n", xle_params.iswan_dhcp_client);
                }
                
                xle_log("[xle_self_heal] mesh wan status is :%s \n", xle_params.mesh_wan_status);
                xle_log("[xle_self_heal] lte wan interface(wwan0) is having ip v4 address status :%d \n", xle_params.is_ipv4present);
                xle_log("[xle_self_heal] lte wan interface(wwan0) is having ip v6 address status :%d \n", xle_params.is_ipv6present);
              
            }
        }

        if( xle_params.devicemode )
        {
            if(xle_params.is_ipv4_wan_route_table)
            {
                xle_log("[xle_self_heal] ipv4  route table 12 is having wwan0 default interface name \n");
            }
            else
            {
                xle_log("[xle_self_heal] ipv4 route table 12 is not having wwan0 interface name \n");
            }
            if(xle_params.is_ipv6_wan_route_table)
            {
                xle_log("[xle_self_heal] ipv6 route table 12 is having wwan0 interface name \n");
            }
            else
            {
                xle_log("[xle_self_heal] ipv6  route table 12 is not having wwan0 interface name \n");
            }
            if(xle_params.is_ipv4_mesh_route_table)
            {
                xle_log("[xle_self_heal] ipv4 is route table 11 is having default mesh interface name \n");
            }
            else
            {
                xle_log("[xle_self_heal] ipv4 route table 11 is not having default mesh interface name \n");
            }
            if(xle_params.is_ipv6_mesh_route_table)
            {
                xle_log("[xle_self_heal] ipv6 route table 11 is having default mesh interface name \n");
            }
            else
            {
                xle_log("[xle_self_heal] ipv6 route table 11  is not having default mesh interface name \n");
            }
            if(xle_params.is_ipv4_mesh_brWan_link)
            {
                xle_log("[xle_self_heal] ip route show is having brWAN interface link\n");
            }
            else
            {
                xle_log("[xle_self_heal] ip  route show is not having brWAN interface link \n");
            }
            if(xle_params.is_ipv6_mesh_brWan_link)
            {
                xle_log("[xle_self_heal] ipv6 route show is having brWAN interface link \n");
            }
            else
            {
                xle_log("[xle_self_heal] ipv6 route show is not having brWAN interface link \n");
            }

            if(xle_params.is_mesh_default_route ) 
            {
                xle_log("[xle_self_heal] default route contains br-home in device mode %d\n", xle_params.devicemode); 
            }
            else
            {
                xle_log("[xle_self_heal] default route doesnot contains br-home in device mode %d\n", xle_params.devicemode); 
            }
        }
        else
        {
            if(xle_params.is_default_route ) 
            {
                xle_log("[xle_self_heal] default route contains %s in device mode %d\n",current_wan_ifname, xle_params.devicemode); 
            }
            else
            {
                xle_log("[xle_self_heal] default route doesnot contains %s in device mode %d\n",current_wan_ifname, xle_params.devicemode); 
            }
        }
    }
    else
    {
       xle_log("[xle_self_heal] Device mode changedin between, no need to print the details\n"); 
    }
    
    if ( logFp != NULL)
    {
        fclose(logFp);
        logFp= NULL;
    }
 return 1 ;
}



