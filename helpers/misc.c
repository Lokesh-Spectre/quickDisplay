#include<stdio.h>

int main(){
    int x=0b0110;
    printf("%d\n",x);
    x<<=1;
    printf("%d\n",x);
    x>>=1;
    printf("%d\n",x);
}