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

/**********************************************************************

    module: ssp_messagebus_interface.c

        For Test and Diagnostic module

    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc., 1997 ~ 2005
        All Rights Reserved.

    ---------------------------------------------------------------

    description:

        SSP implementation of the CCSP Message Bus Interface
        Service.

        *   ssp_TadMbi_MessageBusEngage
        *   ssp_TadMbi_EventCallback

    ---------------------------------------------------------------

    environment:

        Embedded Linux

    ---------------------------------------------------------------

    author:

        Tom Chang

    ---------------------------------------------------------------

    revision:

        06/23/2011  initial revision.

**********************************************************************/

#include "ssp_global.h"


ANSC_HANDLE                 bus_handle         = NULL;
extern ANSC_HANDLE          g_MessageBusHandle_Irep;
extern char                 g_SubSysPrefix_Irep[32];
extern char                 g_Subsystem[32];
extern  BOOL                g_bActive;

#ifdef _ANSC_LINUX
ANSC_STATUS
ssp_TadMbi_MessageBusEngage
    (
        char * component_id,
        char * config_file,
        char * path
    )
{
    ANSC_STATUS                 returnStatus       = ANSC_STATUS_SUCCESS;
    CCSP_Base_Func_CB           cb                 = {0};

    if ( ! component_id || ! path )
    {
        CcspTraceError((" !!! ssp_TadMbi_MessageBusEngage: component_id or path is NULL !!!\n"));
    }

    /* Connect to message bus */
    returnStatus =
        CCSP_Message_Bus_Init
            (
                component_id,
                config_file,
                &bus_handle,
                Ansc_AllocateMemory_Callback,           /* mallocfc, use default */
                Ansc_FreeMemory_Callback            /* freefc,   use default */
            );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        CcspTraceError((" !!! TAD Message Bus Init ERROR !!!\n"));

        return returnStatus;
    }
    ssp_TadMbi_WaitConditionReady(bus_handle, CCSP_DBUS_PSM, CCSP_DBUS_PATH_PSM, component_id);
    CcspTraceInfo(("!!! Connected to message bus... bus_handle: 0x%08X !!!\n", bus_handle));
    g_MessageBusHandle_Irep = bus_handle;
    AnscCopyString(g_SubSysPrefix_Irep, g_Subsystem);

    CCSP_Msg_SleepInMilliSeconds(1000);

    /* Base interface implementation that will be used cross components */
    cb.getParameterValues     = CcspCcMbi_GetParameterValues;
    cb.setParameterValues     = CcspCcMbi_SetParameterValues;
    cb.setCommit              = CcspCcMbi_SetCommit;
    cb.setParameterAttributes = CcspCcMbi_SetParameterAttributes;
    cb.getParameterAttributes = CcspCcMbi_GetParameterAttributes;
    cb.AddTblRow              = CcspCcMbi_AddTblRow;
    cb.DeleteTblRow           = CcspCcMbi_DeleteTblRow;
    cb.getParameterNames      = CcspCcMbi_GetParameterNames;
    cb.currentSessionIDSignal = CcspCcMbi_CurrentSessionIdSignal;

    /* Base interface implementation that will only be used by tad */
    cb.initialize             = ssp_TadMbi_Initialize;
    cb.finalize               = ssp_TadMbi_Finalize;
    cb.freeResources          = ssp_TadMbi_FreeResources;
    cb.busCheck               = ssp_TadMbi_Buscheck;

    /*Componet Health*/
    cb.getHealth              = ssp_TadMbi_GetHealth;

    CcspBaseIf_SetCallback(bus_handle, &cb);


    /* Register service callback functions */
    returnStatus =
        CCSP_Message_Bus_Register_Path
            (
                bus_handle,
                path,
                CcspTandD_path_message_func,
                bus_handle
            );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Path ERROR returnStatus: %d\n!!!\n", returnStatus));

        return returnStatus;
    }


    /* Register event/signal */
    returnStatus =
        CcspBaseIf_Register_Event
            (
                bus_handle,
                0,
                "currentSessionIDSignal"
            );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Event: CurrentSessionIDSignal ERROR returnStatus: %d!!!\n", returnStatus));

        return returnStatus;
    }

    return ANSC_STATUS_SUCCESS;
}

DBusHandlerResult
CcspTandD_path_message_func
    (
        DBusConnection  *conn,
        DBusMessage     *message,
        void            *user_data
    )
{
    CCSP_MESSAGE_BUS_INFO *bus_info =(CCSP_MESSAGE_BUS_INFO *) user_data;
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    DBusMessage *reply;
    reply = dbus_message_new_method_return (message);
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return CcspBaseIf_base_path_message_func
               (
                   conn,
                   message,
                   reply,
                   interface,
                   method,
                   bus_info
               );
}

#endif

int
ssp_TadMbi_Initialize
    (
        void * user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    printf("In %s()\n", __FUNCTION__);

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}

int
ssp_TadMbi_Finalize
    (
        void * user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    printf("In %s()\n", __FUNCTION__);

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}


int
ssp_TadMbi_Buscheck
    (
        void * user_data
    )
{
    printf("In %s()\n", __FUNCTION__);

    return 0;
}

int
ssp_TadMbi_FreeResources
    (
        int priority,
        void * user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    printf("In %s()\n", __FUNCTION__);

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}

