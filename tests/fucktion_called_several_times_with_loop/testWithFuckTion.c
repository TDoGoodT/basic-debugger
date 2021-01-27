#include <stdio.h>

void printingFucktion(){
    printf("CHACHACHA\n");
}

int main(){
    printf("print me before the prints of the fucktion!\n");
    for(int i=0; i< 3; i++){
       printingFucktion();
    }
    printf("print me in std and only there\n");
    return 0;
}
