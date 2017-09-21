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

int find_max(int * array, int count)	{
	int max = -1;
	int i;
	for(i = 0; i < count; i ++)	{
		if (array[i] > max)
			max = array[i];
	}

	return max;
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
