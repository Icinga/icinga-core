#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>


int main()
{
	pid_t pid;
	
	
	switch ( pid = fork() )
	{
        case 0:
                /* Child  */
		sleep(1000000);
		exit(0);
        default:
                /* Parent */
		printf("OK: foo bar baz\n");
         	exit(0);      
        }
} 
