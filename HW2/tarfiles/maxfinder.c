/*
*	Name: 		Jonathan Colen
*	Email:		jc8kf@virginia.edu
*	Class:		CS 4414
*	Professor:	Andrew Grimshaw
*	Assignment:	Machine Problem 2
*
*	The purpose of this program is to demonstrate correct implementation of the Barrier class
*	using binary semaphores. The Barrier implementation is used to perform a multi-threaded 
*	max-finding routine in an array of numbers input via standard in. The array should be fed
*	in as a series of numbers separated by new lines and the output will be the maximum number
*	in the array.
*
*	This program can be compiled via
*		gcc -pthread -o max maxfinder.c -lm
*	And can be run via
*		./max
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

/*
*	Structure for a singly linked list of floats
*	This structure was used to simplify input parsing
*/
typedef struct int_node	{
	float val;
	struct int_node * next;
} Inode;

/*
*	Structure containing the necessary variables for implementing a barrier.
*	The barrier can be passed as an argument to bar_wait. The first N-1 threads
*	to call bar_wait will block, and the Nth wait call will free all of them.
*/
typedef struct barrier	{
	int N;				//The Nth wait call will cause all processes blocking to resume
	int count;			//Stores the number of threads which have called wait
	sem_t * waitq;		//POSIX semaphore (used as binary) that causes processes to block as desired
	sem_t * mutex;		//Binary semaphore controlling access to count variable
	sem_t * throttle;	//Used to ensure signalling (Nth) process knows it has signaled correctly
} Barrier;

/*
*	Structure passed as an argument in pthread_create. Contains a pointer to the array 
*	being operated on by all threads, the length of the array, the index that particular
*	thread is responsible for, and a pointer to the Barrier struct being used.
*/
typedef struct arg_struct	{
	float * array;	//Pointer to the array being searched
	int N;			//Stores the length of the array
	int index;		//Stores the array index the thread is responsible for comparing/storing into
	Barrier * bar;	//Pointer to the barrier
} Argstruct;

/*
*	Initializes the Barrier with all internal variables set to correct values.
*	@param n - the value to be stored in N
*/
void bar_init(Barrier * bar, int n)	{
	bar->N = n;			//Set the N value of the barrier to the integer argument
	bar->count = 0;		//At first, no processes have called bar_wait
	bar->waitq = (sem_t*)malloc(sizeof(sem_t));		
	bar->mutex = (sem_t*)malloc(sizeof(sem_t));
	bar->throttle = (sem_t*)malloc(sizeof(sem_t));
	sem_init(bar->waitq, 0, 0);		//waitq is initialized to zero so the first process blocks
	sem_init(bar->mutex, 0, 1);		//mutex is initialized to one so the first process can access count
	sem_init(bar->throttle, 0, 0);	//throttle is initialized to zero so the signalling process blocks
}

/*
*	The first N-1 processes calling bar_wait will block. The Nth process will signal N-1
*	times allowing all processes to resume. This is used to ensure all threads are coordinated
*	to be running as expected as a given time (for example, to ensure all threads are in the
*	same round).
*/
void bar_wait(Barrier * bar)	{
	sem_wait(bar->mutex);		//Wait on mutex before accessing count
	(bar->count) ++;			//Increment the number of processes that have called count
	if(bar->count == bar->N)	{			//Signal N-1 processes since the Nth process is still running
		int i;
		for(i = 0; i < (bar->N)-1; i ++)	{
			sem_post(bar->waitq);		//Signal to allow a waiting process to resume
			sem_wait(bar->throttle);	//Ensure the process resumed by waiting on throttle
		}
		bar->count = 0;			//Reset count to zero
		sem_post(bar->mutex);	//Signal mutex so other processes can access it
	} else	{
		sem_post(bar->mutex);		//Release lock on count
		sem_wait(bar->waitq);		//Set process to wait
		sem_post(bar->throttle);	//Signal to freeing process that it has been freed
	}
}

/*
*	Flatten a linked list into an array. Useful here since the array gives O[1] access
*	for each thread rather than O[n] access for a linked list.
*/
void ll_to_array(float * arr, Inode * ll)	{
	Inode * head = ll;
	int count = 0;
	while (head)	{
		arr[count] = head->val;
		count ++;
		head = head->next;
	}

}

