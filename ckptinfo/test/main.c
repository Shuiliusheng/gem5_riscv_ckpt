#include<stdio.h>
#include<stdlib.h>
int main()
{
	double arr[100];
	for(int i=0;i<1000;i++){
		arr[i%100] = i * 1.0 / 0.3;
		printf("%d: %lf\n", i, arr[i%100]);
	}
	return 0;
}
