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
#include <stdint.h>
#include "syscfg.h"
#include "safec_lib_common.h"
#include "secure_wrapper.h"
#include <telemetry_busmessage_sender.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/sysinfo.h>

#ifdef CORE_NET_LIB
#include <libnet.h>
#endif

/* Macros */
#define MAX_BRIDGE_NAMESZ 64
#define BUFFER_SZ 1024
#define TELEMETRY_MAX_BUFF 4096
#define RXTXSTATS_TMPFILE "/tmp/rxtxstats.txt"
#define RXTXSTATS_TMPFILE_1 "/tmp/rxtxstats_1.txt"
#define LOG_FILE            "/rdklogs/logs/RXTX100Log.txt"

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

/* Derived from map.c in mesh */
#if defined (_XB6_PRODUCT_REQ_) && !defined (_XB7_PRODUCT_REQ_)
#define BACKHAUL_2G "ath12"
#define BACKHAUL_5G "ath13"
#elif defined (_XB6_PRODUCT_REQ_) && defined (_XB7_PRODUCT_REQ_)
#define BACKHAUL_2G "brlan112"
#define BACKHAUL_5G "brlan113"
#elif defined (_XB6_PRODUCT_REQ_) && defined (_XB7_PRODUCT_REQ_) \
&& defined (_XB8_PRODUCT_REQ_)
#define BACKHAUL_2G "brlan112"
#define BACKHAUL_5G "brlan113"
#elif defined (_SR300_PRODUCT_REQ_) || defined _SR213_PRODUCT_REQ_ \
||  defined (_SE501_PRODUCT_REQ_) || defined (_WNXL11BWL_PRODUCT_REQ_) \
|| defined(_HUB4_PRODUCT_REQ_)
#define BACKHAUL_2G "brlan6"
#define BACKHAUL_5G "brlan7"
#else
/*Not sure what to take, but for now taking some common,
 * may produce interface not exists logs*/
#define BACKHAUL_2G "brlan112"
#define BACKHAUL_5G "brlan113"
#endif

typedef struct BridgeMap_s
{
    char   *iface;
    char   *feature;
} BridgeMap_t;

static BridgeMap_t BridgeMap[] =
{
    {"brlan0",      "Private"},
    {"brlan1",      "XHS"},
    {"br106",       "LnF"},
    {"br403",       "MeshBackhaul"},
    {BACKHAUL_2G,   "2.4Backhaul"},
    {BACKHAUL_5G,   "5Backhaul"},
    {"brebhaul",    "EthernetBackhaul"},
    {"brlan2",      "Hotspot_Open2.4"},
    {"brlan3",      "Hotspot_Open5"},
    {"brlan4",      "Hotspot_Secure2.4"},
    {"brlan5",      "Hotspot_Secure5"}
};

#define BridgeMap_TOTAL (sizeof(BridgeMap)/sizeof(BridgeMap[0]))

/*globals*/
FILE *log_fd = NULL;
static unsigned int no_of_interfaces=0;
char **current_interface_list = NULL;

/* Function declarations*/

int start_rxtx();
static int populate_interfacelist();
void write_to_logfile(const char *fmt, ...);
static int check_interface_exist(char *ifname);
static int fetch_interface_stats(char *ifname,uint64_t *rx_bytes,uint64_t *tx_bytes);
static int read_from_file(char *filename,char *bridge_name,uint64_t *rx_bytes,uint64_t *tx_bytes);
static int store_to_file(char *filename,char *bridge_name,uint64_t rx_bytes,uint64_t tx_bytes);
char *get_bridge_map(char *bridge_name);

 /* rxtx main

 Trigger :   This will be triggered from the cron job every 1 hr,
 Operation : will fetch the interaface stats based on the interfaces
 input or default set of interfaces

 */

int main(int argc,char *argv[])
{

    /* Since cron uses system clock, there may be chances we can
     * fall here at very early stages of boot. if we are within 15 mins
     * of uptime, lets skip*/
    struct sysinfo s_info;

    if (!sysinfo(&s_info))
    {
       if (s_info.uptime < 900)
	  return 0;
    }

/* Open log fd, ensure to close after done*/
    log_fd = fopen(LOG_FILE , "a+");
    if (!log_fd)
    {
        fprintf(stderr,"unable to open log fd for RxTx, Abort\n");
        return -1;
    }

/* Lets initialize syscfg*/
    if (syscfg_init() == -1)
    {
       write_to_logfile("syscfg_init failed rxtx\n");
       fclose(log_fd);
       return -1;
    }

    if (start_rxtx() != SUCCESS)
    {
        write_to_logfile("RxTx100 Capture Failed\n");
    }
    fclose(log_fd);
}

