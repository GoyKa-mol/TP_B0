#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "acquisitionManager.h"
#include "msg.h"
#include "iSensor.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"


//producer count storage
volatile unsigned int produceCount = 0;

pthread_t producers[4];

static void *produce(void *params);
/**
* Buffer interface
*/
#define BUFFER_SIZE 16
MSG_BLOCK Buffer[BUFFER_SIZE];
unsigned int Tlib[BUFFER_SIZE];
unsigned int Tocc[BUFFER_SIZE];
unsigned int i_lib = 0;
unsigned int i_occ = 0;
unsigned int j_lib = 0;
unsigned int j_occ = 0;


/**
* Semaphores and Mutex
*/
#define Slib_INITIAL_VALUE BUFFER_SIZE
#define Slib_NAME "/Slib_sem"
#define Socc_INITIAL_VALUE 0
#define Socc_NAME "/Socc_sem"
sem_t *Slib;
sem_t *Socc;
pthread_mutex_t Com1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Com2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Com3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Com4= PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_producedCount= PTHREAD_MUTEX_INITIALIZER;
/*
* Creates the synchronization elements.
* @return ERROR_SUCCESS if the init is ok, ERROR_INIT otherwise
*/
static unsigned int createSynchronizationObjects(void);

/*
* Increments the produce count.
*/
static void incrementProducedCount(void);

static unsigned int createSynchronizationObjects(void)
{
	//Initialisation du semaphore Slib
	Slib = sem_open(Slib_NAME, O_CREAT, 0644, Slib_INITIAL_VALUE);
    if (Slib == SEM_FAILED)
    {
        perror("[sem_open Slib");
        return ERROR_INIT;
    }
	//Initialisation du semaphore Socc
	Slib = sem_open(Socc_NAME, O_CREAT, 0644, Socc_INITIAL_VALUE);
    if (Socc == SEM_FAILED)
    {
        perror("[sem_open Socc");
        return ERROR_INIT;
    }
	//Initialisation des tableau de suivi d'indice Tlib et Tocc
	for(unsigned int i = 0; i<BUFFER_SIZE;i++){
		Tlib[i] = i;
		Tocc[i] = i;
	}
	printf("[acquisitionManager]Semaphore created\n");
	return ERROR_SUCCESS;
}

static void incrementProducedCount(void)
{
	pthread_mutex_lock(&mutex_producedCount);
	produceCount++;
	pthread_mutex_unlock(&mutex_producedCount);
}

unsigned int getProducedCount(void)
{
	unsigned int p = 0;
	pthread_mutex_lock(&mutex_producedCount);
	p = produceCount;
	pthread_mutex_unlock(&mutex_producedCount);
	return p;
}

MSG_BLOCK getMessage(void){
	unsigned int j
	MSG_BLOCK message

	sem_wait(Socc);
	pthread_mutex_lock(&Com3);
	j = Tocc[j_occ];
	j_occ = (j_occ + 1)%BUFFER_SIZE;
	pthread_mutex_unlock(&Com3);
	message.checksum = Buffer[j].checksum;
	for(int k=0;k<DATA_SIZE;k++){
		message.mData[k] = Buffer[j].mData[k];
	}
	pthread_mutex_lock(&Com4);
	Tlib[j_lib] = j;
	j_lib = (j_lib + 1)%BUFFER_SIZE;
	pthread_mutex_unlock(&Com4);
	sem_post(Slib);

	return message;
}

//TODO create accessors to limit semaphore and mutex usage outside of this C module. ????

unsigned int acquisitionManagerInit(void)
{
	unsigned int i;
	printf("[acquisitionManager]Synchronization initialization in progress...\n");
	fflush( stdout );
	if (createSynchronizationObjects() == ERROR_INIT)
		return ERROR_INIT;
	
	printf("[acquisitionManager]Synchronization initialization done.\n");

	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_create(&producers[i], NULL, produce, NULL);
	}
	return ERROR_SUCCESS;
}

void acquisitionManagerJoin(void)
{
	unsigned int i;
	for (i = 0; i < PRODUCER_COUNT; i++)
	{
		pthread_join(&producers[i], NULL);
	}
	sem_destroy(Slib);
	sem_destroy(Socc);
	printf("[acquisitionManager]Semaphore cleaned\n");
}

void *produce(void* params)
{
	D(printf("[acquisitionManager]Producer created with id %d\n", gettid()));
	unsigned int k = 0;
	unsinged int check;
	MSG_BLOCK inputMessage;
	while (k < PRODUCER_LOOP_LIMIT)
	{
		k++;
		sleep(PRODUCER_SLEEP_TIME+(rand() % 5));
		//Reception du message
		getInput(k, inputMessage);
		//SanityCheck
		check = messageCheck(inputMessage);
		if(check){
			sem_wait(Slib);
			pthread_mutex_lock(&Com1);
			i = Tlib[i_lib];
			i_lib = (i_lib + 1)%BUFFER_SIZE;
			pthread_mutex_unlock(&Com1);
			Buffer[i].checksum = inputMessage.checksum;
			for(int l=0;l<DATA_SIZE;l++){
				Buffer[i].mData[l] = inputMessage.mData[l];
			}
			pthread_mutex_lock(&Com2);
			Tocc[i_occ] = i;
			i_occ = (i_occ + 1)%BUFFER_SIZE;
			incrementProducedCount()
			pthread_mutex_unlock(&Com2);
			sem_post(Slib);
		}
		
	}
	printf("[acquisitionManager] %d termination\n", gettid());
	pthread_exit(NULL);
}