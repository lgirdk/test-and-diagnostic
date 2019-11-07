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
 * diag.c - diagnostics API implementation.
 * leichen2@cisco.com, Mar 2013, Initialize
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include "diag_inter.h"

/* XXX: if there are more instances, we may use a dynamic list to 
 * handle these instances, or with dynamic load. */

extern diag_obj_t *diag_ping_load(void);
extern diag_obj_t *diag_tracert_load(void);

static diag_obj_t *diag_ping;
static diag_obj_t *diag_tracert;
diag_pingtest_stat_t diag_pingtest_stat;

static void trim(char *line)
{
    char *cp;

    if (!line || !strlen(line))
        return;

    for (cp = line + strlen(line) - 1; cp >= line && isspace(*cp); cp--)
        *cp = '\0';

    for (cp = line; *cp && isspace(*cp); cp++)
        ;

    if (cp > line)
        memmove(line, cp, strlen(cp) + 1);

    return;
}

static int is_empty(const char *line)
{
    while (*line++)
        if (!isspace(*line))
            return 0;
    return 1;
}

/* addr is in network order */
static int inet_is_same_net(uint32_t addr1, uint32_t addr2, uint32_t mask)
{
    return (addr1 & mask) == (addr2 & mask);
}

static int inet_is_onlink(const struct in_addr *addr)
{
    char cmd[1024];
    char table[1024];
    char entry[1024];
    FILE *rule_fp;
    FILE *tbl_fp;
    char *pref;
    struct in_addr net;
    uint32_t mask;

    if (!addr)
        return 0;

    /*  
     * let's just lookup the duplicated table,
     * btw, if we "sort | uniq" it the priory is changed
     */
    snprintf(cmd, sizeof(cmd), "ip rule show | sed -n 's/.*\\<lookup\\> \\(.*\\)/\\1/p'");
    if ((rule_fp = popen(cmd, "r")) == NULL)
        return 0;

    while (fgets(table, sizeof(table), rule_fp)) {
        trim(table);
        if (is_empty(table) || strcmp(table, "local") == 0)
            continue;

        snprintf(cmd, sizeof(cmd), "ip route show table %s | awk '/\\<link\\>/ {print $1}'", table);
        if ((tbl_fp = popen(cmd, "r")) == NULL) {
            pclose(rule_fp);
            return 0;
        }

        while (fgets(entry, sizeof(entry), tbl_fp)) {
            trim(entry);
            if (is_empty(entry))
                continue;

            if ((pref = strchr(entry, '/')) == NULL)
                continue;
            *pref++ = '\0';

            if (inet_pton(AF_INET, entry, &net) <= 0)
                continue;

            mask = htonl(0xffffffff << (32 - atoi(pref)));

            if (inet_is_same_net(*(uint32_t *)(addr), *(uint32_t *)&net, mask)) {
                pclose(tbl_fp);
                pclose(rule_fp);
                return 1; /* on-link */
            }
        }

        pclose(tbl_fp);
    }

    pclose(rule_fp);
    return 0;
}

