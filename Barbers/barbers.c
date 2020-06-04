#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/shm.h>


// number of barbers
#define MALE_BARBERS 3
#define FEMALE_BARBERS 2
#define UNISEX_BARBERS 4
#define CLIENTS 20

//genders
#define FEMALE 0
#define MALE 1
#define UNISEX 2
// queue of people
#define QUEUE 13


// cut time
#define CUT_TIME 2
#define HAIRGROWTH_TIME 5

// functions used
void barber(int gender, int id);
void customer(int gender, int id);

// lock/unlock operations
void lock_wr();
void unlock_wr();
void unlock_customer(int gender);
void lock_customer(int gender);
void unlock_barber(int gender);
void lock_barber(int gender);

//shared memory operations
struct shared_memory* get_memory();
void release_memory(struct shared_memory* shm);

// keys
const key_t WR_KEY = 0x1060;
const key_t CUSTOMER_KEY = 0x1123;
const key_t BARBERS_KEY = 0x2260;
const key_t SHARED_MEM_KEY = 0x6060;

const char* sex[3] = {"Female", "Male", "Unisex"};

void terminate(unsigned int index_of_created_process, pid_t *processes_list);

struct shared_memory 
{
	int waiting_male_clients;
	int waiting_female_clients;
};

struct semaphores
{
	int customers;
	int barbers;
	int waiting_room;
};



//access semaphores
void semaphores_get(struct semaphores* sem);

int main(int argc, char** argv) 
{
	printf("Barber shop opens!\n");
	pid_t processesList[MALE_BARBERS + FEMALE_BARBERS + UNISEX_BARBERS + CLIENTS];


	int shm_id = shmget(SHARED_MEM_KEY, sizeof(struct shared_memory), 0666 | IPC_CREAT);
	if (shm_id < 0)
	{
		perror("Error shmget");
		exit(1);
	}

	struct shared_memory* shm = shmat(shm_id, NULL, 0);
	if(shm == (void*)-1)
	{
		perror("Error attaching memory");
		exit(1);
	}
	
	//initialize variables to empty waiting room
	shm->waiting_female_clients = 0;
	shm->waiting_male_clients = 0;

	//create sempahores
	struct semaphores sem;
	sem.barbers = semget(BARBERS_KEY, 3,0666 | IPC_CREAT);
	sem.customers = semget(CUSTOMER_KEY, 2, 0666 | IPC_CREAT);
	sem.waiting_room = semget(WR_KEY, 1, 0666 | IPC_CREAT);

	if (sem.customers < 0 || sem.barbers < 0 || sem.waiting_room < 0)
	{
		perror("Error getting semaphores");
	}


	union semun
	{
		int val;
		unsigned int* array;
	} sem_un;

	//setting initial value for waiting room mutex
	sem_un.val = 1;
	if(semctl(sem.waiting_room, 0, SETVAL, sem_un) < 0)
	{
		perror("Error semclt waitingroom");
		exit(1);
	}	

	//setting initial values for barbers semaphore
	sem_un.val = 0;
	if(semctl(sem.barbers, FEMALE, SETVAL, sem_un) < 0)
	{
		perror("Error semclt female barbers");
		exit(1);
	}

	sem_un.val = 0;
	if(semctl(sem.barbers, MALE, SETVAL, sem_un) < 0)
	{
		perror("Error semclt male barbers");
		exit(1);
	}
	
	sem_un.val = 0;
	if(semctl(sem.barbers, UNISEX, SETVAL, sem_un) < 0)
	{
		perror("Error semclt unisex barbers");
		exit(1);
	}	

	//setting initial values for semaphores of customers
	sem_un.val = 0;
	if(semctl(sem.customers, MALE, SETVAL, sem_un) < 0)
	{
		perror("Error semclt male customer");
		exit(1);
	}	

	sem_un.val = 0;
	if(semctl(sem.customers, FEMALE, SETVAL, sem_un) < 0)
	{
		perror("Error semclt FEMALE customer");
		exit(1);
	}	

	pid_t pid;
	int all_barbers = MALE_BARBERS + FEMALE_BARBERS + UNISEX_BARBERS;

	for(int i = 0; i < all_barbers; ++i)
	{
		pid = fork();
		if(pid < 0)
		{
			perror("Error creating child");
			terminate(i-1, processesList);
		}
		else if(pid > 0)
		{
			processesList[i] = pid;
		}
		else
		{
			if(i < FEMALE_BARBERS)
				barber(FEMALE,i);
			else if ( i < FEMALE_BARBERS + MALE_BARBERS)
				barber(MALE,i - FEMALE_BARBERS);
			else
				barber(UNISEX, i-FEMALE_BARBERS-MALE_BARBERS);
			
			return 0;
		}
	}

	int gender;

	for(int i = 0; i < CLIENTS; ++i)
	{
		pid = fork();

		if(pid < 0)
		{
			perror("Error creating child");
			terminate(i-1, processesList);
		}
		else if(pid > 0)
		{
			processesList[i] = pid;
		}
		else
		{
			srand(time(NULL));
			while (1)
			{
				gender = rand()%2;
				customer(gender,i);
				sleep(HAIRGROWTH_TIME);
			}

			return 0;
			
		}
	}


	sleep(60);

	printf("Closing barbershop!\n");
	terminate(all_barbers + CLIENTS - 1, processesList);

	release_memory(shm);

}