int start_rxtx()
{
    int ret = FAILURE;
    errno_t rc = -1;
    int i;
    char print_list[BUFFER_SZ] = {0};

    /* Lets derive the interface list required to check for stats*/
    if (populate_interfacelist() != SUCCESS)
    {
        write_to_logfile("Unable to populate_interfacelist, Abort RxTxStats Capture\n");
        /* there may be chances the list is allocated partially, lets free if any*/
        goto EXIT;
    }

    /* Not much of possibilities, checking again to double confirm*/
    if (!no_of_interfaces)
    {
        write_to_logfile("Interface list is empty,Abort RxTxStats Capture\n");
        goto EXIT;
    }

    /* Print the interface list, for readability and to check interfaces taken for consideration*/
    sprintf_s(print_list,BUFFER_SZ, "RxTx Interface list to collect stats : ");
    for (i=0;i < no_of_interfaces;i++)
	sprintf_s(print_list+strlen(print_list),BUFFER_SZ-strlen(print_list), "%s,",
							current_interface_list[i]);
    print_list[strlen(print_list)-1] = '\n';

    write_to_logfile("%s",print_list);

    if (access(RXTXSTATS_TMPFILE,F_OK))
    {
        write_to_logfile("we don't have last_rx_bytes and tx bytes file, setting to 0\n");
    }

    for (i=0;i < no_of_interfaces;i++)
    {
        /*  for every interfaces fetch current stats*/
        if (check_interface_exist(current_interface_list[i]) == SUCCESS)
        {
            /*ressetting on each iteration*/
            uint64_t rx_bytes = 0;
            uint64_t tx_bytes = 0;
            uint64_t last_rx_bytes = 0;
            uint64_t last_tx_bytes = 0;
            uint64_t diff_rx_bytes = 0;
            uint64_t diff_tx_bytes = 0;
            char buf[BUFFER_SZ]    = {0};
            char eventName[MAX_BRIDGE_NAMESZ]={0};
            char *bridge_map = get_bridge_map(current_interface_list[i]);

            ret = fetch_interface_stats(current_interface_list[i],&rx_bytes,&tx_bytes);
            if ( ret != FAILURE)
            {
                /*Read last rx bytes and tx bytes stored*/
                if (!access(RXTXSTATS_TMPFILE,F_OK))
                {
                    if (read_from_file(RXTXSTATS_TMPFILE,current_interface_list[i],
                                        &last_rx_bytes,&last_tx_bytes) == FAILURE)
                    {
                        write_to_logfile("we don't have last_rx_bytes and tx bytes data for bridge %s,setting to 0\n",
					  current_interface_list[i]);
                        last_rx_bytes = 0;
                        last_tx_bytes = 0;
                    }
                }

                /*Calculate diff*/
                diff_rx_bytes = (rx_bytes >= last_rx_bytes) ? (rx_bytes - last_rx_bytes) : 0;
                diff_tx_bytes = (tx_bytes >= last_tx_bytes) ? (tx_bytes - last_tx_bytes) : 0;

		double rx_bytes_mb = ((double)rx_bytes) / 1024.0 / 1024.0;
		double tx_bytes_mb = ((double)tx_bytes) / 1024.0 / 1024.0;
		double diff_rx_bytes_mb = ((double)diff_rx_bytes) / 1024.0 / 1024.0;
		double diff_tx_bytes_mb = ((double)diff_tx_bytes) / 1024.0 / 1024.0;

                rc = sprintf_s(buf, sizeof(buf), "%0.2f|%0.2f|%0.2f|%0.2f",
                                                    rx_bytes_mb,tx_bytes_mb,diff_rx_bytes_mb,diff_tx_bytes_mb);
                if (rc < EOK)
                {
                    ERR_CHK(rc);
                    write_to_logfile("Error in populating data buffer\n");
                    continue;
                }
                /* store current value to file for futher reference*/
                store_to_file(RXTXSTATS_TMPFILE_1,current_interface_list[i],rx_bytes,tx_bytes);
                write_to_logfile("RDKBDataConsumption:%s|%s\n",bridge_map,buf);
                rc = sprintf_s(eventName,sizeof(eventName),"dataconsump_split_%s",bridge_map);
                if (rc < EOK)
                {
                    ERR_CHK(rc);
                    write_to_logfile("Error in populating telemetry eventName\n");
                    continue;
                }
                t2_event_s(eventName,buf);
            }
            else
            {
                write_to_logfile("interface_get_stats returned error\n");
            }
        }
        else
        {
            write_to_logfile("%s:Interface doesn't exists,skip collecting rxtx stats\n",current_interface_list[i]);
        }
    }

    /* Lets store it to the actual file, Note we are removing tmp store file too*/
    ret = v_secure_system("mv %s %s", RXTXSTATS_TMPFILE_1,RXTXSTATS_TMPFILE);
    if (ret != SUCCESS)
    {
        write_to_logfile("Failure in copy file via v_secure_system. ret:[%d] \n", ret);
    }

EXIT:
    for (i=0;i < no_of_interfaces;i++) {
        if (current_interface_list[i])
            free(current_interface_list[i]);
    }

    if (current_interface_list)
    {
        free(current_interface_list);
    }

    return SUCCESS;
}