static int inet_is_local(const struct in_addr *addr)
{
    char cmd[1024];
    char entry[1024];
    FILE *fp;
    char *tok, *delim = " \t\r\n", *sp;
    char *dest, *preflen;
    struct in_addr inaddr;

    if (!addr)
        return 0;

    /* 
     * lookup local table to check:
     * 1. broadcast to local network 'broadcast xxx.xxx.xxx.xxx'
     * 2. unicast to address of local device, 'local xxx.xxx.xxx.xxx'
     * 3. unicast to address in same network e.g,. "xxx.xxx.xxx.xxx/xx" 
     */
    snprintf(cmd, sizeof(cmd), "ip route show table local");
    if ((fp = popen(cmd, "r")) == NULL)
        return 0;

    while (fgets(entry, sizeof(entry), fp) != NULL) {
        trim(entry);

        if ((tok = strtok_r(entry, delim, &sp)) == NULL)
            continue;
        if ((dest = strtok_r(NULL, delim, &sp)) == NULL)
            continue;

        if ((preflen = strchr(dest, '/')) != NULL)
            *preflen++ = '\0';
        if (inet_pton(AF_INET, dest, &inaddr) <= 0)
            continue;

        /* check 'broadcast' and 'local' for save */
        if (strcmp(tok, "broadcast") == 0 
                || (strcmp(tok, "local") == 0 && !preflen)) {
            if (*(uint32_t *)&inaddr == *(uint32_t *)addr) {
                pclose(fp);
                return 1;
            }
        } else if (strcmp(tok, "local") == 0 && preflen) {
            if (inet_is_same_net(*(uint32_t *)addr, *(uint32_t *)&inaddr, 
                        htonl(0xFFFFFFFF << (32 - atoi(preflen))))) {
                pclose(fp);
                return 1;
            }
        } else {
            continue; // never got here actually
        }
    }

    pclose(fp);
    return 0;
}
static bool is_ipv6_same(const struct in6_addr *addr1,
        const struct in6_addr *addr2)
{
	int c1, c2, c3, c4;
    c1 = addr1->s6_addr32[0] ^ addr2->s6_addr32[0];
    c2 = addr1->s6_addr32[1] ^ addr2->s6_addr32[1];
    c3 = addr1->s6_addr32[2] ^ addr2->s6_addr32[2];
    c4 = addr1->s6_addr32[3] ^ addr2->s6_addr32[3];
	return ((c1 | c2 | c3 | c4) == 0);
}
static bool is_prefix_equal(const struct in6_addr *addrs1,
        const struct in6_addr *addrs2,
        unsigned int prelen)
{
    const uint32_t *a1 = addrs1->s6_addr32;
    const uint32_t *a2 = addrs2->s6_addr32;
    unsigned int pcmu, pinu;

    /* check complete u32 in prefix */
    pcmu = prelen >> 5;
    if (pcmu && memcmp(a1, a2, pcmu << 2)) 
        return false;

    /* check incomplete u32 in prefix */
    pinu = prelen & 0x1f;
    if (pinu && ((a1[pcmu] ^ a2[pcmu]) & htonl((0xffffffff) << (32 - pinu))))
        return false;

    return true;
}

/* 
 * to check if next hop of the addrs is default route ?
 * entry output
 *   local <addr> ...
 *   <addr>/<pref> ...
 *   <addr> ...
 * e.g.,
 *   local fe80::2676:7dff:feff:e16d via :: dev lo  proto none  metric 0 
 *   ff00::/8 dev l2sm0  metric 256 
 */
static bool inet6_nexthop_not_def(const struct in6_addr *addr)
{
    /* look up all tables */
    FILE *rule_fp = NULL, *tbl_fp = NULL;
    char cmd[128], table[512], entry[512];
    char *dest, *delim = " \t\r\n", *sp, *preflen;
    struct in6_addr daddr;

    rule_fp = popen("ip -6 rule show | sed -n 's/.*\\<lookup\\> \\(.*\\)/\\1/p'", "r");
    if (!rule_fp)
        return false;

    while (fgets(table, sizeof(table), rule_fp)) {
        trim(table);
        if (is_empty(table))
            continue;

        snprintf(cmd, sizeof(cmd), "ip -6 route show table %s", table);
        if ((tbl_fp = popen(cmd, "r")) == NULL)
            break;

        while (fgets(entry, sizeof(entry), tbl_fp)) {
            trim(entry);
            if (is_empty(entry))
                continue;

            if (!(dest = strtok_r(entry, delim, &sp)))
                continue;
            if (strcmp(dest, "default") == 0)
                continue;
            if (strcmp(dest, "local") == 0)
                if (!(dest = strtok_r(NULL, delim, &sp)))
                    continue;

            if ((preflen = strchr(dest, '/')) != NULL)
                *preflen++ = '\0';

            if (inet_pton(AF_INET6, dest, &daddr) <= 0)
                continue;

            if (!preflen) {
                if (is_ipv6_same(addr, &daddr)) {
                    pclose(tbl_fp);
                    pclose(rule_fp);
                    return true;
                }
            } else {
                if (is_prefix_equal(addr, &daddr, atoi(preflen))) {
                    pclose(tbl_fp);
                    pclose(rule_fp);
                    return true;
                }
            }
        }

        pclose(tbl_fp);
    }

    pclose(rule_fp);
    return false;
}

