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

#include "ansc_platform.h"
#include "cosa_telemetry_dml.h"
#include "cosa_telemetry_internal.h"
#include "plugin_main_apis.h"

static BOOL start_telemetry_service = FALSE;

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
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "Enable", TRUE))
    {
        CcspTraceDebug(("%s: Existing value for Telemetry.Enable is: %d\n", __FUNCTION__, pMyObject->Enable));
        *bValue =  pMyObject->Enable;
        return TRUE;
    }
    else
    {
        CcspTraceError(("%s: Unknown parameter %s\n", __FUNCTION__, ParamName));
    }
    return FALSE;
}


BOOL
Telemetry_SetParamBoolValue(ANSC_HANDLE hInsContext, char* ParamName, BOOL bValue)
{
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "Enable", TRUE))
    {
        CcspTraceDebug(("%s Telemetry Enable Set Value is %d: and the existing value is %d\n", __FUNCTION__, bValue, pMyObject->Enable));
        if (pMyObject->Enable == bValue)
        {
            return TRUE;
        }

        /* Telemetry agent receives the events only if T2Enable is enabled */
        if (bValue == TRUE)
        {
            if (syscfg_set(NULL, "T2Enable", "true") != 0)
            {
                CcspTraceError(("syscfg_set failed for Telemetry2 Enable\n"));
                return FALSE;
            }
        }

        if (syscfg_set(NULL, "telemetry_enable", bValue ? "true" : "false") != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        else
        {
            if (syscfg_commit() != 0)
            {
                CcspTraceError(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
                return FALSE;
            }

            if (bValue == TRUE)
            {
                start_telemetry_service = TRUE;
            }
            else
            {
                CcspTraceInfo(("%s: Stop Telemetry Service, removing the cronjob (autodownload_dcmconfig.sh) from the cron table\n", __FUNCTION__));
                system("crontab -l | grep -v 'autodownload_dcmconfig.sh' | crontab -");
                system("killall -9 telemetry2_0");
            }
        }
        pMyObject->Enable = bValue;
        CcspTraceDebug(("%s: Telemetry Enable Set Value is %d: and the value after set is %d\n", __FUNCTION__, bValue, pMyObject->Enable));

        return TRUE;
    }
    else
    {
        CcspTraceError(("%s Unknown parameter %s\n", __FUNCTION__, ParamName));
    }

    return FALSE;
}


BOOL
Telemetry_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "DCMConfigFileURL", TRUE))
    {
        AnscCopyString(pValue, pMyObject->DCMConfigFileURL);
        CcspTraceDebug(("%s: Copied the DCMConfigFileURL string into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    else if(AnscEqualString(ParamName, "UploadRepositoryURL", TRUE))
    {
        char uploadURL[256];
        memset(uploadURL, 0, sizeof(uploadURL));
        syscfg_get(NULL, "UploadRepositoryURL", uploadURL, sizeof(uploadURL));
        AnscCopyString(pMyObject->UploadRepositoryURL, uploadURL);
        AnscCopyString(pValue, pMyObject->UploadRepositoryURL);
        CcspTraceDebug(("%s: Copied the UploadRepositoryURL string into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    else
    {
        CcspTraceError(("%s Unknown parameter %s\n", __FUNCTION__, ParamName));
    }

    return FALSE;
}

BOOL
Telemetry_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "DCMConfigFileURL", TRUE))
    {
        if (syscfg_set(NULL, "DCMConfigFileURL", pString) != 0)
        {
            CcspTraceWarning(("%s: syscfg_set failed for %s\n", __FUNCTION__, ParamName));
            return FALSE;
        }
        else
        {
            if (syscfg_commit() != 0)
            {
                CcspTraceWarning(("%s: syscfg commit failed for %s\n", __FUNCTION__, ParamName));
                return FALSE;
            }
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
    CcspTraceDebug(("%s: Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if ((pMyObject->Enable == TRUE) && (start_telemetry_service == TRUE))
    {
        CcspTraceInfo(("%s: Telemetry is enabled, downloading DCMConfigFile\n", __FUNCTION__));
        CcspTraceInfo(("%s: Starting  Telemetry service\n", __FUNCTION__));
        system("/usr/bin/telemetry2_0 &");
    }
    start_telemetry_service = FALSE;
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
BOOL
DcmDownloadStatus_GetParamStringValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    char*                       pValue
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "HTTPStatusString", TRUE))
    {
        char buff[256];
        memset(buff, 0, sizeof(buff));
        syscfg_get(NULL, "dcm_httpStatusString", buff, sizeof(buff));

        AnscCopyString(pMyObject->pDcmStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    if (AnscEqualString(ParamName, "LastSuccessTimestamp", TRUE))
    {
        char buff[30];
        memset(buff, 0, sizeof(buff));
        syscfg_get(NULL, "dcm_lastSuccessTimestamp", buff, sizeof(buff));

        AnscCopyString(pMyObject->pDcmStatus->LastSuccessTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastSuccessTimestamp);
        CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    else if (AnscEqualString(ParamName, "LastAttemptTimestamp", TRUE))
    {
        char buff[30];
        memset(buff, 0, sizeof(buff));
        syscfg_get(NULL, "dcm_lastAttemptTimestamp", buff, sizeof(buff));

        AnscCopyString(pMyObject->pDcmStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pDcmStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    return FALSE;
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

    if (AnscEqualString(ParamName, "HTTPStatus", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "dcm_httpStatus", buff, sizeof(buff));

        pMyObject->pDcmStatus->HTTPStatus = strtol(buff, NULL, 10);

        *pUlong = (ULONG) pMyObject->pDcmStatus->HTTPStatus;
        return TRUE;
    }

    if (AnscEqualString(ParamName, "AttemptCount", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
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
BOOL
UploadStatus_GetParamStringValue
(
    ANSC_HANDLE                 hInsContext,
    char*                       ParamName,
    char*                       pValue
)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    PCOSA_DATAMODEL_TELEMETRY       pMyObject  = (PCOSA_DATAMODEL_TELEMETRY)g_pCosaBEManager->hTelemetry;
    if (AnscEqualString(ParamName, "HTTPStatusString", TRUE))
    {
        /* collect value */
        char buff[256];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "upload_httpStatusString", buff, sizeof(buff));

        AnscCopyString(pMyObject->pUploadStatus->HTTPStatusString, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->HTTPStatusString);
        CcspTraceDebug(("%s Copied the HTTPStatusString into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    if (AnscEqualString(ParamName, "LastSuccessTimestamp", TRUE))
    {
        /* collect value */
        char buff[30];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "upload_lastSuccessTimestamp", buff, sizeof(buff));

        AnscCopyString(pMyObject->pUploadStatus->LastSuccessTimestamp, buff);
          AnscCopyString(pValue, pMyObject->pUploadStatus->LastSuccessTimestamp);
          CcspTraceDebug(("%s Copied the LastSuccessTimestamp into the parameter\n", __FUNCTION__));
          return TRUE;
    }
    else if (AnscEqualString(ParamName, "LastAttemptTimestamp", TRUE))
    {
        char buff[30];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "upload_lastAttemptTimestamp", buff, sizeof(buff));

        AnscCopyString(pMyObject->pUploadStatus->LastAttemptTimestamp, buff);
        AnscCopyString(pValue, pMyObject->pUploadStatus->LastAttemptTimestamp);
        CcspTraceDebug(("%s Copied the LastAttemptTimestamp into the parameter\n", __FUNCTION__));
        return TRUE;
    }
    return FALSE;
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

    if (AnscEqualString(ParamName, "HTTPStatus", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "upload_httpStatus", buff, sizeof(buff));

        pMyObject->pUploadStatus->HTTPStatus = strtol(buff, NULL, 10);
        *pUlong = pMyObject->pUploadStatus->HTTPStatus;
        return TRUE;
    }

    if (AnscEqualString(ParamName, "AttemptCount", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
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
    if (AnscEqualString(ParamName, "AttemptInterval", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
        syscfg_get( NULL, "dcm_retry_attemptInterval", buff, sizeof(buff));

        pMyObject->pRetryConfig->AttemptInterval = strtol(buff, NULL, 10);
        *pUlong = (ULONG) pMyObject->pRetryConfig->AttemptInterval;
        return TRUE;
    }

    if (AnscEqualString(ParamName, "MaxAttempts", TRUE))
    {
        /* collect value */
        char buff[12];
        memset(buff, 0, sizeof(buff));
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

    if (AnscEqualString(ParamName, "AttemptInterval", TRUE))
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
        char value[12];
        snprintf(value, sizeof(value), "%u", uValue);
        if (syscfg_set(NULL, "dcm_retry_attemptInterval", value) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_attemptInterval\n", __FUNCTION__));
            return FALSE;
        }
        if (syscfg_commit() != 0)
        {
            CcspTraceWarning(("%s syscfg commit failed for dcm_retry_attemptInterval\n", __FUNCTION__));
            return FALSE;
        }
        pMyObject->pRetryConfig->AttemptInterval = uValue;
        return TRUE;
    }

    if (AnscEqualString(ParamName, "MaxAttempts", TRUE))
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
        char value[12];
        snprintf(value, sizeof(value), "%u", uValue);
        if (syscfg_set(NULL, "dcm_retry_maxAttempts", value) != 0)
        {
            CcspTraceWarning(("%s syscfg set failed for dcm_retry_maxAttempts\n", __FUNCTION__));
            return FALSE;
        }
        if (syscfg_commit() != 0)
        {
            CcspTraceWarning(("%s syscfg commit failed for dcm_retry_maxAttempts\n", __FUNCTION__));
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
