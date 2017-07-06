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
 * diag_traceroute.c - backend for TraceRoute
 * leichen2@cisco.com, 1 Apr 2014, Initialize
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "diag_inter.h"

#define TRACERT_DEF_CNT     1
#define TRACERT_DEF_TIMO    3
#define TRACERT_DEF_SIZE    38
#define TRACERT_DEF_MAXHOP  30

static diag_err_t tracert_start(diag_obj_t *diag, const diag_cfg_t *cfg, diag_stat_t *stat);
static diag_err_t tracert_stop(diag_obj_t *diag);
static diag_err_t tracert_forcestop(diag_obj_t *diag);
static diag_err_t tracert_clearstatis(diag_obj_t *diag);

static diag_obj_t diag_tracert = {
    .mode       = DIAG_MD_TRACERT,
    .state      = DIAG_ST_NONE,
    .mutex      = PTHREAD_MUTEX_INITIALIZER,
    .cfg        = {
        .cnt    = TRACERT_DEF_CNT,
        .timo   = TRACERT_DEF_TIMO,
        .size   = TRACERT_DEF_SIZE,
        .maxhop = TRACERT_DEF_MAXHOP,
    },
    .ops        = {
        .start      = tracert_start,
        .stop       = tracert_stop,
        .forcestop  = tracert_forcestop,
        .clearstatis= tracert_clearstatis,
    },
};

/*
 * 72.872 ms  72.517 ms  71.808 ms
 * 72.311 ms  71.855 ms  71.536 ms
 * 72.243 ms 209.85.243.156 (209.85.243.156)  72.803 ms 209.85.243.158 (209.85.243.158)  71.727 ms
 *
 * for the third case the response comes from different nodes
 * as TR181 do not support that, let's discard the "address(name)" info.
 */
static void convert_rtts(char *rtts, size_t size)
{
    char *tok1, *tok2, *delim = " \t\r\n", *sp;
    char *start, buf[1024];
    float f;

    buf[0] = '\0';
    for (start = rtts; 
            (tok1 = strtok_r(start, delim, &sp)) && (tok2 = strtok_r(NULL, delim, &sp));
            start = NULL) {
        if (strcmp(tok2, "ms") == 0) {
            sscanf(tok1, "%f", &f);
            if (start)
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d", (int)f);
            else
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ",%d", (int)f);
        } else if (tok2[0] == '(') {
            continue;
        } else {
            return; // bad format
        }   
    }   

    snprintf(rtts, size, "%s", buf);
}

diag_obj_t *diag_tracert_load(void)
{
    return &diag_tracert;
}

static diag_err_t tracert_start(diag_obj_t *diag, const diag_cfg_t *cfg, diag_stat_t *stat)
{
    char cmd[512];
    char line[512];
    size_t left;
    FILE *fp;
    diag_err_t err = DIAG_ERR_INTERNAL;

    assert(diag == &diag_tracert);

    cmd[0] = '\0', left = sizeof(cmd);
    
    left -= snprintf(cmd + strlen(cmd), left, "traceroute %s ", cfg->host);
    if (strlen(cfg->ifname))
        left -= snprintf(cmd + strlen(cmd), left, "-i %s ", cfg->ifname);
    if (cfg->cnt)
        left -= snprintf(cmd + strlen(cmd), left, "-q %u ", cfg->cnt);
    if (cfg->timo)
        left -= snprintf(cmd + strlen(cmd), left, "-w %u ", cfg->timo);
    if (cfg->tos)
        left -= snprintf(cmd + strlen(cmd), left, "-t %u ", cfg->tos);
    if (cfg->maxhop)
        left -= snprintf(cmd + strlen(cmd), left, "-m %u ", cfg->maxhop);
    if (cfg->size)
        left -= snprintf(cmd + strlen(cmd), left, "%u ", cfg->size);
    left -= snprintf(cmd + strlen(cmd), left, "2>&1 ");

    fprintf(stderr, "%s: %s\n", __FUNCTION__, cmd);

    if ((fp = popen(cmd, "r")) == NULL) {
        return DIAG_ERR_INTERNAL;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return DIAG_ERR_OTHER;
    }

    if (strncmp(line, "traceroute to", strlen("traceroute to")) != 0) {
        if (strstr(line, "Name or service not known") != NULL
                || strstr(line, "bad address") != NULL)
            err = DIAG_ERR_RESOLVE;
        else
            err = DIAG_ERR_OTHER;
    } else {
        unsigned nhop = 0;
        tracert_hop_t *hops = NULL;
        void *ptr;
        int idx;

        while (fgets(line, sizeof(line), fp) != NULL) {
            ptr = realloc(hops, (nhop + 1) * sizeof(tracert_hop_t));
            if (ptr == NULL) {
                if (hops) free(hops);
                fprintf(stderr, "%s: no memory\n", __FUNCTION__);
                return DIAG_ERR_NOMEM;
            }
            hops = ptr;

            /* 9  xxx.cisco.com (10.112.0.118)  40.469 ms  40.092 ms  40.528 ms */
            memset(&hops[nhop], 0, sizeof(tracert_hop_t));
            sscanf(line, "%d %s (%[^)]) %[^\n]", 
                    &idx, hops[nhop].host, hops[nhop].addr, hops[nhop].rtts);
            /* TR-181 doesn't like the format, let's convert it */
            convert_rtts(hops[nhop].rtts, sizeof(hops[nhop].rtts));
            hops[nhop].icmperr = 0; // TODO: we can use output: '!H, !S, ...'
            nhop++;
        }

        stat->u.tracert.nhop = nhop;
        stat->u.tracert.hops = hops;

        err = DIAG_ERR_OK;
    }

    fprintf(stderr, "> done: %d\n", err);
    {
        int i;
        tracert_hop_t *hop;

        fprintf(stderr, "nhop: %u resp %u\n", stat->u.tracert.nhop, stat->u.tracert.resptime);
        for (i = 0; i < stat->u.tracert.nhop; i++) {
            hop = &stat->u.tracert.hops[i];
            fprintf(stderr, "hop[%d]: host %s addr %s icmp %u rtts: %s\n", 
                    i, hop->host, hop->addr, hop->icmperr, hop->rtts);
        }
    }

    pclose(fp);
    return err;
}

static diag_err_t tracert_stop(diag_obj_t *diag)
{
    assert(diag == &diag_tracert);

    if (system("killall traceroute >/dev/null 2>&1") != 0)
        return DIAG_ERR_INTERNAL;
    return DIAG_ERR_OK;
}

static diag_err_t tracert_forcestop(diag_obj_t *diag)
{
    assert(diag == &diag_tracert);

    if (system("killall -9 traceroute >/dev/null 2>&1") != 0)
        return DIAG_ERR_INTERNAL;
    return DIAG_ERR_OK;
}

static diag_err_t tracert_clearstatis(diag_obj_t *diag)
{
    assert(diag == &diag_tracert);

    if (diag->stat.u.tracert.hops)
        free(diag->stat.u.tracert.hops);
    diag->stat.u.tracert.hops = NULL;
    diag->stat.u.tracert.nhop = 0;
    diag->stat.u.tracert.resptime = 0;

    return DIAG_ERR_OK;
}
