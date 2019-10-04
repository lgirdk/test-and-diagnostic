#include<stdio.h>

#include<stdlib.h>

#include<string.h>

int main()

{

        while(1)

        {

                char *ptr = malloc(100);

                strcpy(ptr, "HelloWorld");

        }

        return 0;

}
