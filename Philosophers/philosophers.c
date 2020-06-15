#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>


//Number of philosophers
#define N	5
#define LEFT	( philo_id + N - 1 ) % N
#define RIGHT	( philo_id + 1 ) % N

#define THINKING 0
#define HUNGRY 1
#define EATING 2

//synchronization operations
void grab_forks( int philo_id );
void put_away_forks( int philo_id );
void test(int philo_id);

//Philosopher actions
void think(int philo_id);
void eat(int philo_id);

//Philosopher function
void* philosopher(void* arg);


pthread_mutex_t	m = PTHREAD_MUTEX_INITIALIZER;
int state [N];
pthread_mutex_t	s[N] = PTHREAD_MUTEX_INITIALIZER;
pthread_t philosophers_threads_id[N];


int main(int argc, char** argv)
{
	int philosophers_id[N];	


	//we start initializing philosophers to thinking and locking mutextes
	for(int i = 0; i < N; ++i)
	{
		philosophers_id[i] = i;
		state[i] = THINKING;
		
		if(pthread_mutex_init(&s[i],NULL) != 0)
		{
			perror("Mutex initialization fail");
			exit(1);
		}

		pthread_mutex_lock(&s[i]);
	}

	//create thread and run a subroutine philosopher on it
	for(int i = 0; i < N; ++i)
	{
		if(pthread_create(&philosophers_threads_id[i], NULL, philosopher, (void*)&philosophers_id[i]) != 0)
		{
			perror("Error Creating thread");
			exit(1);
		}
	}

	sleep(40); //40 sec runtime

	//terminate threadss and destroy philosopher mutexes
	for(int i = 0; i < N; ++i)
	{
		pthread_cancel(philosophers_threads_id[i]);
		pthread_join(philosophers_threads_id[i],NULL);

		pthread_mutex_destroy(&s[i]);
	}

	printf("Programm finised runnning!\n");

	return 0;
}


void* philosopher(void* arg)
{
	if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
	{
		perror("Failed to set cancel type asynchronous");
		exit(1);
	}


	int philo_id = *((int*)arg);

	while(1)
	{
		think(philo_id);
		grab_forks(philo_id);
		eat(philo_id);
		put_away_forks(philo_id);
	}

	return NULL;
}

void think(int philo_id)
{
	printf("Philosopher [%d] is thinking\n",philo_id);
	sleep(2);
}

void eat(int philo_id)
{
	printf("Philosopher [%d] is eating\n",philo_id);
	sleep(2);
}


void grab_forks( int philo_id )
{
	pthread_mutex_lock(&m);
	state[philo_id] = HUNGRY;
	printf("Philosopher [%d] is hungry and is trying to take forks\n",philo_id);
	test(philo_id);
	pthread_mutex_unlock(&m);
	pthread_mutex_lock(&s[philo_id]);
}

void put_away_forks( int philo_id )
{
	pthread_mutex_lock(&m);
	state[philo_id] = THINKING;
	printf("Philosopher [%d] has finished eating and put away his forks\n", philo_id);
	test( LEFT );
	test( RIGHT );
	pthread_mutex_unlock(&m);
}

void test(int philo_id)
{
	if( state[philo_id] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING )
	{
		state[philo_id] = EATING;
		pthread_mutex_unlock(&s[philo_id]);
	}
}