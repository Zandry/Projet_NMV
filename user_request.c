#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define NMV_KILL _IOR('N',0,void*)
#define NMV_WAIT _IOR('N',1,void*)
#define NMV_LSMOD _IOW('N',2,void*)
#define NMV_MEMINFO _IOW('N',3,void*)
#define BUFFER_SIZE 1024
#define MAX_PROC_WAIT 100


struct kill_event {
	int signal; 
	pid_t pid_nr; 
	bool syn; 
	/*
		true: synchrone
		false: asynchrone
	*/
};

struct wait_event {
	int nb_pid;
	bool wait_all;
	pid_t * pid_nr;
};

int main(int argc, char ** argv)
{
	int f = open("/dev/projet_nmv", O_RDONLY);
	char request[1024];
	char response[1024];
	unsigned long pid = 0;
	bool syn;
	char * ptr;
	struct kill_event my_kill_event;
	struct wait_event my_wait_event;
	pid_t pid_nr[MAX_PROC_WAIT];//On suppose que l'on peut attendre jusqu'a MAX_PROC_WAIT = 100 processus dans la commande wait(all)
	my_wait_event.pid_nr = pid_nr;
	const char* CLEAR_SCREE_ANSI = "\e[1;1H\e[2J";
	printf("\t\tWelcome to our Linux Kernel Command Simulator (LKCS)\n");
	printf("\t\tAuthors: Minh-Hieu PHAM and Jonathan Espie--Caullet\n");
	printf("\t\t\tMaster SAR - UPMC 2016\n\n");
	printf("Command simulation: \n");
	printf("1. Send a signal to a process in asynchronous mode: kill [pid] [signal] &\n");
	printf("2. Send a signal to a process in synchronous mode: kill [pid] [signal]\n");
	printf("3. List all loaded modules in kernel: lsmod\n");
	printf("4. Display current information about memory: meminfo\n");
	printf("5. Wait for any given process to terminate: wait [pid] [pid ...]\n");
	printf("6. Wait for all given process to terminate: waitall [pid] [pid ...]\n");
	printf("7. Exit the simulator: quit\n");
	while (1)
	{
		printf("Please enter your command : ");
		fgets(request, 1023, stdin);//Lit la commande tapée par l'utilisateur
		syn = !(request[strlen(request)-2] == '&');//Regarde si la commande est lancée en mode asynchrone

		if (strncmp(request, "kill", 4) == 0){
			sscanf(request, "kill %d %d", &(my_kill_event.pid_nr), &(my_kill_event.signal));
			my_kill_event.syn = syn;
			printf("Sending (%s): %d signal to %d process\n", syn ? "sync" : "async", my_kill_event.signal, my_kill_event.pid_nr);
			ioctl(f, NMV_KILL, &my_kill_event); 
		}else if (strncmp(request, "lsmod", 5) == 0){
			ioctl(f, NMV_LSMOD, &response);
			printf("->List of loaded modules: \n%s", response);
		}else if(strncmp(request, "meminfo", 7) == 0){
			ioctl(f, NMV_MEMINFO, &response);
			printf("->Meminfo:\n%s", response);
		}else if (strncmp(request, "quit", 4) == 0){
			break;	
		}else if (strncmp(request, "wait", 4) == 0){
			my_wait_event.nb_pid = 0;
			my_wait_event.wait_all = (strncmp(request, "waitall", 7) == 0);
			printf("Waiting for %s process to terminate :\n", my_wait_event.wait_all ? "all" : "a");
			ptr = strtok (request," ");//Coupe la commande en fonction des espaces
			while (1)
			{
				ptr = strtok (NULL," ");//Donne le morceau suivant de la commande
				if (ptr == NULL || my_wait_event.nb_pid >= MAX_PROC_WAIT || sscanf(ptr, "%d", &(my_wait_event.pid_nr[my_wait_event.nb_pid])) == 0)
					break;
				printf("Pid : %d\n", my_wait_event.pid_nr[my_wait_event.nb_pid]);
				my_wait_event.nb_pid++;
			}
			printf("%d process monitored\n", my_wait_event.nb_pid);
			
			ioctl(f, NMV_WAIT, &my_wait_event);
			printf("finished waiting !\n");
		}else{
			printf("Bad command, please try again!\n");
		}
	}
	printf("Have a nice day and See you next time!\n");
	return 0;
}
