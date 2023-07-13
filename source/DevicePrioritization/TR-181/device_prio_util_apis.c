/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syscfg/syscfg.h"
#include "safec_lib_common.h"
#include "device_prio_rbus_handler_apis.h"
#include "device_prio_apis.h"

extern rbusHandle_t g_rbusHandle;


char const* getParamName(char const* path)
{
    char const* p = path + strlen(path);
    while(p > path && *(p-1) != '.')
        p--;
    return p;
}

char* getDevicePrioParamName(devicePrioParam_t param) {
    switch(param) {
		case DP_QOS_ACTIVE_RULES:
			return DM_DSCP_CONTROL_ACTIVE_RULES;
        default:
            return NULL;
    }
}

/*** APIs for publishing event ***/	
rbusError_t 
DevicePrio_PublishToEvent
	(
		char* event_name, 
		char* eventData 
	)
{
    int ret = RBUS_ERROR_BUS_ERROR ;
    rbusEvent_t event;
    rbusObject_t data;
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, eventData);
    rbusObject_Init(&data, NULL);
    rbusObject_SetValue(data, event_name, value);
    event.name = event_name;
    event.data = data;
    event.type = RBUS_EVENT_GENERAL;

	/* Process the event publish*/
	ret = rbusEvent_Publish(g_rbusHandle, &event);
	if(ret != RBUS_ERROR_SUCCESS)
	{
		if (ret == RBUS_ERROR_NOSUBSCRIBERS) {
			ret = RBUS_ERROR_SUCCESS;
			CcspTraceError(("%s: No subscribers found\n", __FUNCTION__));
		}
		else {
			CcspTraceError(("Unable to Publish event data %s  rbus error code : %d\n",event_name, ret));
		}
	}
	else
	{
		CcspTraceInfo(("%s : Publish to %s ret value is %d\n", __FUNCTION__,event_name,ret));
	}
    /* release rbus value and object variable */
    rbusValue_Release(value);    
    return ret;
}