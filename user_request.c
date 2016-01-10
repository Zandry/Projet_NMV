#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>


#define NMV_KILL _IOR('N',0,void*)
#define NMV_WAIT _IOR('N',1,void*)
#define NMV_LSMOD _IOW('N',2,void*)
#define NMV_MEMINFO _IOW('N',3,void*)
#define BUFFER_SIZE 1024


struct kill_event {
	int signal; 
	pid_t pid_nr; 
	bool syn; 
	/*
		true: synchrone
		false: asynchrone
	*/
};

int main(int argc, char ** argv)
{
	int f = open("/dev/projet_nmv", O_RDONLY);
	char request[1024];
	char response[1024];
	unsigned long pid = 0;
	struct kill_event my_kill_event;
	const char* CLEAR_SCREE_ANSI = "\e[1;1H\e[2J";
	printf("\t\tWelcome to our Linux Kernel Command Simulator (LKCS)\n");
	printf("\t\tAuthors: Minh-Hieu PHAM and Jonathan Espie--Caullet\n");
	printf("\t\t\tMaster SAR - UPMC 2016\n\n");
	printf("Command simulation: \n");
	printf("1. Send a signal to a process in asynchronization mode: kill & [pid] [signal]\n");
	printf("2. Send a signal to a process in synchronization mode: kill [pid] [signal]\n");
	printf("3. List all loaded modules in kernel: lsmod\n");
	printf("4. Display current information about memory: meminfo\n");
	while (1)
	{
		printf("Please enter your command (Enter quit to exit simulator): ");
		scanf("%s", request);
		if (strncmp(request, "kill &", 6) == 0){
			printf("Enter Pid number: ");
			scanf("%d", &(my_kill_event.pid_nr));
			printf("Enter signal number: ");
			scanf("%d", &(my_kill_event.signal));
			my_kill_event.syn = false; 
			ioctl(f, NMV_KILL, &my_kill_event);
		}else if (strncmp(request, "kill", 4) == 0){
			scanf("%d", &(my_kill_event.pid_nr));
			scanf("%d", &(my_kill_event.signal));
			my_kill_event.syn = true;
			ioctl(f, NMV_KILL, &my_kill_event); 
		}else if (strncmp(request, "lsmod", 5) == 0){
			ioctl(f, NMV_LSMOD, &response);
			printf("->List of loaded modules: \n%s", response);
		}else if(strncmp(request, "meminfo", 7) == 0){
			ioctl(f, NMV_MEMINFO, &response);
			printf("->Meminfo:\n%s", response);
		}else if (strncmp(request, "quit", 4) == 0){
			break;	
		}else{
			printf("Bad command, please try again!\n");
		}
	}
	printf("Have a nice day and See you next time!\n");
	return 0;
}
