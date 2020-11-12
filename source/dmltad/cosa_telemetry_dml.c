/*********************************************************************************
 * Copyright 2019 Liberty Global B.V.
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
  *********************************************************************************/

#include <syscfg/syscfg.h>

#include "ansc_platform.h"
#include "cosa_telemetry_dml.h"
#include "cosa_telemetry_internal.h"
#include "plugin_main_apis.h"

static BOOL start_dcm_service = FALSE;

/***********************************************************************
 APIs for Object:
    COSA_telemetry.
    *  Telemetry_GetParamBoolValue
    *  Telemetry_SetParamBoolValue
    *  Telemetry_GetParamStringValue
    *  Telemetry_SetParamStringValue
***********************************************************************/

BOOL
Telemetry_GetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL* bValue)
{
    return TRUE;
}

BOOL
Telemetry_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    return TRUE;
}

ANSC_STATUS
Telemetry_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "DCMConfigFileURL") == 0)
    {
        AnscCopyString(pValue, pMyObject->DCMConfigFileURL);
        CcspTraceDebug(("%s: Copied the DCMConfigFileURL string into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "UploadRepositoryURL") == 0)
    {
        syscfg_get(NULL, "UploadRepositoryURL", pMyObject->UploadRepositoryURL, sizeof(pMyObject->UploadRepositoryURL));
        AnscCopyString(pValue, pMyObject->UploadRepositoryURL);
        CcspTraceDebug(("%s: Copied the UploadRepositoryURL string into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL
Telemetry_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "DCMConfigFileURL") == 0)
    {
        if (syscfg_set(NULL, "DCMConfigFileURL", pString) != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        if (syscfg_commit() != 0)
        {
            CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        AnscCopyString(pMyObject->DCMConfigFileURL, pString);
        CcspTraceDebug(("%s Set value %s: Exit\n", __FUNCTION__, pString));
        return TRUE;
    }

    return FALSE;
}

BOOL
Telemetry_Validate
(
    ANSC_HANDLE                 hInsContext,
    char*                       pReturnParamName,
    ULONG*                      puLength
)
{
    return TRUE;
}

BOOL
Telemetry_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}

BOOL
Telemetry_Rollback
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}


/***********************************************************************

 APIs for Object:

    Telemetry.DcmDownloadStatus.
    *  DcmDownloadStatus_GetParamStringValue
    *  DcmDownloadStatus_GetParamUlongValue
    *  DcmDownloadStatus_Validate
    *  DcmDownloadStatus_Commit
    *  DcmDownloadStatus_Rollback

***********************************************************************/
ANSC_STATUS
DcmDownloadStatus_GetParamStringValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    char*                       pValue
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatusString") == 0)
    {
        char buff[256];

        syscfg_get(NULL, "dcm_httpStatusString", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastSuccessTimestamp") == 0)
    {
        char buff[30];

        syscfg_get(NULL, "dcm_lastSuccessTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->LastSuccessTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastSuccessTimestamp);
        CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastAttemptTimestamp") == 0)
    {
        char buff[30];

        syscfg_get(NULL, "dcm_lastAttemptTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pDcmStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL
DcmDownloadStatus_GetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG*                      pUlong
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatus") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_httpStatus", buff, sizeof(buff));
        pMyObject->pDcmStatus->HTTPStatus = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pDcmStatus->HTTPStatus;
        return TRUE;
    }

    if (strcmp(ParamName, "AttemptCount") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_attemptCount", buff, sizeof(buff));
        pMyObject->pDcmStatus->AttemptCount = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pDcmStatus->AttemptCount;
        return TRUE;
    }

    return FALSE;
}

BOOL
DcmDownloadStatus_Validate
(
    ANSC_HANDLE                 hInsContext,
    char*                       pReturnParamName,
    ULONG*                      puLength
)
{
    return TRUE;
}

BOOL
DcmDownloadStatus_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}

BOOL
DcmDownloadStatus_Rollback
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}


/***********************************************************************

 APIs for Object:

    Telemetry.UploadStatus.
    *  UploadStatus_GetParamStringValue
    *  UploadStatus_GetParamUlongValue
    *  UploadStatus_Validate
    *  UploadStatus_Commit
    *  UploadStatus_Rollback

***********************************************************************/
ANSC_STATUS
UploadStatus_GetParamStringValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    char*                       pValue
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY       pMyObject  = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatusString") == 0)
    {
        char buff[256];

        syscfg_get( NULL, "upload_httpStatusString", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastSuccessTimestamp") == 0)
    {
        char buff[30];

        syscfg_get( NULL, "upload_lastSuccessTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->LastSuccessTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->LastSuccessTimestamp);
        CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    if (strcmp(ParamName, "LastAttemptTimestamp") == 0)
    {
        char buff[30];

        syscfg_get( NULL, "upload_lastAttemptTimestamp", buff, sizeof(buff));
        AnscCopyString(pMyObject->pUploadStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

BOOL
UploadStatus_GetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG*                      pUlong
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY       pMyObject  = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "HTTPStatus") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "upload_httpStatus", buff, sizeof(buff));
        pMyObject->pUploadStatus->HTTPStatus = strtol(buff, NULL, 10);
        *pUlong = pMyObject->pUploadStatus->HTTPStatus;
        return TRUE;
    }

    if (strcmp(ParamName, "AttemptCount") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "upload_attemptCount", buff, sizeof(buff));
        pMyObject->pUploadStatus->AttemptCount = strtol(buff, NULL, 10);
        *pUlong = pMyObject->pUploadStatus->AttemptCount;
        return TRUE;
    }

    return FALSE;
}

BOOL
UploadStatus_Validate
(
    ANSC_HANDLE                 hInsContext,
    char*                       pReturnParamName,
    ULONG*                      puLength
)
{
    return TRUE;
}

BOOL
UploadStatus_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}

BOOL
UploadStatus_Rollback
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}


/***********************************************************************

 APIs for Object:

    Telemetry.ConnectivityTest.

    *  DcmRetryConfig_GetParamUlongValue
    *  DcmRetryConfig_SetParamUlongValue
    *  DcmRetryConfig_Validate
    *  DcmRetryConfig_Commit
    *  DcmRetryConfig_Rollback

***********************************************************************/

BOOL
DcmRetryConfig_GetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG*                      pUlong
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "AttemptInterval") == 0)
    {
        char buff[12];

        syscfg_get( NULL, "dcm_retry_attemptInterval", buff, sizeof(buff));
        pMyObject->pRetryConfig->AttemptInterval = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pRetryConfig->AttemptInterval;
        return TRUE;
    }

    if (strcmp(ParamName, "MaxAttempts") == 0)
    {
        char buff[12];

        syscfg_get(NULL, "dcm_retry_maxAttempts", buff, sizeof(buff));
        pMyObject->pRetryConfig->MaxAttempts = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pRetryConfig->MaxAttempts;
        return TRUE;
    }

    return FALSE;
}