/*
*	Accept input until an empty line is reached. Each line contains an input number.
*/
int load_input(Inode * ll)	{
	int input_length = 100;		//No number should be longer than this, right?
	char input[input_length];
	float val;					//Stores the numerical input values
	int count = 0;				//Keeps track of how many values have been read in
	Inode * curr = ll;			//Pointer to the current linked list node

	while(1)	{
		if(fgets(input, sizeof input, stdin) == NULL)	{	//If EOF, return
			return count;
		}
		if(sscanf(input, "%f", &val)  < 1 || input[0] == '\n')	{
			return count;	//Return if pattern not matched or empty line is found
		}
		
		curr->val = val;								//Store value in ll-node
		curr->next = (Inode*)malloc(sizeof(Inode));		//Allocate next ll-node
		curr = curr->next;								//Move to next ll-node
		
		count ++;	//Increment the number of values in the linked list
	}
}

/*
*	Function called by each thread to find the maximum value in the array.
*	arg is a void pointer to an Argstruct which contains the necessary information for each thread
*/
void * find_max_thread(void * arg)	{
	Argstruct * a = (Argstruct*) arg;

	int level = log2(a->N);		//Each thread runs log2(N) levels
	int cmp_offset = 1;			//Index offset of the value to compare to, starts at 1
	int i;
	for(i = 1; i <= level; i ++)	{	//Start at level 1. Solution exists at level = level (last one)
		//In round 1, all threads are active. In round 2, threads 0, 4, 8 are active. 
		//The pattern is that in round n, threads that satisfy index = k*2^n where k is an integer
		//are active.
		if ((a->index) % ((int)pow(2, i)) == 0)	{	//Checks if the thread is active in this round
			int cmp = a->index + cmp_offset;		//Determine the index that should be compared
			if ((a->array)[a->index] < (a->array)[cmp])	//Put the bigger of the two in index	
				(a->array)[a->index] = (a->array)[cmp];
		}
		
		cmp_offset <<= 1;	//Double the offset for the next round
		bar_wait(a->bar);	//Barrier wait whether this thread is useful this round or not
	}

	pthread_exit((void*)a);	//Exit the thread
}

/*
*	Multi-threaded max-finding function. Generates count/2 threads operating on array.
*	After all threads have finished, the value will be stored in array[0], which will
*	be returned.
*/
float find_max(float * array, int count)	{
	Argstruct args[count / 2];	//Argument structures for each thread
	pthread_t tids[count / 2];	//IDs for each thread to be used in pthread_join
	pthread_attr_t attr;		//Attributes for the threads

	Barrier * bar = (Barrier*)malloc(sizeof(Barrier));	//Allocate barrier
	bar_init(bar, count / 2);							//Initialize barrier
	pthread_attr_init(&attr);							//Initialzie attributes

	int i;
	for(i = 0; i < count / 2; i ++)	{
		args[i].array = array;	//Set pointer to the array
		args[i].N = count;		//Set the number of values in the array
		args[i].index = 2 * i;	//Start by comparing each subsequent pair, so each thread checks 2*i
		args[i].bar = bar;		//Set pointer to the barrier
		pthread_create(&tids[i], &attr, find_max_thread, (void*)&args[i]);
	}

	for(i = 0; i < count / 2; i ++)	{
		pthread_join(tids[i], NULL);
	}

	return array[0];	//Return the maximum value which is stored in array[0]
}

/*
*	Main method. Reads and parses input, finds maximum, and prints max value to stdout.
*/
int main()	{
	int count = 0;		//Stores the number of values in the list
	Inode * input = (Inode*)malloc(sizeof(Inode));	//Linked list for input reading

	count = load_input(input);	//Load input into linked list
	float * array = (float*)malloc(sizeof(float) * count);	//Allocate necessary memory for array
	ll_to_array(array, input);	//Flatten linked list into array
	//Free the Inodes now
	Inode * tmp = input;
	while(input)	{
		tmp = input->next;
		free(input);
		input = tmp;
	}

	//Find max and print it out
	float max = find_max(array, count);
	printf("%f\n", max);

	return 0;
}
