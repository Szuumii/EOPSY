#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#define NUM_CHILD 7
#define WITH_SIGNALS



#ifdef WITH_SIGNALS
void sigint_handler(int);
bool keyboard_pressed = false;
void send_sigterm();
void term_child(int);
#endif


void create_child(int);

int number_of_created_kids = 0;
pid_t kids_table[NUM_CHILD];



int main (int argc, char** argv)
{

	int i;
	int temp;
	int processes_exited = 0;
	pid_t terminated_processes_id[NUM_CHILD];
	int exit_status_table[NUM_CHILD];
	int stat;


	#ifdef WITH_SIGNALS
	for(i = 1; i < _NSIG; ++i)
	{
		signal(i,SIG_IGN);
	}
	signal(SIGCHLD,SIG_DFL);
	signal(SIGINT,sigint_handler);
	#endif


	printf("Parent[%d] process created\n", getpid());

	for(i = 0; i < NUM_CHILD; ++i)
	{
		create_child(i);
		sleep(1);

		#ifdef WITH_SIGNALS
		if(keyboard_pressed)
		{
			printf("Parent[%d] keyboard interruption pressed during child creation\n",getpid());
			send_sigterm();
			break;
		}
		#endif
	}
	
	

	while(1)
	{
		temp = wait(&stat); //suspending the call for process until one of kids is terminated

		//((temp == -1) ? break: processes_exited++);	
		if(temp == -1)
			break;
		else
			processes_exited++;
	}
	

	printf("Parent[%d] is saying there are no more kids to be processed\n",getpid());

	printf("Child processes terminations: %d\n",processes_exited);

	#ifdef WITH_SIGNALS   // restore all default signals
	for(i = 1; i < _NSIG; ++i)
	{
		signal(i, SIG_DFL);
	}
	#endif
}


void create_child(int i)
{
	pid_t child_id;
	
	child_id = fork();

	if( child_id < 0)
	{
		perror("Child creation failed\n");
		#ifdef WITH_SIGNALS
		send_sigterm();
		#endif
		exit(1);
	}
	else if (child_id == 0)
	{
		#ifdef WITH_SIGNALS
		signal(SIGINT,SIG_IGN);
		signal(SIGTERM,term_child);
		#endif

		printf("Child[%d] created from parent(%d)\n",getpid(),getppid());
		sleep(10); //100 to w8 10 sec
		printf("Child[%d] execution complete\n",getpid());
		exit(0);
	}
	else
	{
		kids_table[i] = child_id;
		number_of_created_kids++;
	}	
}

#ifdef WITH_SIGNALS
void send_sigterm()
{
	int i;
	for(i = 0; i < NUM_CHILD; ++i)
	{
		if (kids_table[i] != 0)
		{
			printf("Parent[%d] is sending SIGTERM to a child[%d]\n",getpid(), kids_table[i]);
			kill(kids_table[i],SIGTERM);
		}
	}

}


void sigint_handler(int signal)
{
	printf("Parent[%d] keyboard interruption clicked\n",getpid());
	keyboard_pressed = true;
}

void term_child(int signal)
{
	printf("Child[%d] is beeing terminated\n",getpid());
	exit(1);
}

#endif

