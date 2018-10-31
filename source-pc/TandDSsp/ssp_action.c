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

    module: ssp_action.c

        For CCSP Test and Diagnostic Module

    ---------------------------------------------------------------

    copyright:

        Cisco Systems, Inc., 1997 ~ 2005
        All Rights Reserved.

    ---------------------------------------------------------------

    description:

        SSP implementation of the Test and Diagnostic module.

        *   ssp_create_tad
        *   ssp_engage_tad
        *   ssp_cancel_tad
        *   ssp_TadCCDmGetComponentName
        *   ssp_TadCCDmGetComponentVersion
        *   ssp_TadCCDmGetComponentAuthor
        *   ssp_TadCCDmGetComponentHealth
        *   ssp_TadCCDmGetComponentState
        *   ssp_TadCCDmGetLoggingEnabled
        *   ssp_TadCCDmSetLoggingEnabled
        *   ssp_TadCCDmGetLoggingLevel
        *   ssp_TadCCDmSetLoggingLevel
        *   ssp_TadCCDmGetMemMaxUsage
        *   ssp_TadCCDmGetMemMinUsage
        *   ssp_TadCCDmGetMemConsumed

    ---------------------------------------------------------------

    environment:

        Embedded Linux

    ---------------------------------------------------------------

    author:

        Tom Chang

    ---------------------------------------------------------------

    revision:

        06/15/2011  initial revision.

**********************************************************************/

#include "ssp_global.h"
#include "ccsp_trace.h"
#include <time.h>
#include "cosa_plugin_api.h"

extern ULONG                            g_ulAllocatedSizePeak;

extern  PDSLH_CPE_CONTROLLER_OBJECT     pDslhCpeController;
extern  PDSLH_DATAMODEL_AGENT_OBJECT    g_DslhDataModelAgent;
extern  PCOMPONENT_COMMON_DM            g_pComponent_Common_Dm;
extern  PCCSP_FC_CONTEXT                 pTadFcContext;
extern  PCCSP_CCD_INTERFACE              pTadCcdIf;
extern  ANSC_HANDLE                     bus_handle;
extern  char                            g_Subsystem[32];

static  COMPONENT_COMMON_DM             CommonDm = {0};

#define  COSA_PLUGIN_XML_FILE           "/usr/ccsp/tad/TestAndDiagnostic.XML"

#if defined(_ANSC_WINDOWSNT)
#define  COSA_DIAG_PLUGIN_LIBRARY_NAME             "bisga_diagnostic.dll"
#elif defined(_ANSC_LINUX)
#define  COSA_DIAG_PLUGIN_LIBRARY_NAME             "libdiagnostic.so"
#endif

#define  COSA_DIAG_PLUGIN_INIT_PROC                "COSA_Diag_Init"
#define  COSA_DIAG_PLUGIN_UNLOAD_PROC              "COSA_Diag_Unload"
#define  COSA_DIAG_PLUGIN_GETRESULTS_PROC          "COSA_Diag_GetResults"
#define  COSA_DIAG_PLUGIN_SCHEDIAG_PROC            "COSA_Diag_ScheduleDiagnostic"
#define  COSA_DIAG_PLUGIN_SETDIAGPARAMS_PROC       "COSA_Diag_SetDiagParams"
#define  COSA_DIAG_PLUGIN_SETDIAGSTATE_PROC        "COSA_Diag_SetDiagState"
#define  COSA_DIAG_PLUGIN_CANCELDIAG_PROC          "COSA_Diag_CancelDiagnostic"
#define  COSA_DIAG_PLUGIN_MEMORYUSAGE_PROC         "COSA_Diag_MemoryUsage"
#define  COSA_DIAG_PLUGIN_MEMORYTABLE_PROC         "COSA_Diag_MemoryTable"
#define  COSA_DIAG_PLUGIN_GETCONFIGS_PROC          "COSA_Diag_GetConfigs"
#define  COSA_DIAG_PLUGIN_EXPORTFUNC_PROC          "COSA_Diag_ExportFunc"