static const char *assign_iface(const char *host, char *buf, size_t size)
{
    struct in_addr inaddr;
    struct in6_addr in6addr;

    if (inet_pton(AF_INET, host, &inaddr) > 0) {
        if (inet_is_local(&inaddr))
            return NULL;
        if (inet_is_onlink(&inaddr))
            return NULL;
    } else if (inet_pton(AF_INET6, host, &in6addr) > 0) {
        if (inet6_nexthop_not_def(&in6addr))
            return NULL;
    }

    //snprintf(buf, size, "%s", "erouter0");//LNT_EMU
    snprintf(buf, size, "%s", "eth0");
    return buf;
}

static diag_obj_t *get_diag_by_mode(diag_mode_t mode)
{
    switch (mode) {
    case DIAG_MD_PING:
        return diag_ping;
    case DIAG_MD_TRACERT:
        return diag_tracert;
    default:
        return NULL;
    }
}

static diag_err_t __diag_stop(diag_obj_t *diag)
{
#ifdef _GNU_SOURCE
    struct timespec timo;
#endif

    diag->op_stop(diag);
    pthread_mutex_unlock(&diag->mutex);

#ifdef _GNU_SOURCE
    timo.tv_sec = 3;
    timo.tv_nsec = 0;
    if (pthread_timedjoin_np(diag->task, NULL, &timo) != 0) {
        fprintf(stderr, "%s: not return, force stop it\n", __FUNCTION__);
        pthread_mutex_lock(&diag->mutex);
        if (diag->op_forcestop)
            diag->op_forcestop(diag);
        pthread_mutex_unlock(&diag->mutex);

        pthread_cancel(diag->task);
        pthread_join(diag->task, NULL);
    }
#else
    pthread_join(diag->task, NULL);
#endif
    pthread_mutex_lock(&diag->mutex);
    diag->state = DIAG_ST_NONE;

    return DIAG_ERR_OK;
}

static void *diag_task(void *arg)
{
    diag_obj_t  *diag = arg;
    diag_cfg_t  cfg;
    diag_stat_t stat;
    diag_err_t  err;
    char        buf[IFNAMSIZ];
    diag_cfg_t  cfgtemp;
    int retrycount = 0;

    if (!diag)
        return NULL;

    pthread_mutex_lock(&diag->mutex);
    cfg = diag->cfg;
    pthread_mutex_unlock(&diag->mutex);
    int len = strlen(cfg.host);
    if( (cfg.host[0] == '\'') && (cfg.host[len-1] == '\'') )
    {
        memmove(cfg.host,&cfg.host[1],len-2);
        cfg.host[len-2]='\0';
    }    
    /**
     * XXX: work around for dual WAN issue.
     * We have two WAN default route, one for wan0 another for erouter0.
     * if wan0 is not connect to Internel and erouter0 is.
     * ping LAN/WAN must also be OK. 
     *
     * consider if wan0 is connected to Internet and no -I is OK.
     * but LAN user cannot access Internet since.
     *
     * wo have policy route for traffic from LAN (direct to erotuer0)
     * but not for localout traffic (ping).
     */
    if (!strlen(cfg.ifname) && assign_iface(cfg.host, buf, sizeof(buf))) {
        snprintf(cfg.ifname, sizeof(cfg.ifname), "%s", buf);
        fprintf(stderr, "%s: Changing ifname to %s !!!!\n", __FUNCTION__, buf);
    }
/*If Diagstate comes first and wait for daig params to set*/
   if(!strlen(cfg.host))
    {
      do
      {
        sleep(1);
        diag_getcfg(DIAG_MD_PING, &cfgtemp);
        cfg = cfgtemp;
        if(strlen(cfg.host))
            break;
        else
        retrycount++;
      }while(retrycount == 5);

    }

    err = diag->op_start(diag, &cfg, &stat);

    fprintf(stderr, "%s: op_start %d\n", __FUNCTION__, err);

    pthread_mutex_lock(&diag->mutex);
    switch (err) {
    case DIAG_ERR_OK:
        diag->state = DIAG_ST_COMPLETE;
        diag->stat = stat;
        diag->err = DIAG_ERR_OK;
        break;
    default:
        diag->state = DIAG_ST_ERROR;
        if (err >= DIAG_ERR_RESOLVE)
            diag->err = err;
        else
            diag->err = DIAG_ERR_OTHER;
        break;
    }
    pthread_mutex_unlock(&diag->mutex);

    return NULL;
}

