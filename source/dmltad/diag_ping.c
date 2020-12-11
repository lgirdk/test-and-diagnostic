/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/*
 * diag_ping.c - backend for IPPing
 * leichen2@cisco.com, Mar 2013, Initialize
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "diag_inter.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "safec_lib_common.h"

#include "plugin_main_apis.h"
extern  COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc;
extern  ANSC_HANDLE                         bus_handle;
#define PING_DEF_CNT        1
#define PING_DEF_TIMO       10
#define PING_DEF_SIZE       56
#define PING_DEF_DNS_QUERY_TYPE      1
#define PING_DEF_INTERVAL            1

static diag_err_t ping_start(diag_obj_t *diag, const diag_cfg_t *cfg, diag_stat_t *st);
static diag_err_t ping_stop(diag_obj_t *diag);
static diag_err_t ping_forcestop(diag_obj_t *diag);

static diag_obj_t diag_ping = {
    .mode       = DIAG_MD_PING,
    .state      = DIAG_ST_NONE,
    .mutex      = PTHREAD_MUTEX_INITIALIZER,
    .cfg        = {
        .cnt        = PING_DEF_CNT,
        .timo       = PING_DEF_TIMO,
        .size       = PING_DEF_SIZE,
        .pingdnsquerytype  = PING_DEF_DNS_QUERY_TYPE,
        .interval   = PING_DEF_INTERVAL,
    },
    .ops        = {
        .start      = ping_start,
        .stop       = ping_stop,
        .forcestop  = ping_forcestop,
    },
};

diag_obj_t *diag_ping_load(void)
{
    return &diag_ping;
}

static BOOL isDSLiteEnabled()
{
    ANSC_STATUS             retval  = ANSC_STATUS_FAILURE;
    parameterValStruct_t    param;
    char                    name[] = "Device.DSLite.InterfaceSetting.1.Status";
    char                    value[16] = {};
    int                     valSize = 16;
    int                     size = 16;

    param.parameterName  = name;
    param.parameterValue = value;
     retval = g_GetParamValueByPathNameProc(bus_handle, &param, &valSize);

    if ( retval == ANSC_STATUS_SUCCESS ) {
        if ( 0== strcmp(value, "Enabled") ) {
            return TRUE;
        }
    }
    return FALSE;
}
static int getIPbyInterfaceName(char *interface, char *ip)
{
    int fd;
    struct ifreq ifr;
    strcpy(ip, "0.0.0.0");
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return -1;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
        strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }
    close(fd);
    return 0;
}

static BOOL isIPv4Host(const char *host)
{
    struct addrinfo hints, *res, *rp;
    int errcode;
    BOOL isIpv4 = FALSE;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo (host, NULL, &hints, &res);
    if (errcode != 0) {
        return FALSE;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            isIpv4 = TRUE;
            break;
        }
    }

    if (res != NULL) {
        freeaddrinfo(res);
    }

    return isIpv4;
}

/*
 * Methods to implement "ping":
 *
 * 1. use built-in "ping" utility - esaiest way, but not compatible.
 * 2. porting open-source package and customization - best solution, 
 *    more compatible we can do it in the future (when ? emm.. )
 * 3. write own ICMP echo/response functions - NEVER DO IT.
 *    have to handle too may details incluing IPv4/IPv6 packet, and NDS.
 */
