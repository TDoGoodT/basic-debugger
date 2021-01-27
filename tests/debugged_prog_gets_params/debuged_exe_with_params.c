#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void printingFuck(int printMe){
	printf("this number %d is needed to be written in output\n", printMe);
}

int main(int argc, char** argv){
	
	printf("Before calling\n");
	printingFuck(atoi(argv[1]));
	printf("After Returning\n");
	
	return 0;
}