COSA_DIAG_PLUGIN_INFO                               g_CosaDiagPluginInfo;

COSAGetParamValueByPathNameProc     g_GetParamValueByPathNameProc   = NULL;


ANSC_STATUS
ssp_create_tad
    (
    )
{
    /* Create component common data model object */

    g_pComponent_Common_Dm = (PCOMPONENT_COMMON_DM)AnscAllocateMemory(sizeof(COMPONENT_COMMON_DM));

    if ( !g_pComponent_Common_Dm )
    {
        return ANSC_STATUS_RESOURCES;
    }

    ComponentCommonDmInit(g_pComponent_Common_Dm);

    g_pComponent_Common_Dm->Name     = AnscCloneString(CCSP_COMPONENT_NAME_TAD);
    g_pComponent_Common_Dm->Version  = 1;
    g_pComponent_Common_Dm->Author   = AnscCloneString("Tom Zhang");

    /* Create ComponentCommonDatamodel interface*/
    if ( !pTadCcdIf )
    {
        pTadCcdIf = (PCCSP_CCD_INTERFACE)AnscAllocateMemory(sizeof(CCSP_CCD_INTERFACE));

        if ( !pTadCcdIf )
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pTadCcdIf->Name, CCSP_CCD_INTERFACE_NAME);

            pTadCcdIf->InterfaceId              = CCSP_CCD_INTERFACE_ID;
            pTadCcdIf->hOwnerContext            = NULL;
            pTadCcdIf->Size                     = sizeof(CCSP_CCD_INTERFACE);

            pTadCcdIf->GetComponentName         = ssp_TadCCDmGetComponentName;
            pTadCcdIf->GetComponentVersion      = ssp_TadCCDmGetComponentVersion;
            pTadCcdIf->GetComponentAuthor       = ssp_TadCCDmGetComponentAuthor;
            pTadCcdIf->GetComponentHealth       = ssp_TadCCDmGetComponentHealth;
            pTadCcdIf->GetComponentState        = ssp_TadCCDmGetComponentState;
            pTadCcdIf->GetLoggingEnabled        = ssp_TadCCDmGetLoggingEnabled;
            pTadCcdIf->SetLoggingEnabled        = ssp_TadCCDmSetLoggingEnabled;
            pTadCcdIf->GetLoggingLevel          = ssp_TadCCDmGetLoggingLevel;
            pTadCcdIf->SetLoggingLevel          = ssp_TadCCDmSetLoggingLevel;
            pTadCcdIf->GetMemMaxUsage           = ssp_TadCCDmGetMemMaxUsage;
            pTadCcdIf->GetMemMinUsage           = ssp_TadCCDmGetMemMinUsage;
            pTadCcdIf->GetMemConsumed           = ssp_TadCCDmGetMemConsumed;
            pTadCcdIf->ApplyChanges             = ssp_TadCCDmApplyChanges;
        }
    }

    /* Create context used by data model */
    pTadFcContext = (PCCSP_FC_CONTEXT)AnscAllocateMemory(sizeof(CCSP_FC_CONTEXT));

    if ( !pTadFcContext )
    {
        return ANSC_STATUS_RESOURCES;
    }
    else
    {
        AnscZeroMemory(pTadFcContext, sizeof(CCSP_FC_CONTEXT));
    }

    pDslhCpeController = DslhCreateCpeController(NULL, NULL, NULL);

    if ( !pDslhCpeController )
    {
        CcspTraceWarning(("CANNOT Create pDslhCpeController... Exit!\n"));

        return ANSC_STATUS_RESOURCES;
    }

    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS
ssp_engage_tad
    (
    )
{
	ANSC_STATUS					    returnStatus                                         = ANSC_STATUS_SUCCESS;
        char                                                CrName[256];

    g_pComponent_Common_Dm->Health = CCSP_COMMON_COMPONENT_HEALTH_Yellow;

    if ( pTadCcdIf )
    {
        pTadFcContext->hCcspCcdIf = (ANSC_HANDLE)pTadCcdIf;
        pTadFcContext->hMessageBus = bus_handle;
    }

    g_DslhDataModelAgent->SetFcContext((ANSC_HANDLE)g_DslhDataModelAgent, (ANSC_HANDLE)pTadFcContext);

    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)MsgHelper_CreateCcdMbiIf((void*)bus_handle, g_Subsystem));
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pTadCcdIf);
    pDslhCpeController->SetDbusHandle((ANSC_HANDLE)pDslhCpeController, bus_handle);
    pDslhCpeController->Engage((ANSC_HANDLE)pDslhCpeController);

    if ( g_Subsystem[0] != 0 )
    {
        _ansc_sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
    }
    else
    {
        _ansc_sprintf(CrName, "%s", CCSP_DBUS_INTERFACE_CR);
    }

    if ( TRUE )
    {
        ANSC_HANDLE hDiagPlugin = NULL;

        hDiagPlugin = (ANSC_HANDLE)AnscLoadLibrary(COSA_DIAG_PLUGIN_LIBRARY_NAME);

        if( hDiagPlugin == NULL)
        {
            CcspTraceWarning(("Unable to load library -- %s\n", COSA_DIAG_PLUGIN_LIBRARY_NAME));
#ifdef _ANSC_LINUX
            CcspTraceWarning(("cause:%s\n",  dlerror() ));
#endif
            g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
        }
        else
        {
            g_CosaDiagPluginInfo.InitProc = (COSADiagInitProc)
                AnscGetProcAddress
                    (
                        hDiagPlugin,
                        COSA_DIAG_PLUGIN_INIT_PROC
                    );

            if ( g_CosaDiagPluginInfo.InitProc == NULL )
            {
                AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_INIT_PROC));

                g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
            }
            else
            {
                g_CosaDiagPluginInfo.UnloadProc = (COSADiagUnloadProc)
                    AnscGetProcAddress
                        (
                            hDiagPlugin,
                            COSA_DIAG_PLUGIN_UNLOAD_PROC
                        );

                if ( g_CosaDiagPluginInfo.UnloadProc == NULL )
                {
                    AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_UNLOAD_PROC));

                    g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                }
                else
                {
                    g_CosaDiagPluginInfo.GetResultsProc = (CosaDiagGetResultsProc)
                        AnscGetProcAddress
                            (
                                hDiagPlugin,
                                COSA_DIAG_PLUGIN_GETRESULTS_PROC
                            );

                    if ( g_CosaDiagPluginInfo.GetResultsProc == NULL )
                    {
                        AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_GETRESULTS_PROC));

                        g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                    }
                    else
                    {
                        g_CosaDiagPluginInfo.ScheduleDiagnosticProc = (CosaDiagScheduleDiagnosticProc)
                            AnscGetProcAddress
                                (
                                    hDiagPlugin,
                                    COSA_DIAG_PLUGIN_SCHEDIAG_PROC
                                );

                        if ( g_CosaDiagPluginInfo.ScheduleDiagnosticProc == NULL )
                        {
                            AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_SCHEDIAG_PROC));

                            g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                        }
                        else
                        {
                            g_CosaDiagPluginInfo.SetDiagParamsProc = (CosaDiagSetDiagParamsProc)
                                AnscGetProcAddress
                                    (
                                        hDiagPlugin,
                                        COSA_DIAG_PLUGIN_SETDIAGPARAMS_PROC
                                    );

                            if ( g_CosaDiagPluginInfo.SetDiagParamsProc == NULL )
                            {
                                AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_SETDIAGPARAMS_PROC));

                                g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                            }
                            else
                            {
                                g_CosaDiagPluginInfo.SetDiagStateProc = (CosaDiagSetDiagStateProc)
                                    AnscGetProcAddress
                                        (
                                            hDiagPlugin,
                                            COSA_DIAG_PLUGIN_SETDIAGSTATE_PROC
                                        );

                                if ( g_CosaDiagPluginInfo.SetDiagStateProc == NULL )
                                {
                                    AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_SETDIAGSTATE_PROC));

                                    g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                }
                                else
                                {
                                    g_CosaDiagPluginInfo.CancelDiagnosticProc = (CosaDiagCancelDiagnosticProc)
                                        AnscGetProcAddress
                                            (
                                                hDiagPlugin,
                                                COSA_DIAG_PLUGIN_CANCELDIAG_PROC
                                            );

                                    if ( g_CosaDiagPluginInfo.CancelDiagnosticProc == NULL )
                                    {
                                        AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_CANCELDIAG_PROC));

                                        g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                    }
                                    else
                                    {
                                        g_CosaDiagPluginInfo.MemoryUsageProc = (CosaDiagMemoryUsageProc)
                                            AnscGetProcAddress
                                                (
                                                    hDiagPlugin,
                                                    COSA_DIAG_PLUGIN_MEMORYUSAGE_PROC
                                                );

                                        if ( g_CosaDiagPluginInfo.MemoryUsageProc == NULL )
                                        {
                                            AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_MEMORYUSAGE_PROC));

                                            g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                        }
                                        else
                                        {
                                            g_CosaDiagPluginInfo.MemoryTableProc = (CosaDiagMemoryTableProc)
                                                AnscGetProcAddress
                                                    (
                                                        hDiagPlugin,
                                                        COSA_DIAG_PLUGIN_MEMORYTABLE_PROC
                                                    );

                                            if ( g_CosaDiagPluginInfo.MemoryTableProc == NULL )
                                            {
                                                AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_MEMORYTABLE_PROC));

                                                g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                            }
                                            else
                                            {
                                                g_CosaDiagPluginInfo.GetConfigsProc = (CosaDiagGetConfigsProc)
                                                    AnscGetProcAddress
                                                        (
                                                            hDiagPlugin,
                                                            COSA_DIAG_PLUGIN_GETCONFIGS_PROC
                                                        );

                                                if ( g_CosaDiagPluginInfo.GetConfigsProc == NULL )
                                                {
                                                    AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_GETCONFIGS_PROC));

                                                    g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                                }
                                                else
                                                {
                                                    g_CosaDiagPluginInfo.ExportFuncProc = (CosaDiagExportFuncProc)
                                                        AnscGetProcAddress
                                                            (
                                                                hDiagPlugin,
                                                                COSA_DIAG_PLUGIN_EXPORTFUNC_PROC
                                                            );

                                                    if ( g_CosaDiagPluginInfo.ExportFuncProc == NULL )
                                                    {
                                                        AnscTraceWarning(("Unable to Get ProcAddress of  '%s'\n", COSA_DIAG_PLUGIN_EXPORTFUNC_PROC));

                                                        g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_LOAD_LIBRARY;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if ( g_CosaDiagPluginInfo.uLoadStatus == COSA_STATUS_SUCCESS )
        {
            /* COSA Diagnostic library loaded successfully */
            g_CosaDiagPluginInfo.uPluginVersion = 1;

            returnStatus = g_CosaDiagPluginInfo.InitProc(bus_handle);

            if ( returnStatus != ANSC_STATUS_SUCCESS )
            {
                g_CosaDiagPluginInfo.uLoadStatus = COSA_STATUS_ERROR_INIT;
            }

            g_DslhDataModelAgent->RegisterInternalApi(g_DslhDataModelAgent, "COSAGetDiagPluginInfo", COSAGetDiagPluginInfo);
        }
    }

    if ( g_GetParamValueByPathNameProc == NULL )
    {
        g_GetParamValueByPathNameProc = 
            (COSAGetParamValueByPathNameProc)COSAAcquireFunction("COSAGetParamValueByPathName");

        if ( !g_GetParamValueByPathNameProc )
        {
            printf("Test and Diagnostic - failed to load the function COSAGetParamValueByPathName!\n");
        }
    }

    returnStatus =
        pDslhCpeController->RegisterCcspDataModel
            (
                (ANSC_HANDLE)pDslhCpeController,
                CrName, /*CCSP_DBUS_INTERFACE_CR,*/             /* CCSP CR ID */
                COSA_PLUGIN_XML_FILE,               /* Data Model XML file. Can be empty if only base data model supported. */
                CCSP_COMPONENT_NAME_TAD,            /* Component Name    */
                CCSP_COMPONENT_VERSION_TAD,         /* Component Version */
                CCSP_COMPONENT_PATH_TAD,            /* Component Path    */
                g_Subsystem                         /* Component Prefix  */
            );

    if ( returnStatus == ANSC_STATUS_SUCCESS || CCSP_SUCCESS == returnStatus)
    {
        /* System is fully initialized */
        g_pComponent_Common_Dm->Health = CCSP_COMMON_COMPONENT_HEALTH_Green;
    }
    pollTime();
    return ANSC_STATUS_SUCCESS;
}


