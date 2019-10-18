#include<stdlib.h>
#include<string.h>
#include <unistd.h>
int main()
{
	while(1)
	{
		//Increased memory allocation size for faster memory fill up
		char *ptr = (char*)malloc(100 * 1024);
		memset( ptr, 0xF , 100*1024 );
		//Reduced sleep timer for faster memory fill up
		usleep( 1500 );
	}
	return 0;
}

