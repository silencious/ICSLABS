// Name: Tian Jiahe
// Student ID: 5130379056

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void t32(int M, int N, int A[N][M], int B[M][N], int i, int j){
	int r, c, t;
    for(r = 0; r < 8; r++){
        t = A[i+r][j+r];
        for(c = 0; c < 8; c++){
            if(r == c)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
        }
        B[j+r][i+r] = t;
    }
} 

void swap(int* a, int* b){
	*a = *a + *b;
	*b = *a - *b;
	*a = *a - *b;
}

// this is used to help transpose64 to complete its work
void adjust(int M, int N, int B[M][N], int i, int j, int r, int c){
	for(c=0; c<4; c++){
		swap(&B[i][j+c], &B[i+2][j+c]);
		swap(&B[i+1][j+c], &B[i+3][j+c]);
	}
	for(r=0; r<4; r++)
		for(c=r+1; c<4; c++)
			swap(&B[i+r][j+c], &B[i+c][j+r]);
}

// this function is designed especially for transpose 8x8 blocks
// in 64x64 blocks, which copy the data from A to B first and then 
// use adjust() to put data to where they belong to 
void t64(int M, int N, int A[N][M], int B[M][N], int i, int j){
	int r, c;
	int t0, t1, t2, t3, t4, t5, t6, t7;	//totally 12 variables
	t0 = A[i+0][j+4];
	t1 = A[i+0][j+5];
	t2 = A[i+0][j+6];
	t3 = A[i+0][j+7];
	t4 = A[i+1][j+4];
	t5 = A[i+1][j+5];
	t6 = A[i+1][j+6];
	t7 = A[i+1][j+7];
	for(r=0; r<2; r++)
		for(c=0; c<4; c++)
			B[j+r+2][i+c] = A[i+r][j+c];
	for(r=4; r<6; r++)
		for(c=0; c<4; c++)
			B[j+r-2][i+c+4] = A[i+r][j+c];
	for(r=4; r<6; r++)
		for(c=4; c<8; c++)
			B[j+r+2][i+c] = A[i+r][j+c];
	B[j+6][i+0] = t0;
	B[j+6][i+1] = t1;
	B[j+6][i+2] = t2;
	B[j+6][i+3] = t3;
	B[j+7][i+0] = t4;
	B[j+7][i+1] = t5;
	B[j+7][i+2] = t6;
	B[j+7][i+3] = t7;
	t0 = A[i+2][j+4];
	t1 = A[i+2][j+5];
	t2 = A[i+2][j+6];
	t3 = A[i+2][j+7];
	t4 = A[i+3][j+4];
	t5 = A[i+3][j+5];
	t6 = A[i+3][j+6];
	t7 = A[i+3][j+7];
	for(r=2; r<4; r++)
		for(c=0; c<4; c++)
			B[j+r-2][i+c] = A[i+r][j+c];
	for(r=6; r<8; r++)
		for(c=0; c<4; c++)
			B[j+r-6][i+c+4] = A[i+r][j+c];
	for(r=6; r<8; r++)
		for(c=4; c<8; c++)
			B[j+r-2][i+c] = A[i+r][j+c];
	B[j+4][i+0] = t0;
	B[j+4][i+1] = t1;
	B[j+4][i+2] = t2;
	B[j+4][i+3] = t3;
	B[j+5][i+0] = t4;
	B[j+5][i+1] = t5;
	B[j+5][i+2] = t6;
	B[j+5][i+3] = t7;

	adjust(M, N, B, j+4, i+4, r, c);
	adjust(M, N, B, j+4, i, r, c);
	adjust(M, N, B, j, i, r, c);
	adjust(M, N, B, j, i+4, r, c);
}
// Another function to transpose 64x64 matrix, failed to reach 1300
/*
void t64_2(int M, int N, int A[N][M], int B[M][N], int i, int j){
    int r, c;
    int t0, t1, t2, t3, t4, t5;//, t6, t7;
	t1 = A[i][j+4];
	t2 = A[i][j+5];
	t3 = A[i][j+6];
	t4 = A[i][j+7];
    for(r = 0; r < 4; r++){
        t0 = A[i+r][j+r];
        for(c = 0; c < 4; c++){
            if(r == c)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
        }
		if(r == 3)
			t5 = t0;
        else
			B[j+r][i+r] = t0;
    }
	for(r = 7; r >= 4; r--){
        t0 = A[i+r][j+r-4];
        for(c = 0; c < 4; c++){
            if(r == c+4)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
        }
		if(r == 7)
			B[j+3][i+3] = t5;
        B[j+r-4][i+r] = t0;
    }
    for(r = 4; r < 8; r++){
        t0 = A[i+r][j+r];
        for(c = 4; c < 8; c++){
            if(r == c)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
        }
        B[j+r][i+r] = t0;
    }
    for(r = 1; r < 4; r++){
        t0 = A[i+r][j+r+4];
        for(c = 4; c < 8; c++){
            if(r+4 == c)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
			if(r == 1 && c == 4)
				B[j+4][i] = t1;
			if(r == 2 && c == 5)
				B[j+5][i] = t2;
			if(r == 1 && c == 6)
				B[j+6][i] = t3;
			if(r == 1 && c == 7)
				B[j+7][i] = t4;
        }
        B[j+r+4][i+r] = t0;
    }
}
*/

void todd(int M, int N, int A[N][M], int B[M][N], int i, int j){
	int r, c;
	int t;
    for(r = 0; r < 8; r++){
        t = A[i+r][j+r];
        for(c = 0; c < 8; c++){
            if(r == c)
                continue;
            B[j+c][i+r] = A[i+r][j+c];
        }
        B[j+r][i+r] = t;
    }
} 

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    if((M == 32) && (N == 32)){
        for(i = 0; i < N; i += 8)
            for(j = 0; j < M; j += 8)
                t32(M, N, A, B, i, j);
        return;
    }
    if((M == 64) && (N == 64)){
        for(i = 0; i < N; i += 8)
            for(j = 0; j < M; j += 8)
                t64(M, N, A, B, i, j);
        return;
    }
	if((M == 61) && (N == 67)){
		for(i = 0; i < N; i++)
			for(j = 56; j < M; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 0; j < 8; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 8; j < 16; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 16; j < 24; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 24; j < 32; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 32; j < 40; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 40; j < 48; j++)
				B[j][i] = A[i][j];
        for(i = 64; i < N; i++)
			for(j = 48; j < 56; j++)
				B[j][i] = A[i][j];
        for(j = 48; j >= 0; j -= 8){
        	for(i = 0; i < 64; i += 8){
                todd(M, N, A, B, i, j);
			}
		}
		return;
	}
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            B[j][i] = A[i][j];
        }
    }    
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    //registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

