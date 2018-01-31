#include<stdio.h>
#include<unistd.h>
/* simple program that will count from 1, and it will sleep for 1 second per print */
int main(int argc, char const *argv[])
{
	int i = 1;
	while(1){
		printf("%d\n", i);
		fflush(stdout);
		i++;
		sleep(1);
	}
	return 0;
}
