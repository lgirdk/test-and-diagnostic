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

#ifdef __GNUC__
#if (!defined _NO_EXECINFO_H_)
#include <execinfo.h>
#endif
#endif

#include "ssp_global.h"
#include "stdlib.h"

PDSLH_CPE_CONTROLLER_OBJECT     pDslhCpeController      = NULL;
PCOMPONENT_COMMON_DM            g_pComponent_Common_Dm  = NULL;
PCCSP_FC_CONTEXT                pTadFcContext           = (PCCSP_FC_CONTEXT            )NULL;
PCCSP_CCD_INTERFACE             pTadCcdIf               = (PCCSP_CCD_INTERFACE         )NULL;
PCCC_MBI_INTERFACE              pTadMbiIf               = (PCCC_MBI_INTERFACE          )NULL;
char                            g_Subsystem[32]         = {0};
BOOL                            g_bActive               = FALSE;

int  cmd_dispatch(int  command)
{
    ULONG                           ulInsNumber = 0;
    parameterValStruct_t val[3] = {0};
    char*                           pParamNames[]      = {"Device.IP.Diagnostics.IPPing."};
    parameterValStruct_t**          ppReturnVal        = NULL;
    parameterInfoStruct_t**         ppReturnValNames   = NULL;
    parameterAttributeStruct_t**    ppReturnvalAttr    = NULL;
    ULONG                           ulReturnValCount   = 0;
    ULONG                           i                  = 0;

    switch ( command )
    {
            case	'e' :

#ifdef _ANSC_LINUX
                CcspTraceInfo(("Connect to bus daemon...\n"));

            {
                char                            CName[256];

                if ( g_Subsystem[0] != 0 )
                {
                    _ansc_sprintf(CName, "%s%s", g_Subsystem, CCSP_COMPONENT_ID_TAD);
                }
                else
                {
                    _ansc_sprintf(CName, "%s", CCSP_COMPONENT_ID_TAD);
                }

                ssp_TadMbi_MessageBusEngage
                    ( 
                        CName,
                        CCSP_MSG_BUS_CFG,
                        CCSP_COMPONENT_PATH_TAD
                    );
            }

#endif

                ssp_create_tad();
                ssp_engage_tad();

                g_bActive = TRUE;

                CcspTraceInfo(("Test & Diagnostic Module loaded successfully...\n"));

            break;

            case    'r' :

            CcspCcMbi_GetParameterValues
                (
                    DSLH_MPA_ACCESS_CONTROL_ACS,
                    pParamNames,
                    1,
                    &ulReturnValCount,
                    &ppReturnVal,
                    NULL
                );



            for ( i = 0; i < ulReturnValCount; i++ )
            {
                CcspTraceWarning(("Parameter %d name: %s value: %s \n", i+1, ppReturnVal[i]->parameterName, ppReturnVal[i]->parameterValue));
            }


/*
            CcspCcMbi_GetParameterNames
                (
                    "Device.DeviceInfo.",
                    0,
                    &ulReturnValCount,
                    &ppReturnValNames
                );

            for ( i = 0; i < ulReturnValCount; i++ )
            {
                CcspTraceWarning(("Parameter %d name: %s bWritable: %d \n", i+1, ppReturnValNames[i]->parameterName, ppReturnValNames[i]->writable));
            }
*/
/*
            CcspCcMbi_GetParameterAttributes
                (
                    pParamNames,
                    1,
                    &ulReturnValCount,
                    &ppReturnvalAttr
                );
*/
/*
            CcspCcMbi_DeleteTblRow
                (
                    123,
                    "Device.X_CISCO_COM_SWDownload.SWDownload.1."
                );
*/

			break;

        case    'm':

                AnscPrintComponentMemoryTable(pComponentName);

                break;

        case    't':

                AnscTraceMemoryTable();

                break;

        case    'c':

                ssp_cancel_tad();

                break;

        default:
            break;
    }

    return 0;
}