static diag_err_t ping_start(diag_obj_t *diag, const diag_cfg_t *cfg, diag_stat_t *st)
{
    char            cmd[1024];
    char            result[256];
    FILE            *fp;
    size_t          left;
    unsigned int    sent;
    int             copy;
    unsigned        cnt;
    errno_t         rc = -1;
    const char      *awkcmd = 
        " > /var/tmp/pinging.txt; cat /var/tmp/pinging.txt | awk -F'[ /]+' '/transmitted/ {SEND=$1; RECV=$4; }; "
        " /^ping:/ { print; exit } " /* capture ping error message */
        " /^round-trip/ { if ($5 == \"mdev\") { MIN=$7; AVG=$8; MAX=$9 } else { MIN=$6; AVG=$7; MAX=$8 } } "
        " /^rtt/ {MIN=$7; AVG=$8; MAX=$9 } "
        " END {print SEND, RECV, MIN, AVG, MAX}' 2>&1";

    assert(diag == &diag_ping);

    if (!cfg || !strlen(cfg->host) || !st)
        return DIAG_ERR_PARAM;

    cmd[0] = '\0', left = sizeof(cmd);
    if (cfg->cnt <= 0)
        cnt = PING_DEF_CNT; /* or never return */
    else
        cnt = cfg->cnt;

    if( cfg->pingdnsquerytype == 2)
    {
        left -= snprintf(cmd + strlen(cmd), left, "ping6 %s ", cfg->host);
    }
    else
    {
        left -= snprintf(cmd + strlen(cmd), left, "ping %s ", cfg->host);
    }

    if (isDSLiteEnabled() && cfg->pingdnsquerytype != 2 && isIPv4Host(cfg->host)) { 
        char ifip[16] = {};
        getIPbyInterfaceName("brlan0", ifip);
        left -= snprintf(cmd + strlen(cmd), left, "-I %s ", ifip);
    } else {
        if (strlen(cfg->ifname))
            left -= snprintf(cmd + strlen(cmd), left, "-I %s ", cfg->ifname);
    }

    if (cnt)
    {
        left -= sprintf_s(cmd + strlen(cmd), left, "-c %u ", cnt);
        if(left < EOK)
        {
            ERR_CHK(rc);
        }
    }
    if (cfg->size)
    {
        left -= sprintf_s(cmd + strlen(cmd), left, "-s %u ", cfg->size);
        if(left < EOK)
        {
            ERR_CHK(rc);
        }
    }
    if (cfg->timo)
    {
        left -= sprintf_s(cmd + strlen(cmd), left, "-W %u ", cfg->timo);
        if(left < EOK)
        {
            ERR_CHK(rc);
        }
    }
    if (cfg->interval)
    {
        left -= snprintf(cmd + strlen(cmd), left, "-i %u ", cfg->interval);
    }
#ifdef PING_HAS_QOS
    if (cfg->tos)
    {
        left -= sprintf_s(cmd + strlen(cmd), left, "-Q %u ", cfg->tos);
        if(left < EOK)
        {
            ERR_CHK(rc);
        }
    }
#endif

#if defined(_PLATFORM_TURRIS_)
    left -= sprintf_s(cmd + strlen(cmd), left, "'%s' ", cfg->host);
    if(left < EOK)
    {
        ERR_CHK(rc);
    }
#endif
    left -= sprintf_s(cmd + strlen(cmd), left, "2>&1 ");
    if(left < EOK)
    {
        ERR_CHK(rc);
    }

    if (left <= strlen(awkcmd) + 1)
        return DIAG_ERR_NOMEM;
    rc = sprintf_s(cmd + strlen(cmd), left, "%s ", awkcmd);
    if(rc < EOK)
    {
        ERR_CHK(rc);
    }

    fprintf(stderr, "%s: %s\n", __FUNCTION__, cmd);

    if ((fp = popen(cmd, "r")) == NULL)
        return DIAG_ERR_INTERNAL;

    if (fgets(result, sizeof(result), fp) == NULL) {
        pclose(fp);
        return DIAG_ERR_OTHER;
    }

    fprintf(stderr, "%s: result: %s\n", __FUNCTION__, result);

    rc = memset_s(st, sizeof(*st), 0, sizeof(*st));
    ERR_CHK(rc);
    copy = sscanf(result, "%u %u %f %f %f", 
            &sent, &st->u.ping.success, &st->u.ping.rtt_min, 
            &st->u.ping.rtt_avg, &st->u.ping.rtt_max);

    if (strstr(result, "ping: unknown host") != NULL
            || strstr(result, "ping: bad address") != NULL) {
        pclose(fp);
        return DIAG_ERR_RESOLVE;
    }

    if((sent > 0) && (st->u.ping.success == 0)) {
        st->u.ping.failure = sent - st->u.ping.success;
        pclose(fp);
        return DIAG_ERR_OTHER;
    }

    if (copy == 5 || copy == 2) { /* RTT may not exist */
        st->u.ping.failure = sent - st->u.ping.success;
        pclose(fp);
        return DIAG_ERR_OK;
    }

    pclose(fp);
    return DIAG_ERR_OTHER;
}

static diag_err_t ping_stop(diag_obj_t *diag)
{
    assert(diag == &diag_ping);

    if (system("killall ping >/dev/null 2>&1") != 0)
        return DIAG_ERR_INTERNAL;
    return DIAG_ERR_OK;
}

static diag_err_t ping_forcestop(diag_obj_t *diag)
{
    assert(diag == &diag_ping);

    if (system("killall -9 ping >/dev/null 2>&1") != 0)
        return DIAG_ERR_INTERNAL;
    return DIAG_ERR_OK;
}