diag_err_t diag_init(void)
{

	//diagnositic PING test initialization
	diag_pingtest_init( );

    if ((diag_ping = diag_ping_load()) == NULL)
        goto errout;

    if ((diag_tracert = diag_tracert_load()) == NULL)
        goto errout;

    return DIAG_ERR_OK;

errout:
    // TODO:
    return DIAG_ERR_INTERNAL;
}

diag_err_t diag_term(void)
{
    // TODO:
    diag_ping = NULL;
    diag_tracert = NULL;
    return DIAG_ERR_OK;
}

diag_err_t diag_start(diag_mode_t mode)
{
    diag_obj_t *diag = get_diag_by_mode(mode);

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    /* need recyle even DIAG_ST_COMPLETE/ERROR */
    if (diag->state != DIAG_ST_NONE) {
        if (__diag_stop(diag) != DIAG_ERR_OK) {
            pthread_mutex_unlock(&diag->mutex);
            fprintf(stderr, "%s: fail to stop acting diag\n", __FUNCTION__);
            return DIAG_ERR_INTERNAL;
        }
    }

    if (diag->op_clearstatis)
        diag->op_clearstatis(diag);
    memset(&diag->stat, 0, sizeof(diag->stat));

    if (pthread_create(&diag->task, NULL, diag_task, diag) != 0) {
        diag->state = DIAG_ST_ERROR;
        diag->err = DIAG_ERR_INTERNAL;
        pthread_mutex_unlock(&diag->mutex);
        fprintf(stderr, "%s: fail to start diag task\n", __FUNCTION__);
        return DIAG_ERR_INTERNAL;
    }

    diag->state = DIAG_ST_ACTING;
    pthread_mutex_unlock(&diag->mutex);
    return DIAG_ERR_OK;
}

diag_err_t diag_stop(diag_mode_t mode)
{
    diag_obj_t *diag = get_diag_by_mode(mode);
    diag_err_t err = DIAG_ERR_OK;

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    /* need recyle even DIAG_ST_COMPLETE/ERROR */
    if (diag->state != DIAG_ST_NONE)
        err = __diag_stop(diag);
    diag->state = DIAG_ST_NONE;
    pthread_mutex_unlock(&diag->mutex);

    if (err != DIAG_ERR_OK)
        fprintf(stderr, "%s: fail to stop diag\n", __FUNCTION__);
    return err;
}

diag_err_t diag_setcfg(diag_mode_t mode, const diag_cfg_t *cfg)
{
    diag_obj_t *diag = get_diag_by_mode(mode);
    diag_err_t err;

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
/*From TR-181, "If the ACS sets the value of this parameter to Requested, the CPE MUST initiate the corresponding diagnostic test. When writing, the only allowed value is Requested. To ensure the use of the proper test parameters (the writable parameters in this object), the test parameters MUST be set either prior to or at the same time as (in the same SetParameterValues) setting the DiagnosticsState to Requested."
Mamidi:If we stop the diag test here that is causing the actual test to stop  and assigning the diag state to none(if setparams comes with Diagstate and params comes in a sigle call). This logic is expecting that always set params like host, numberofRepetions... comes in one setparams call and Requested comes in a separate setparams calls.
*/
#if 0
    /* need recyle even DIAG_ST_COMPLETE/ERROR */
    if (diag->state != DIAG_ST_NONE) {
        if ((err = __diag_stop(diag)) != DIAG_ERR_OK) {
            pthread_mutex_unlock(&diag->mutex);
            fprintf(stderr, "%s: fail to stop actiing diag\n", __FUNCTION__);
            return err;
        }
    }
#endif

    memcpy(&diag->cfg, cfg, sizeof(diag_cfg_t));

    if (diag->op_clearstatis)
        diag->op_clearstatis(diag);
    memset(&diag->stat, 0, sizeof(diag->stat));

    diag->state = DIAG_ST_NONE;
    pthread_mutex_unlock(&diag->mutex);

    return DIAG_ERR_OK;
}