static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#if (!defined _COSA_SIM_) && (!defined _NO_EXECINFO_H_)
        void* tracePtrs[100];
        char** funcNames = NULL;
        int i, count = 0;

        int fd;
        const char* path = "/nvram/tadssp_backtrace";
        fd = open(path, O_RDWR | O_CREAT);
        if (fd < 0)
        {
            fprintf(stderr, "failed to open backtrace file: %s", path);
            return;
        }

        count = backtrace( tracePtrs, 100 );
        backtrace_symbols_fd( tracePtrs, count, fd );
        close(fd);

        funcNames = backtrace_symbols( tracePtrs, count );

        if ( funcNames ) {
            // Print the stack trace
            for( i = 0; i < count; i++ )
                printf("%s\n", funcNames[i] );

            // Free the string pointers
            free( funcNames );
        }
#endif
#endif
}

#if defined(_ANSC_LINUX)
static void daemonize(void) {
	int fd;
	switch (fork()) {
	case 0:
		break;
	case -1:
		// Error
		CcspTraceInfo(("Error daemonizing (fork)! %d - %s\n", errno, strerror(
				errno)));
		exit(0);
		break;
	default:
		_exit(0);
	}

	if (setsid() < 	0) {
		CcspTraceInfo(("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno)));
		exit(0);
	}

//	chdir("/");


#ifndef  _DEBUG

	fd = open("/dev/null", O_RDONLY);
	if (fd != 0) {
		dup2(fd, 0);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 1) {
		dup2(fd, 1);
		close(fd);
	}
	fd = open("/dev/null", O_WRONLY);
	if (fd != 2) {
		dup2(fd, 2);
		close(fd);
	}
#endif
}

void sig_handler(int sig)
{
    if ( sig == SIGINT ) {
    	signal(SIGINT, sig_handler); /* reset it to this function */
    	CcspTraceError(("SIGINT received!\n"));
        exit(0);
    }
    else if ( sig == SIGUSR1 ) {
    	signal(SIGUSR1, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGUSR1 received!\n"));
    }
    else if ( sig == SIGUSR2 ) {
    	CcspTraceWarning(("SIGUSR2 received!\n"));
    }
    else if ( sig == SIGCHLD ) {
    	signal(SIGCHLD, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
    	signal(SIGPIPE, sig_handler); /* reset it to this function */
    	CcspTraceWarning(("SIGPIPE received!\n"));
    }
    else {
    	/* get stack trace first */
    	_print_stack_backtrace();
    	CcspTraceError(("Signal %d received, exiting!\n", sig));
    	exit(0);
    }

}

#endif

int main(int argc, char* argv[])
{
    ANSC_STATUS                     returnStatus       = ANSC_STATUS_SUCCESS;
    int                             cmdChar            = 0;
    BOOL                            bRunAsDaemon       = TRUE;
    int                             idx                = 0;

#if defined(_DEBUG) && defined(_COSA_SIM_)
    AnscSetTraceLevel(CCSP_TRACE_LEVEL_INFO);
#endif

    for (idx = 1; idx < argc; idx++)
    {
        if ( (strcmp(argv[idx], "-subsys") == 0) )
        {
            AnscCopyString(g_Subsystem, argv[idx+1]);
        }
        else if ( strcmp(argv[idx], "-c") == 0 )
        {
            bRunAsDaemon = FALSE;
        }
    }

    /* Set the global pComponentName */
    pComponentName = CCSP_COMPONENT_NAME_TAD;

#ifdef   _DEBUG
    /*AnscSetTraceLevel(CCSP_TRACE_LEVEL_INFO);*/
#endif

#if  defined(_ANSC_WINDOWSNT)

    AnscStartupSocketWrapper(NULL);

    display_info();

    cmd_dispatch('e');

    while ( cmdChar != 'q' )
    {
        cmdChar = getchar();

        cmd_dispatch(cmdChar);
    }
#elif defined(_ANSC_LINUX)
    if ( bRunAsDaemon )
        daemonize();

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    /*signal(SIGCHLD, sig_handler);*/
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGPIPE, SIG_IGN);

//    if (write_pid_file("/var/tmp/CcspTandDSsp.pid") != 0)
//        fprintf(stderr, "%s: fail to write PID file\n", argv[0]);

    cmd_dispatch('e');

    if ( bRunAsDaemon )
    {
        while(1)
        {
            sleep(30);
        }
    }
    else
    {
        while ( cmdChar != 'q' )
        {
            cmdChar = getchar();

            sleep(30);
            cmd_dispatch(cmdChar);
        }
    }
#endif

    if ( g_bActive )
    {
        ssp_cancel_tad();

        g_bActive = FALSE;
    }

    return 0;
}


