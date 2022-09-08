#include "ImagehealthChecker.h"
void report_t2(char * event,char type,char *val)
{
    if (type == 's')
    {
        if(T2ERROR_SUCCESS != t2_event_s(event, val))
        {
            IHC_PRINT("%s T2 send Failed\n",__FUNCTION__);
        }
    }
    if (type == 'd')
    {
        if(T2ERROR_SUCCESS != t2_event_d(event, atoi(val)))
        {
            IHC_PRINT("%s T2 send Failed\n",__FUNCTION__);
        }
    }
}
int fnd_cli_diff(int old_val,int new_val)
{
    float diff=0.0;
    if(new_val < old_val)
    {
        diff = ((float)new_val / old_val) * 100;
        return (int)diff;
    }
    return -1;
}