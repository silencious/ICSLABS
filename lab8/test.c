#include <stdio.h>
#include <stdlib.h>

extern void transpose_submit(int M, int N, int A[N][M], int B[M][N]);

int main(){
	int A[64][64], B[64][64];
	int i, j;
	for(i=0; i<64; i++)
		for(j=0; j<64; j++)
			A[i][j] = i*100 + j;
		
	transpose_submit(64,64,A,B);
	for(i=0; i<64; i++){
		for(j=0; j<64; j++)
			printf("%d ",B[i][j]);
		printf("\n");
	}
	return 0;
}
