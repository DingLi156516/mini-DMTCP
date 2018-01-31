#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<ucontext.h>
#include<sys/mman.h>
//define the start address
#ifndef ADDRESS
#define ADDRESS 0x5300000
#endif
//define where should the new stack pointer be
#ifndef STACKPTR
#define STACKPTR 0x5400000
#endif
//define the new stack length
#ifndef LENGTH
#define LENGTH 0x100000
#endif
//define the path which can 
#ifndef CKPT_NAME
#define CKPT_NAME "/proc/self/maps"
#endif
/* define the struct that read stored header information*/
typedef struct section_header
{
	void *start;
	void *end;
	unsigned long size;
	char permission[4];
	int context;
	
} Header;
char ckpt_image[1000];
ucontext_t context;
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
/* read to the end of the line and save necessary information for identify the "stack"*/
void finishline(int fd, char* data){
	char judge = ' ';
	while(judge != '\n'){
		if(read(fd, &judge, 1) < 0){
			error();
		}
		//find data with"[]" and store it
		else if(judge == '['){
			while(judge != ']'){
				read(fd, &judge, 1);
				*data = judge;
				data++;
			}
		}
	}
}
/*The function that will restore the saved headers and data in each address range */
void restore_memory(){
	char data[1000];
	char permission[4];
	char onechar;
	Header header;
	Header header2;
	int self_fd = open(CKPT_NAME, O_RDONLY);
	if(self_fd < 0){
		printf("openself\n");
	}
	unsigned long size;
	// while loop that find the [stack]
	while(read(self_fd, &onechar, 1)){
		lseek(self_fd, -1, SEEK_CUR);
		readhex(self_fd, &header.start);
		readhex(self_fd, &header.end);
		if(read(self_fd, &permission, 4) < 0){
			error();
		}
		strcpy(header.permission, permission);
		finishline(self_fd,data);
		unsigned long l =(void *)header.end - (void *)header.start;
		if(header.permission[0]=='r' && (strstr(data,"stack") != NULL)){
			size = l;
			printf("%d\n", l);
			//printf("%s\n",header.permission );
			printf("%p-%p\n", header.start, header.end);
			break;
		}
		memset(data, '\0', 1000);
	}
	close(self_fd);
	printf("unmap the current stack\n");
	size_t length = size; 
	//unamp the current stack
	void *unmapAddr = (void *)header.start;
	if(munmap(unmapAddr, length) != 0){
		printf("munmap\n");
	}
	printf("end ummap.\n");
	// open myckpt file
	int fd = open(ckpt_image, O_RDONLY);
	if(fd < 0){
		printf("openmyckpt\n");
	}
	//loop the whole myckpt file
	while(read(fd, &header2, sizeof(Header)) > 0){
	//printf("%p-%p\n",header2.start,header2.end);
	//printf("%lx\n", header2.size);
	//find the context
	if(header2.context == 1){
		if(read(fd, &context, sizeof(ucontext_t)) < 0){
			printf("readcontext\n");
		}
	}
	// determine which file is readbale, writeable and executable.
	int prot = PROT_WRITE;
	if (header2.permission[0] == 'r') {
		prot |= PROT_READ;
	}
	if (header2.permission[1] == 'w') {
		prot |= PROT_WRITE;
	}
	if (header2.permission[2] == 'x') {
		prot |= PROT_EXEC;
	}
        unsigned long count = (void *)header2.end - (void *)header2.start;
	//assign address range for the data
        mmap(header2.start, header2.size, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        //read data into that address range
        if(read(fd, header2.start, count) < 0){
        	//printf("read data\n");
        }
	}
	close(fd);
	printf("begin restore context.\n");
	if(setcontext(&context) < 0){
			error();
	}
	
}
/*main function which assign new stack and start restore the information*/
void main(int argc, char const *argv[])
{
	
	if (argc == 1)
	{
		printf("No file name, please enter a file name\n");
		
	}
	//get file name
	strcpy(ckpt_image, argv[1]);
	long startaddr = ADDRESS;
    	void *stackstart = (void *)startaddr;
    	size_t stacksize = LENGTH;
	void *stackptr = (void *)STACKPTR;
	//assign new stack
	void *map = mmap(stackstart, stacksize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
	if (map == MAP_FAILED) {
      		perror("mmap");
      		exit(1);
    	}
	//switch stack pointer
	asm volatile ("mov %0,%%rsp" : : "g" (stackptr) : "memory");
	restore_memory();
}

