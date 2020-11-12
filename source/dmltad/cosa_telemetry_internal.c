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


/**************************************************************************
    module: cosa_telemetry_internal.c
    For COSA Data Model Library Development
    description:
    This file implementes Enable /Disable telemetry service
    environment:
    platform independent
**************************************************************************/

#include <syscfg/syscfg.h>

#include "cosa_telemetry_internal.h"
#include "cosa_telemetry_dml.h"
#include "plugin_main_apis.h"


/**********************************************************************
Caller:         Owner of the object
Prototype:      ANSC_HANDLE CosatelemetryCreate();
Description:    This function constructs cosa telemetry object and return handle.
Argument:
Return:         Newly created telemetry object.
**********************************************************************/
ANSC_HANDLE CosatelemetryCreate(void)
{
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    PCOSA_DATAMODEL_TELEMETRY       pMyObject    = (PCOSA_DATAMODEL_TELEMETRY)NULL;
    /*
     * We create object by first allocating memory for holding the variables and member functions.
     */
    pMyObject = (PCOSA_DATAMODEL_TELEMETRY)AnscAllocateMemory(sizeof(COSA_DATAMODEL_TELEMETRY));

    if (!pMyObject)
    {
        CcspTraceError(("%s exit ERROR\n", __FUNCTION__));
        return (ANSC_HANDLE)NULL;
    }

    pMyObject->Oid               = COSA_DATAMODEL_TELEMETRY_OID;
    pMyObject->Create            = CosatelemetryCreate;
    pMyObject->Remove            = CosatelemetryRemove;
    pMyObject->Initialize        = CosatelemetryInitialize;

    pMyObject->Initialize((ANSC_HANDLE)pMyObject);
    return (ANSC_HANDLE)pMyObject;
}

PDML_DCM_DOWNLOAD_STATUS DmlGetDcmStatus(ANSC_HANDLE hThisObject)
{
    char intBuf[12];

    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;
    PDML_DCM_DOWNLOAD_STATUS  pDcmStatus = (PDML_DCM_DOWNLOAD_STATUS)NULL;

    pDcmStatus = (PDML_DCM_DOWNLOAD_STATUS)AnscAllocateMemory(sizeof(DML_DCM_DOWNLOAD_STATUS));
    if (!pDcmStatus)
    {
        CcspTraceError(("\n %s DCM Download Status allocation failed\n", __FUNCTION__));
        return NULL;
    }

    syscfg_get(NULL, "dcm_lastSuccessTimestamp", pDcmStatus->LastSuccessTimestamp, sizeof(pDcmStatus->LastSuccessTimestamp));
    syscfg_get(NULL, "dcm_lastAttemptTimestamp", pDcmStatus->LastAttemptTimestamp, sizeof(pDcmStatus->LastAttemptTimestamp));
    syscfg_get(NULL, "dcm_httpStatusString", pDcmStatus->HTTPStatusString, sizeof(pDcmStatus->HTTPStatusString));

    syscfg_get(NULL, "dcm_httpStatus", intBuf, sizeof(intBuf));
    pDcmStatus->HTTPStatus = strtol(intBuf, NULL, 10);

    syscfg_get(NULL, "dcm_attemptCount", intBuf, sizeof(intBuf));
    pDcmStatus->AttemptCount = strtol(intBuf, NULL, 10);

    return pDcmStatus;
}

PDML_ODH_UPLOAD_STATUS DmlGetUploadStatus(ANSC_HANDLE hThisObject)
{
    char intBuf[12];

    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;
    PDML_ODH_UPLOAD_STATUS    pUploadStatus = (PDML_ODH_UPLOAD_STATUS)NULL;

    pUploadStatus = (PDML_ODH_UPLOAD_STATUS)AnscAllocateMemory(sizeof(DML_ODH_UPLOAD_STATUS));
    if (!pUploadStatus)
    {
        CcspTraceError(("\n %s DCA upload status allocation failed\n", __FUNCTION__));
        return NULL;
    }

    syscfg_get(NULL, "upload_lastSuccessTimestamp", pUploadStatus->LastSuccessTimestamp, sizeof(pUploadStatus->LastSuccessTimestamp));
    syscfg_get(NULL, "upload_lastAttemptTimestamp", pUploadStatus->LastAttemptTimestamp, sizeof(pUploadStatus->LastAttemptTimestamp));
    syscfg_get(NULL, "upload_httpStatusString", pUploadStatus->HTTPStatusString, sizeof(pUploadStatus->HTTPStatusString));

    syscfg_get( NULL, "upload_httpStatus", intBuf, sizeof(intBuf));
    pUploadStatus->HTTPStatus = strtol(intBuf, NULL, 10);

    syscfg_get(NULL, "upload_attemptCount", intBuf, sizeof(intBuf));
    pUploadStatus->AttemptCount = strtol(intBuf, NULL, 10);

    return pUploadStatus;
}

