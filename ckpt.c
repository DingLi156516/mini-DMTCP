#include<stdio.h>
#include<ucontext.h>
#include<signal.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<setjmp.h>
/* define the struct that store header information*/
typedef struct section_header
{
	void *start;
	void *end;
	unsigned long size;
	char permission[4];
	//judge if it is context
	int context;	
} Header;

/* save the context and wait for use*/
void savecontext(int fd, ucontext_t* context) {
	Header header;
	header.start = NULL;
	header.end = NULL;
	header.permission[0] = '-';
	header.permission[1] = '-';
	header.permission[2] = '-';
	header.permission[3] = '-';
	header.size = sizeof(*context);
	header.context = 1;
	// write a file header for context
	if (write(fd, &header, sizeof(header)) < 0) {
		//printf("write error in context header\n");
	}
	//store context
	if (write(fd, context, sizeof(*context)) < 0) {
		//printf("write error in context body\n");
	}
}
/* an error function that will print error message*/
void error(){
	printf("An error occured, please try again!\n");
	exit(1);
}
/* read the hex numbers and convert it*/
void readhex (int fd, void **value){
	char c;
	unsigned long int v;

  	v = 0;
  	while (1) {
   		if(read(fd, &c, 1) < 0){
   			error();
   		}
    	if ((c >= '0') && (c <= '9')) c -= '0';
    	else if ((c >= 'a') && (c <= 'f')) c -= 'a' - 10;
    	else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
   		else break;
    	v = v * 16 + c;
  	}
  	*value = (void*)v;
}
/* read to the end of the line*/
void finishline(int fd){
	char judge = ' ';
	while(judge != '\n'){
		if(read(fd, &judge, 1) < 0){
			error();
		}
	}
}
/* function that will receive the signal and create a checkpiont image*/
void signalhandler(){
	printf("Creating CheckPoint Image....\n");
	char onechar;
	Header header;
	char permission[4];
	//open the file
	int fd = open("/proc/self/maps", O_RDONLY);
	int output = open("./myckpt", O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if(fd < 0 || output < 0){
		error();
	}
	printf("Restore process...\n");
	//loop the whole file
	while(read(fd, &onechar, 1)){
		lseek(fd, -1, SEEK_CUR);
		readhex(fd, &header.start);
		readhex(fd, &header.end);
		if(read(fd, &permission, 4) < 0){
			error();
		}
		strcpy(header.permission, permission);
		finishline(fd);
		header.size =header.end - header.start;
		header.context = 0;
		//if the file is writeable, write data to the output file
		if(header.permission[0] == 'r'){
			if(write(output, &header, sizeof(header)) < 0){
				//printf("write\n");
			}
			if(write(output, header.start, header.size) < 0){
				//printf("write2\n");
			}
		}
	}
	ucontext_t context;
	if (getcontext(&context) < 0) {
		printf("Error in getcontext.\n");
	}
	//save the context we got.
	savecontext(output, &context);
	close(fd);
	close(output);

}
__attribute__((constructor))
void myconstructor() {
	signal(SIGUSR2,signalhandler);
}

