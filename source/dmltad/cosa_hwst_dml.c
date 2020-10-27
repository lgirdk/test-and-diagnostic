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

#include "ansc_platform.h"
#include "plugin_main_apis.h"
#include "cosa_hwst_dml.h"
#include "platform_hal.h"

#define HWSELFTEST_RESULTS_SIZE 2048
#define HWSELFTEST_RESULTS_FILE "/tmp/hwselftest.results"

BOOL hwst_runTest = FALSE;

/***********************************************************************


 APIs for Object:

    X_RDK_hwHealthTest.

    *  hwHealthTest_GetParamBoolValue
    *  hwHealthTest_SetParamBoolValue
    *  hwHealthTest_GetParamStringValue
***********************************************************************/

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
    hwHealthTest_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )

    description:

        This function is called to retrieve Boolean parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/

BOOL
hwHealthTest_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */
    if ( AnscEqualString(ParamName, "executeTest", TRUE))
    {
#ifdef COLUMBO_HWTEST
        *pBool = hwst_runTest;
        AnscTraceFlow(("%s Execute tests : %d \n", __FUNCTION__, *pBool));
        return TRUE;
#else
        *pBool = FALSE;
#endif
    }
    AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        BOOL
    hwHealthTest_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    );

    description:

        This function is called to set BOOL parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
hwHealthTest_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */
    if ( AnscEqualString(ParamName, "executeTest", TRUE))
    {
#ifdef COLUMBO_HWTEST
        AnscTraceFlow(("%s Execute tests : %d \n", __FUNCTION__, bValue));
        FILE* fp = fopen("/tmp/.hwst_run", "r");
        char* clientVer = (char*) malloc(8*sizeof(char));
        char version[8] = {'\0'};
        if(NULL != fp)
        {
            if(NULL != clientVer)
            {
                fscanf(fp, "%s", clientVer);
                strcpy(version,clientVer);
                free(clientVer);
                clientVer = NULL;
            }
        }

        if(NULL != clientVer)
        {
            free(clientVer);
        }

        if(fp != NULL )
        {
            fclose(fp);
            if(strcmp(version, "0001") && bValue)
            {
                AnscTraceFlow(("Multiple connections not allowed"));
                return FALSE;
            }
        }
        if (bValue && !strcmp(version, "0001"))
        {
            AnscTraceFlow(("Hwselftest is already running, hence returning success\n"));
            return TRUE;
        }

        char cmd[128] = {0};
        hwst_runTest = bValue;
        if(hwst_runTest)
        {
            AnscTraceFlow(("%s Execute tests value is set to true\n", __FUNCTION__));
            if(fopen(HWSELFTEST_RESULTS_FILE, "r"))
            {
                if (remove(HWSELFTEST_RESULTS_FILE) == 0)
                    AnscTraceFlow(("%s Deleted results file\n", __FUNCTION__));
            }
            memset(cmd, 0, sizeof(cmd));
            AnscCopyString(cmd, "/usr/bin/hwselftest_run.sh 0001 &");
            AnscTraceFlow(("Executing Hwselftest..\n"));
            system(cmd);
        }
        else
        {
            AnscTraceFlow(("%s Execute tests value is set to false\n", __FUNCTION__));
        }
        return TRUE;
#else
        hwst_runTest = FALSE;
#endif
    }
    AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));
    return FALSE;
}

/**********************************************************************

    caller:     owner of this object

    prototype:

        ULONG
        hwHealthTest_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue
            );

    description:

        This function is called to retrieve string parameter value;

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue
                The string value buffer;

    return:     0 if succeeded;
                1 unable to read results file;

**********************************************************************/
ULONG
hwHealthTest_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue
    )
{
    if ( AnscEqualString(ParamName, "Results", TRUE))
    {
#ifdef COLUMBO_HWTEST
        AnscTraceFlow(("%s Results get\n", __FUNCTION__));

        FILE *p = fopen(HWSELFTEST_RESULTS_FILE, "r");
        if (p == NULL)
        {
            AnscTraceWarning(("%s, hwselftest.results not present.\n", __FUNCTION__));
            return 1;
        }

        char hwst_result_string[HWSELFTEST_RESULTS_SIZE] = {'\0'};
        char results_data[HWSELFTEST_RESULTS_SIZE] = {'\0'};
        int offset = 0;
        while(fgets(results_data, HWSELFTEST_RESULTS_SIZE, p) != NULL && results_data[0] != '\n')
        {
            strcpy(hwst_result_string + offset, results_data); /* copy input at offset into output */
            offset += strlen(results_data);               /* advance the offset by the length of the string */
            AnscTraceFlow(("%s Results output string after copying a new line: %s\n", __FUNCTION__, hwst_result_string));
        }
        hwst_runTest = FALSE;
        AnscCopyString(pValue, hwst_result_string);
        AnscTraceFlow(("%s Results - Overall result: %s\n", __FUNCTION__, pValue));
        fclose(p);
        return 0;
#endif
    }
    AnscTraceWarning(("Unsupported parameter '%s'\n", ParamName));
    return 1;
}