static int populate_interfacelist()
{
    int rc =0;
    char val[BUFFER_SZ] = {0};
    int count = 0;

    if (!syscfg_get(NULL, "rxtxstats_interface_list", val, sizeof(val)))
    {
        if (val[0] != '\0')
        {
            char *delimiter = ",";
            char* tok;

            tok = strtok(val, delimiter);

            // Checks for delimiter
            while (tok != 0) {
                if (!count)
                {
                    current_interface_list = (char **)calloc(1,sizeof (char *));
                    if (!current_interface_list)
                    {
                        write_to_logfile("%s:calloc failed\n",__FUNCTION__);
                        return FAILURE;
                    }
                }
                else
                {
                    current_interface_list = (char **)realloc(current_interface_list,sizeof(char *)*(count+1));
                    if (!current_interface_list)
                    {
                        write_to_logfile("%s:realloc failed\n",__FUNCTION__);
                        return FAILURE;
                    }
                }
                current_interface_list[count] = (char *)calloc(1,MAX_BRIDGE_NAMESZ);
                if (!current_interface_list[count])
                {
                    write_to_logfile("Unable to allocate memory\n",__FUNCTION__);
                    return FAILURE;
                }
                rc = strcpy_s(current_interface_list[count++],MAX_BRIDGE_NAMESZ,tok);
                ERR_CHK(rc);
                tok = strtok(0, delimiter);
            }
        }
        else
        {
            write_to_logfile("syscfg rx tx interface list is empty\n");
        }
    }
    else
    {
        write_to_logfile("%s:Interface list not configured or default list not available\n",
									      __FUNCTION__);
    }
    no_of_interfaces = count;
    return SUCCESS;
}

static int check_interface_exist(char *ifname)
{
    struct ifreq ifr;
    int fd;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        if (errno == ENODEV) {
            close(fd);
            return FAILURE;
        }
    }
    close(fd);
    return SUCCESS;
}

static int fetch_interface_stats(char *ifname,uint64_t *rx_bytes,uint64_t *tx_bytes)
{
    int ret = SUCCESS;
/* Fetching from core net LIB, we need only BYTES so setting mask as IFSTAT_RXTX_BYTES*/
#if defined CORE_NET_LIB
    cnl_iface_stats rxtxIface;
    memset(&rxtxIface,0,sizeof(cnl_iface_stats));
    ret=interface_get_stats(IFSTAT_RXTX_BYTES, ifname, &rxtxIface);
    if (ret == SUCCESS)
    {
        *rx_bytes=rxtxIface.rx_bytes;
        *tx_bytes=rxtxIface.tx_bytes;
    }
    return ret;
#else
/* No core net lib support, have to fetch from ifconfig*/
    FILE *fp = NULL;
    char outValue[BUFFER_SZ] ={0};
    char *endptr = NULL;
    char *rx_str = NULL;
    char *tx_str = NULL;
    unsigned long long int rx;
    unsigned long long int tx;

    if (!(fp = v_secure_popen("r", "ifconfig %s | grep \"RX bytes\" | tr '(' '|' | tr ':' '|' | cut -d'|' -f2,4",ifname)))
    {
        write_to_logfile("%s:popen failed\n",__FUNCTION__);
        return FAILURE;
    }

    while (fgets(outValue, BUFFER_SZ, fp)!=NULL)
    {
        size_t len = strlen(outValue);
        if (len > 0 && outValue[len-1] == '\n') {
            outValue[len-1] = '\0';
        }
    }

    if ( outValue[0] != '\0' )
    {
        rx_str= strtok(outValue, "|");
        /* popen output has space in rxbytes,remove it*/
        if (rx_str && (rx_str[strlen(rx_str)-2] == ' '))
            rx_str[strlen(rx_str)-2] = '\0';
        tx_str= strtok(0,"|");
        if (rx_str && (rx=strtoull(rx_str,&endptr,10)) && endptr)
        {
            *rx_bytes = rx;
        }
        endptr = NULL;
        if (tx_str && (tx = strtoull(tx_str,&endptr,10)) && endptr)
        {
            *tx_bytes = tx;
        }
        ret = v_secure_pclose(fp);
        if(ret != 0)
        {
            write_to_logfile("Error in closing pipe ! : %d \n", ret);
            return FAILURE;
        }
        return SUCCESS;
    }
    else
    {
        ret=v_secure_pclose(fp);
        if (ret != 0)
           write_to_logfile("Error in closing pipe ! : %d \n", ret);
        return FAILURE;
    }
#endif
}

