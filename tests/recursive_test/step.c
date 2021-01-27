#include <stdio.h>

void step(int n){
	printf("We are in the %d step\n",n);
	if (n==1) return;
	step(n-1);
}

int main(){
	printf("Hello! I'm starting the steps\n");
	step(10);
	printf("Im after the fucktion\n");
	return 0;
}