BOOL
DcmRetryConfig_SetParamUlongValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    ULONG                       uValue
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject= (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;

    if (strcmp(ParamName, "AttemptInterval") == 0)
    {
        if ( pMyObject->pRetryConfig->AttemptInterval == uValue )
        {
            return TRUE;
        }

        if (uValue < 45 || uValue > 300)    // Range needs to be discussed
        {
            CcspTraceWarning(("%s AttemptInterval value %u is not in range\n", __FUNCTION__, uValue));
            return FALSE;
        }
        /* save update to backup */
        if (syscfg_set_u_commit(NULL, "dcm_retry_attemptInterval", uValue) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_attemptInterval\n", __FUNCTION__));
            return FALSE;
        }

        pMyObject->pRetryConfig->AttemptInterval = uValue;
        return TRUE;
    }

    if (strcmp(ParamName, "MaxAttempts") == 0)
    {
        if ( pMyObject->pRetryConfig->MaxAttempts == uValue )
        {
            return TRUE;
        }

        if (uValue < 2 || uValue > 5)
        {
            CcspTraceWarning(("%s MaxAttempts value %u is not in range\n", __FUNCTION__, uValue));
            return FALSE;
        }
        /* save update to backup */
        if (syscfg_set_u_commit(NULL, "dcm_retry_maxAttempts", uValue) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_maxAttempts\n", __FUNCTION__));
            return FALSE;
        }

        pMyObject->pRetryConfig->MaxAttempts = uValue;
        return TRUE;
    }

    return FALSE;
}

BOOL
DcmRetryConfig_Validate
(
    ANSC_HANDLE                 hInsContext,
    char*                       pReturnParamName,
    ULONG*                      puLength
)
{
    return TRUE;
}

BOOL
DcmRetryConfig_Commit
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}

BOOL
DcmRetryConfig_Rollback
(
    ANSC_HANDLE                 hInsContext
)
{
    return TRUE;
}
