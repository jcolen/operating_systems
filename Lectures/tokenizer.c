#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()	{

	char input[100];
	char seps[] = " ";
	char *token;

	fgets(input, sizeof input, stdin);
	printf("Length is : %zd\n", strlen(input));
	printf("Tokens:\n");
	//Establish string and get next token
	token = strtok(input, seps);

	while (token != NULL)	{
		printf("%s\n", token);
		token = strtok(NULL, seps);		
	}


	return 0;
}