static int store_to_file(char *filename,char *bridge_name,uint64_t rx_bytes,uint64_t tx_bytes)
{
    FILE *pFile = fopen(filename, "a+");
    if (!pFile)
    {
        write_to_logfile("%s:fopen failed for file %s",__FUNCTION__,filename);
        return FAILURE;
    }
    fprintf(pFile,"%s:%" PRIu64 ",%" PRIu64 "\n",bridge_name,rx_bytes,tx_bytes);
    fclose(pFile);
    pFile = NULL;
    return SUCCESS;
}


static int read_from_file(char *filename,char *bridge_name,uint64_t *rx_bytes,uint64_t *tx_bytes)
{
    FILE *fp = NULL;
    char outValue[1024] ={0};

    char *endptr = NULL;
    char *rx_str = NULL;
    char *tx_str = NULL;
    unsigned long long int rx;
    unsigned long long int tx;

    int ret = 0;

    if (!(fp = v_secure_popen("r", "cat %s | grep %s | cut -d : -f2",filename,bridge_name)))
    {
        write_to_logfile("%s:popen failed\n",__FUNCTION__);
        return FAILURE;
    }

    while (fgets(outValue, 1024, fp)!=NULL)
    {
        size_t len = strlen(outValue);
        if (len > 0 && outValue[len-1] == '\n') {
            outValue[len-1] = '\0';
        }
    }

    if ( outValue[0] != '\0' )
    {
        rx_str= strtok(outValue, ",");
        tx_str= strtok(0,",");
        if (rx_str && (rx=strtoull(rx_str,&endptr,10)) && endptr)
        {
            *rx_bytes = rx;
        }
        endptr = NULL;
        if (tx_str && (tx = strtoull(tx_str,&endptr,10)) && endptr)
        {
            *tx_bytes = tx;
        }
    }
    else
    {
        return FAILURE;
    }

    ret = v_secure_pclose(fp);
    if(ret != 0)
    {
        write_to_logfile("Error in closing pipe ! : %d \n", ret);
        return FAILURE;
    }
    return SUCCESS;
}

char *get_bridge_map(char *bridge_name)
{
    int i;
    errno_t rc = -1;
    int ind = -1;
    for (i=0;i < BridgeMap_TOTAL;i++)
    {
        rc = strcmp_s( bridge_name,strlen(bridge_name),BridgeMap[i].iface, &ind);
        ERR_CHK(rc);
        if((!ind) && (rc == EOK))
        {
            return BridgeMap[i].feature;
        }

    }

    /* We are out of loop, we couldn't find any matching map, as best as
    we can print bridge name itself, let other team frame markers based on
    that*/
    write_to_logfile("Couldn't find bridge map,taking interface name:%s\n", bridge_name);
    return bridge_name;
}


void write_to_logfile(const char *fmt, ...)
{
    time_t ltime;
    char utcstring[256] = {0};
    struct tm time_info = {0};
    va_list args;

    if (log_fd) {
        ltime = time(NULL);
        localtime_r(&ltime, &time_info);
        strftime(utcstring, 128, "%Y-%m-%d %T", &time_info);
        if (utcstring[strlen(utcstring) - 1] == '\n')
            utcstring[strlen(utcstring) - 1] = '\0';
        va_start(args, fmt);
        fprintf(log_fd, "%s : ", utcstring);
        vfprintf(log_fd, fmt, args);
        va_end(args);
        fflush(log_fd);
    }
    return;
}