diag_err_t diag_getcfg(diag_mode_t mode, diag_cfg_t *cfg)
{
    diag_obj_t *diag = get_diag_by_mode(mode);

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    memcpy(cfg, &diag->cfg, sizeof(diag_cfg_t));
    pthread_mutex_unlock(&diag->mutex);

    return DIAG_ERR_OK;
}

diag_err_t diag_getstatis(diag_mode_t mode, diag_stat_t *stat)
{
    diag_obj_t *diag = get_diag_by_mode(mode);

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    *stat = diag->stat;
    pthread_mutex_unlock(&diag->mutex);

    return DIAG_ERR_OK;
}

diag_err_t diag_getstate(diag_mode_t mode, diag_state_t *state)
{
    diag_obj_t *diag = get_diag_by_mode(mode);

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    *state = diag->state;
    pthread_mutex_unlock(&diag->mutex);

    return DIAG_ERR_OK;
}

diag_err_t diag_geterr(diag_mode_t mode, diag_err_t *error)
{
    diag_obj_t *diag = get_diag_by_mode(mode);

    if (!diag)
        return DIAG_ERR_PARAM;

    pthread_mutex_lock(&diag->mutex);
    *error = diag->err;
    pthread_mutex_unlock(&diag->mutex);

    return DIAG_ERR_OK;
}
diag_err_t diag_init_blocksize(void)
{
  diag_cfg_t                      cfg;
	char buf[10];
    if (diag_getcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return DIAG_ERR_PARAM;
        
	syscfg_init();
	memset(buf,0,sizeof(buf));
	syscfg_get( NULL, "selfheal_ping_DataBlockSize", buf, sizeof(buf));
	cfg.size = atoi(buf);

    if (diag_setcfg(DIAG_MD_PING, &cfg) != DIAG_ERR_OK)
        return DIAG_ERR_PARAM;
        
        return DIAG_ERR_OK;
}

diag_err_t diag_pingtest_init( void )
{
	diag_pingtest_device_details_t *pingtest_devdet = diag_pingtest_getdevicedetails( );

	/* validation */
	if( NULL == pingtest_devdet )
	{
		return DIAG_ERR_PARAM;
	}

	//PartnerID
	diag_getPartnerID( pingtest_devdet->PartnerID );
	
	//ecmMAC
	pingtest_devdet->ecmMAC[ 0 ] = '\0';

	//Device ID
	pingtest_devdet->DeviceID[ 0 ] = '\0';
	
	//Device Model
	pingtest_devdet->DeviceModel[ 0 ] = '\0';

	return DIAG_ERR_OK;
}

diag_pingtest_device_details_t* diag_pingtest_getdevicedetails(void)
{
    return ( &diag_pingtest_stat.device_details);
}

#define DEVICE_PROPERTIES    "/etc/device.properties"
diag_err_t diag_getPartnerID( char *partnerID )
{
	if( NULL == partnerID )
	{
		return DIAG_ERR_OTHER;
	}
	else
	{
		FILE	*deviceFilePtr;
		char	*pBldTypeStr		= NULL;
		char	 fileContent[ 255 ] = { '\0' };
		int 	 offsetValue		= 0;
		deviceFilePtr = fopen( DEVICE_PROPERTIES, "r" );
		
		// changed from default "comcast" to "comcast"
		sprintf( partnerID, "%s", "comcast" );

		if ( deviceFilePtr ) 
		{
			while ( fscanf( deviceFilePtr , "%s", fileContent ) != EOF ) 
			{
				if ( ( pBldTypeStr = strstr( fileContent, "PARTNER_ID" ) ) != NULL ) 
				{
					offsetValue = strlen( "PARTNER_ID=" );
					pBldTypeStr = pBldTypeStr + offsetValue ;
					break;
				}
			}
			
			fclose( deviceFilePtr );
		
			if( pBldTypeStr )
			{
				sprintf( partnerID, "%s", pBldTypeStr );
			}
		}
	}
	
	return DIAG_ERR_OK;
}
