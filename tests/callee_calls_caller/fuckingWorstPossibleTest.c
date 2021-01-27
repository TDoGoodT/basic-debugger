
#include <stdio.h>
#include <stdbool.h>


void callerFunc(bool isCalling);

void calleeFunc(){
	printf("Has to printed to given output text\n");
	printf("Entering caller fucktion, has to print to output file\n");
	callerFunc(false);
	printf("We have exited the caller fucktion, after this caller will not write to given output file\n");
}

void callerFunc(bool isCalling){
	printf("Entered Calling Func\n");
	if(isCalling){
		calleeFunc();
	}
	printf("Has to be printed to given output only once!\n");
	return;
}

int main(){
	printf("We are starting the test (printed in stdId no matter what flag you entered)\n");
	callerFunc(true);
	printf("We have ended the test (printed in stdId no matter what flag you entered)\n");
	return 0;
}