ANSC_STATUS
ssp_cancel_tad
    (
    )
{
    pDslhCpeController->Cancel((ANSC_HANDLE)pDslhCpeController);
    AnscFreeMemory(pDslhCpeController);

    return ANSC_STATUS_SUCCESS;
}


char*
ssp_TadCCDmGetComponentName
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->Name;
}


ULONG
ssp_TadCCDmGetComponentVersion
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->Version;
}


char*
ssp_TadCCDmGetComponentAuthor
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->Author;
}


ULONG
ssp_TadCCDmGetComponentHealth
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->Health;
}


ULONG
ssp_TadCCDmGetComponentState
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->State;
}



BOOL
ssp_TadCCDmGetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->LogEnable;
}


ANSC_STATUS
ssp_TadCCDmSetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject,
        BOOL                            bEnabled
    )
{
    /*CommonDm.LogEnable = bEnabled;*/
    if(g_pComponent_Common_Dm->LogEnable == bEnabled) return ANSC_STATUS_SUCCESS;
    g_pComponent_Common_Dm->LogEnable = bEnabled;

    if (!bEnabled)
        AnscSetTraceLevel(CCSP_TRACE_INVALID_LEVEL);
    else
        AnscSetTraceLevel(g_pComponent_Common_Dm->LogLevel);

    return ANSC_STATUS_SUCCESS;
}


