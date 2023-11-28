#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h>
#include <pthread.h>
#include "messageAdder.h"
#include "msg.h"
#include "iMessageAdder.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"

//consumer thread
pthread_t consumer;
//Message computed
volatile MSG_BLOCK out;
//Consumer count storage
volatile unsigned int consumeCount = 0;
//Definition des mutex :
pthread_mutex_t mutex_currentSum = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_consumeCount = PTHREAD_MUTEX_INITIALIZER;


/**
 * Increments the consume count.
 */
static void incrementConsumeCount(void);

/**
 * Consumer entry point.
 */
static void *sum( void *parameters );


MSG_BLOCK getCurrentSum(){
	MSG_BLOCK CurrentSum;
	pthread_mutex_lock(&mutex_CurrentSum);
	currentSum.checksum = out.checksum;
	for(int i=0;i<DATA_SIZE;i++){
		currentSum.mData[i] = out.mData[i];
	}
	pthread_mutex_unlock(&mutex_CurrentSum);
	return currentSum;
}


unsigned int getConsumedCount(){
	unsigned int count;
	pthread_mutex_lock(&mutex_consumeCount);
	count = consumeCount;
	pthread_mutex_unlock(&mutex_consumeCount);
	return count;
}


void messageAdderInit(void){
	out.checksum = 0;
	for (size_t i = 0; i < DATA_SIZE; i++)
	{
		out.mData[i] = 0;
	}
	pthread_create(&consumer, NULL, sum, NULL);
}

void messageAdderJoin(void){
	pthread_join(&consumer, NULL);
}

static void *sum( void *parameters )
{
	D(printf("[messageAdder]Thread created for sum with id %d\n", gettid()));
	unsigned int i = 0;
	MSG_BLOCK receivedMessage;
	while(i<ADDER_LOOP_LIMIT){
		i++;
		sleep(ADDER_SLEEP_TIME);
		receivedMessage = getMessage();
		
		pthread_mutex_lock(&mutex_CurrentSum);
		messageAdd(out, receivedMessage);
		pthread_mutex_lock(&mutex_consumeCount);
		consumeCount++;
		pthread_mutex_unlock(&mutex_consumeCount);
		pthread_mutex_unlock(&mutex_CurrentSum);

	}
	printf("[messageAdder] %d termination\n", gettid());
	pthread_exit(NULL);
}


