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

#define PING_DEF_CNT        1
#define PING_DEF_TIMO       10
#define PING_DEF_SIZE       56

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
    const char      *awkcmd = 
        " | awk -F'[ /]+' '/transmitted/ {SEND=$1; RECV=$4; }; "
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

    left -= snprintf(cmd + strlen(cmd), left, "ping %s ", cfg->host);
    if (strlen(cfg->ifname))
        left -= snprintf(cmd + strlen(cmd), left, "-I %s ", cfg->ifname);
    if (cnt)
        left -= snprintf(cmd + strlen(cmd), left, "-c %u ", cnt);
    if (cfg->size)
        left -= snprintf(cmd + strlen(cmd), left, "-s %u ", cfg->size);
    if (cfg->timo)
        left -= snprintf(cmd + strlen(cmd), left, "-W %u ", cfg->timo);
#ifdef PING_HAS_QOS
    if (cfg->tos)
        left -= snprintf(cmd + strlen(cmd), left, "-Q %u ", cfg->tos);
#endif
    left -= snprintf(cmd + strlen(cmd), left, "2>&1 ");

    if (left <= strlen(awkcmd) + 1)
        return DIAG_ERR_NOMEM;
    snprintf(cmd + strlen(cmd), left, "%s ", awkcmd);

    fprintf(stderr, "%s: %s\n", __FUNCTION__, cmd);

    if ((fp = popen(cmd, "r")) == NULL)
        return DIAG_ERR_INTERNAL;

    if (fgets(result, sizeof(result), fp) == NULL) {
        pclose(fp);
        return DIAG_ERR_OTHER;
    }

    fprintf(stderr, "%s: result: %s\n", __FUNCTION__, result);

    memset(st, 0, sizeof(*st));
    copy = sscanf(result, "%u %u %f %f %f", 
            &sent, &st->u.ping.success, &st->u.ping.rtt_min, 
            &st->u.ping.rtt_avg, &st->u.ping.rtt_max);
    if (copy == 5 || copy == 2) { /* RTT may not exist */
        st->u.ping.failure = sent - st->u.ping.success;
        pclose(fp);
        return DIAG_ERR_OK;
    }

    if (strstr(result, "ping: unknown host") != NULL
            || strstr(result, "ping: bad address") != NULL) {
        pclose(fp);
        return DIAG_ERR_RESOLVE;
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
