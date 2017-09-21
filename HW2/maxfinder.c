#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

typedef struct int_node	{
	int val;
	struct int_node * next;
} Inode;

typedef struct barrier	{
	int N;	//n-1 threads will block, the nth wait call will free all of them
	int count;
	sem_t * waitq;
	sem_t * mutex;
	sem_t * throttle;
} Barrier;

typedef struct arg_struct	{
	int * array;	//Pointer to the array being searched
	int N;			//Stores the length of the array
	int index;		//Stores the index of the current thread NOTE maybe not necessary because tid
	Barrier * bar;	//Pointer to the barrier
} Argstruct;

void bar_init(Barrier * bar, int n)	{
	bar->N = n;
	bar->count = 0;
	bar->waitq = (sem_t*)malloc(sizeof(sem_t));
	bar->mutex = (sem_t*)malloc(sizeof(sem_t));
	bar->throttle = (sem_t*)malloc(sizeof(sem_t));
	sem_init(bar->waitq, 0, 0);
	sem_init(bar->mutex, 0, 1);
	sem_init(bar->throttle, 0, 0);
}

void bar_wait(Barrier * bar)	{
	sem_wait(bar->mutex);
	(bar->count) ++;
	if(bar->count == bar->N)	{			//Signal N-1 processes since the Nth process is still running
		int i;
		for(i = 0; i < (bar->N)-1; i ++)	{
			sem_post(bar->waitq);
			sem_wait(bar->throttle);
		}
		bar->count = 0;
		sem_post(bar->mutex);
	} else	{
		sem_post(bar->mutex);		//Release lock on count
		sem_wait(bar->waitq);		//Set process to wait
		sem_post(bar->throttle);	//Signal to freeing process that it has been freed
	}
}

void ll_to_array(int * arr, Inode * ll)	{
	Inode * head = ll;
	int count = 0;
	while (head)	{
		arr[count] = head->val;
		count ++;
		head = head->next;
	}

}

int load_input(Inode * ll)	{
	int input_length = 100;
	char input[input_length];
	int val;
	int count = 0;
	Inode * head = ll;

	while(1)	{
		if(fgets(input, sizeof input, stdin) == NULL)	{
			return count;
		}
		if(sscanf(input, "%d", &val)  < 1 || input[0] == '\n')	{
			return count;
		}
		
		head->val = val;
		head->next = (Inode*)malloc(sizeof(Inode));
		head = head->next;
		
		count ++;
	}
}

void * find_max_thread(void * arg)	{
	Argstruct * a = (Argstruct*) arg;

	int level = log2(a->N);	//Each thread runs log2(N) levels
	int cmp_offset = 1;			//Index offset of the value to compare to
	int i;
	for(i = 1; i <= level; i ++)	{	//Start at level 1. Solution exists at level = level
		if ((a->index) % ((int)pow(2, i)) == 0)	{	//Do we want this thread modifying the array right now?
			int cmp = a->index + cmp_offset;
			if ((a->array)[a->index] < (a->array)[cmp])	//Put the bigger of the two in index	
				(a->array)[a->index] = (a->array)[cmp];
		}
		
		cmp_offset <<= 1;
		bar_wait(a->bar);	//Barrier wait whether this thread is useful this round or not
	}

	pthread_exit((void*)a);
}

int find_max(int * array, int count)	{
	Argstruct args[count / 2];
	pthread_t tids[count / 2];
	pthread_attr_t attr;

	Barrier * bar = (Barrier*)malloc(sizeof(Barrier));	
	bar_init(bar, count / 2);
	pthread_attr_init(&attr);

	int i;
	for(i = 0; i < count / 2; i ++)	{
		args[i].array = array;
		args[i].N = count;
		args[i].index = 2 * i;	//Start by comparing each subsequent pair
		args[i].bar = bar;
		pthread_create(&tids[i], &attr, find_max_thread, (void*)&args[i]);
	}

	for(i = 0; i < count / 2; i ++)	{
		pthread_join(tids[i], NULL);
	}

	return array[0];
}

int main()	{
	int count = 0;
	Inode * input = (Inode*)malloc(sizeof(Inode));

	count = load_input(input);
	int * array = (int*)malloc(sizeof(int) * count);
	ll_to_array(array, input);
	//Free the Inodes now
	Inode * tmp = input;
	while(input)	{
		tmp = input->next;
		free(input);
		input = tmp;
	}

	int max = find_max(array, count);
	printf("%d\n", max);

	return 0;
}