PDML_DCM_RETRY_CONFIG DmlGetRetryCfg(ANSC_HANDLE hThisObject)
{
    char intBuf[12];

    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;
    PDML_DCM_RETRY_CONFIG     pRetryConfig = (PDML_DCM_RETRY_CONFIG)NULL;

    pRetryConfig = (PDML_DCM_RETRY_CONFIG)AnscAllocateMemory(sizeof(DML_DCM_RETRY_CONFIG));
    if (!pRetryConfig)
    {
        CcspTraceError(("\n %s DCM retry config allocation failed\n", __FUNCTION__));
        return NULL;
    }

    syscfg_get(NULL, "dcm_retry_attemptInterval", intBuf, sizeof(intBuf));
    pRetryConfig->AttemptInterval = strtol(intBuf, NULL, 10);

    syscfg_get( NULL, "dcm_retry_maxAttempts", intBuf, sizeof(intBuf));
    pRetryConfig->MaxAttempts = strtol(intBuf, NULL, 10);

    return pRetryConfig;
}

/**********************************************************************
Caller:         Owner of the object
Prototype:      ANSC_HANDLE CosatelemetryInitialize ();
Description:    This function Initializes the datamodel object and call the services accordingly.
Argument:
Return:         Returns the ANSC_STATUS.

**********************************************************************/
ANSC_STATUS CosatelemetryInitialize(ANSC_HANDLE hThisObject)
{
    ANSC_STATUS                     returnStatus        = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_TELEMETRY       pMyObject           = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;
    CcspTraceDebug(("%s Entered\n", __FUNCTION__));
    returnStatus = CosaDmlTelemetryInit((ANSC_HANDLE)pMyObject);

    if (returnStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s exit ERROR\n", __FUNCTION__));
        return returnStatus;
    }

    pMyObject->pDcmStatus = DmlGetDcmStatus((ANSC_HANDLE)pMyObject);
    pMyObject->pUploadStatus = DmlGetUploadStatus((ANSC_HANDLE)pMyObject);
    pMyObject->pRetryConfig = DmlGetRetryCfg((ANSC_HANDLE)pMyObject);

    return returnStatus;
}

/**********************************************************************
Caller:         Owner of the object
Prototype:      ANSC_HANDLE CosatelemetryRemove ();
Description:    This function deletes the momory of the Telemetry Object.
Argument:
Return:         Returns the ANSC_STATUS.

**********************************************************************/
ANSC_STATUS CosatelemetryRemove ( ANSC_HANDLE hThisObject )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PCOSA_DATAMODEL_TELEMETRY       pMyObject    = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;

    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    if (pMyObject)
    {
        if(pMyObject->pDcmStatus )
        {
            AnscFreeMemory(pMyObject->pDcmStatus);
            pMyObject->pDcmStatus = NULL;
        }
        if (pMyObject->pUploadStatus)
        {
            AnscFreeMemory(pMyObject->pUploadStatus);
            pMyObject->pUploadStatus = NULL;
        }
        if (pMyObject->pRetryConfig)
        {
            AnscFreeMemory(pMyObject->pRetryConfig);
            pMyObject->pRetryConfig = NULL;
        }

        /* Remove self */
        AnscFreeMemory((ANSC_HANDLE)pMyObject);
    }

    return returnStatus;
}

/**********************************************************************
Caller:         Owner of the object
Prototype:      ANSC_HANDLE CosaDmlTelemetryInit ();
Description:    This function Initializes the objects for Telemetry and run the services accordingly..
Argument:
Return:         Returns the ANSC_STATUS_SUCCESS.

**********************************************************************/
ANSC_STATUS CosaDmlTelemetryInit(ANSC_HANDLE hThisObject)
{
    char buf[8];
    PCOSA_DATAMODEL_TELEMETRY pMyObject = (PCOSA_DATAMODEL_TELEMETRY)hThisObject;

    CcspTraceDebug(("%s Entered\n", __FUNCTION__));

    syscfg_get(NULL, "telemetry_enable", buf, sizeof(buf));
    pMyObject->Enable = (strcmp(buf, "true") == 0) ? TRUE : FALSE;

#ifdef _PUMA6_ARM_
    if (pMyObject->Enable == TRUE)
    {
        system("rpcclient2 'syscfg set telemetry_enable true; syscfg set T2Enable true'");
        system("rpcclient2 '/usr/sbin/icu -R -B -p 192.168.254.254:2222'");
    }
#endif

    pMyObject->DCMConfigForceDownload = FALSE;

    syscfg_get(NULL, "UploadRepositoryURL", pMyObject->UploadRepositoryURL, sizeof(pMyObject->UploadRepositoryURL));

    syscfg_get(NULL, "T2ConfigURL", pMyObject->DCMConfigFileURL, sizeof(pMyObject->DCMConfigFileURL));

    return ANSC_STATUS_SUCCESS;
}