ULONG
ssp_TadCCDmGetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->LogLevel;
}


ANSC_STATUS
ssp_TadCCDmSetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject,
        ULONG                           LogLevel
    )
{
    /*CommonDm.LogLevel = LogLevel;*/
    if(g_pComponent_Common_Dm->LogLevel == LogLevel) return ANSC_STATUS_SUCCESS;
    g_pComponent_Common_Dm->LogLevel = LogLevel;

    if (g_pComponent_Common_Dm->LogEnable)
        AnscSetTraceLevel(LogLevel);        

    return ANSC_STATUS_SUCCESS;
}


ULONG
ssp_TadCCDmGetMemMaxUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_ulAllocatedSizePeak;
}


ULONG
ssp_TadCCDmGetMemMinUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_pComponent_Common_Dm->MemMinUsage;
}


ULONG
ssp_TadCCDmGetMemConsumed
    (
        ANSC_HANDLE                     hThisObject
    )
{
    LONG             size = 0;

    size = AnscGetComponentMemorySize(CCSP_COMPONENT_NAME_TAD);
    if (size == -1 )
        size = 0;

    return size;
}


ANSC_STATUS
ssp_TadCCDmApplyChanges
    (
        ANSC_HANDLE                     hThisObject
    )
{
    ANSC_STATUS                         returnStatus    = ANSC_STATUS_SUCCESS;
    /* Assume the parameter settings are committed immediately. */
    /*g_pComponent_Common_Dm->LogEnable = CommonDm.LogEnable;
    g_pComponent_Common_Dm->LogLevel  = CommonDm.LogLevel;

    AnscSetTraceLevel((INT)g_pComponent_Common_Dm->LogLevel);*/

    return returnStatus;
}


ANSC_HANDLE
COSAGetDiagPluginInfo
    (
        ANSC_HANDLE                 hThisObject
    )
{
	return &g_CosaDiagPluginInfo;
}

