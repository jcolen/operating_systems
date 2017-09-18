#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()	{

	while(1)	{
		//read a line of input
		char input[100];
		printf("> ");
		fgets(input, sizeof input, stdin);
		printf("Input is : ");
		fputs(input, stdout);
	}

	return 0;
}