void barber(int gender, int id)
{
	struct shared_memory* shm = get_memory();
	printf("%s barber: [%d] created\n",sex[gender],id);
	int client_gender;
	while(1)
	{
		switch (gender)
		{
		case FEMALE:
			lock_customer(gender);
			lock_wr();

			shm->waiting_female_clients--;

			unlock_barber(gender);
			unlock_wr();
			printf("%s barber [%d] cutting hair.....\n",sex[gender],id);
			sleep(CUT_TIME);
			break;
		
		case MALE:
			lock_customer(gender);
			lock_wr();

			shm->waiting_male_clients--;

			unlock_barber(gender);
			unlock_wr();
			sleep(CUT_TIME);
			printf("%s barber [%d] cutting hair.....\n",sex[gender],id);
			break;
		
		case UNISEX:
			
			if (shm->waiting_female_clients >= shm->waiting_male_clients)
			{
				client_gender = FEMALE;
			}
			else
			{
				client_gender = MALE;
			}

			lock_customer(client_gender);
			lock_wr();

			if (gender == FEMALE)
				shm->waiting_female_clients--;
			else
				shm->waiting_male_clients--;
			
			unlock_barber(gender);
			unlock_wr();
			printf("%s barber [%d] cutting hair.....\n",sex[gender],id);
			sleep(CUT_TIME);
			break;
		}
	}
	release_memory(shm);
}

void customer(int gender, int id)
{
	struct shared_memory* shm = get_memory();
	struct semaphores sem;
	semaphores_get(&sem);
	printf("%s client: [%d] entered waiting room\n", sex[gender] , id);
	lock_wr();
	int barber_gender;
	if((shm->waiting_female_clients + shm->waiting_male_clients) < QUEUE)
	{
		switch (gender)
		{
		case FEMALE:
			shm->waiting_female_clients++;
			unlock_customer(gender);
			unlock_wr();

			if(semctl(sem.barbers, UNISEX, GETVAL) >= semctl(sem.barbers, FEMALE, GETVAL))
				barber_gender = UNISEX;
			else
				barber_gender = FEMALE;
			
			lock_barber(barber_gender);
			printf("%s client: [%d] ready to be served\n",sex[gender],id);
			break;
		case MALE:
			shm->waiting_male_clients++;
			unlock_customer(gender);
			unlock_wr();

			if(semctl(sem.barbers, UNISEX, GETVAL) >= semctl(sem.barbers, MALE, GETVAL))
				barber_gender = UNISEX;
			else
				barber_gender = MALE;

			lock_barber(barber_gender);
			printf("%s client: [%d] ready to be served\n",sex[gender],id);
			break;
		}

	}
	else
	{
		unlock_wr();
		printf("Client: [%d] Waiting room full, leaving the barbershop\n",id);
	}
	
	release_memory(shm);
}


struct shared_memory* get_memory()
{
	int shm_id = shmget(SHARED_MEM_KEY, sizeof(struct shared_memory), 0666);
	if (shm_id < 0)
	{
		perror("Error getting memory");
		exit(1);
	}

	struct shared_memory* shm;
	shm = shmat(shm_id, NULL, 0);
	if(shm == (void*)-1)
	{
		perror("Error attaching memory");
		exit(1);
	}

	return shm;
}

void release_memory(struct shared_memory* shm)
{
	if(shmdt(shm) < 0)
	{
		perror("Error detaching memory");
	}
	return;
}

void semaphores_get(struct semaphores* sem)
{
	sem->customers = semget(CUSTOMER_KEY,2,0666);
	sem->barbers = semget(BARBERS_KEY,3, 0666);
	sem->waiting_room = semget(WR_KEY,1,0666);
	if(sem->customers < 0 || sem->barbers < 0 || sem->waiting_room < 0)
		perror("error getting semaphores");
	
	return;

}

void unlock_wr()
{
	struct sembuf v = {0,+1,SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.waiting_room, &v, 1) < 0)
	{
		perror("Semop wr unlock err");
		exit(1);
	}
}

void lock_wr()
{
	struct sembuf p = {0, -1, SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.waiting_room,&p,1) < 0)
	{
		perror("Semop wr lock err");
		exit(1);
	}
}

void unlock_customer(int gender)
{
	struct sembuf v = {gender,+1,SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.customers, &v, 1) < 0)
	{
		perror("Semop customer unlock err");
		exit(1);
	}
}



void lock_customer(int gender)
{
	struct sembuf p = {gender, -1, SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.customers,&p,1) < 0)
	{
		perror("Semop customer lock err");
		exit(1);
	}
}

void unlock_barber(int gender)
{
	struct sembuf v = {gender,+1,SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.barbers, &v, 1) < 0)
	{
		perror("Semop barber unlock err");
		exit(1);
	}
}


void lock_barber(int gender)
{
	struct sembuf p = {gender, -1, SEM_UNDO};
	struct semaphores sems;
	semaphores_get(&sems);
	if(semop(sems.barbers,&p,1) < 0)
	{
		perror("Semop lock err");
		exit(1);
	}
}


void terminate(unsigned int index_of_created_process, pid_t *processes_list)
{
	for(int i = index_of_created_process; i >= 0; i--)
	{
		kill(processes_list[i],SIGTERM);
	}
}

