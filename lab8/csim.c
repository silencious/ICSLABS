// Name: Tian Jiahe
// Student ID: 5130379056

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"

#define TRUE 1
#define FALSE 0
#define WORDBIT 64
#define MAXLEN 100


void usage(char* argv[]){
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
}

void getarg(int argc, char* argv[], int* verbose, int* s, int* E, int* b, FILE** fin){
    char* trace;
    char ch;
    while((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch(ch){
            case 'h':{
                usage(argv);
                exit(0);
            }
            case 'v':{
                *verbose = TRUE;
                break;
            }
            case 's':{
                *s = atoi(optarg);
                break;
            }
            case 'E':{
                *E = atoi(optarg);
                break;
            }
            case 'b':{
                *b = atoi(optarg);
                break;
            }
            case 't':{
                trace = (char *)malloc(strlen(optarg));
                strcpy(trace, optarg);
                break;
            }
            case '?':{
                usage(argv);
                exit(1);
            }
        }
    }
    if(*s == 0 || *E == 0 || *b == 0 || trace == NULL){
        printf("%s: Missing required command line argument\n", argv[0]);
        usage(argv);
        exit(1);
    }
    *fin = fopen(trace, "r");
    if(*fin == NULL){
        printf("%s: No such file or directory\n", trace);
        usage(argv);
        exit(1);
    }
    free(trace);
}

void* locate(void* cache, int s, int E, int b, unsigned long addr){
    int t = WORDBIT - s - b;
    int len = 10 + (1 << b); //1 byte for valid, 1 byte for LRU, 8 byte for tag
    long tag = addr >> (s + b);
    int index = (addr << t) >> (t + b);
    void* line = cache + len * E * index;
    for(int i=0; i<E; i++){
        if(((char*)line)[0] == 0){
            line += len;
            continue;
        }
        ((unsigned char*)line)[1] += 1;
        line += len;
    }
    line = cache + len * E * index;
    for(int i=0; i<E; i++){
        if(((char*)line)[0] == 0){
            line += len;
            continue;
        }
        long cachetag = ((long*)(line + 2))[0];
        if(tag == cachetag){
            ((unsigned char*)line)[1] = 1;
            return line;
        }
        line += len;
    }
    return NULL;
}

int setline(void* cache, int s, int E, int b, unsigned long addr){
    int t = WORDBIT - s - b;
    int len = 10 + (1 << b); //1 byte for valid, 1 byte for LRU, 8 byte for tag
    long tag = addr >> (s + b);
    int index = (addr << t) >> (t + b);
    void* line = cache + len * E * index;
    unsigned int max = 0, lru =0;    //least recently used
    for(int i=0; i<E; i++){
        if(((char*)line)[0] == 0){
            ((char*)line)[0] = 1;
            ((char*)line)[1] = 1;
            ((long*)(line + 2))[0] = tag;
            return 1;
        }
        if(((unsigned char*)line)[1] > max){
            max = ((unsigned char*)line)[1];
            lru = i;
        }
        line += len;
    }
    line = cache + len * (E * index + lru);
    ((char*)line)[0] = 1;
    ((char*)line)[1] = 1;
    ((long*)(line + 2))[0] = tag;
    return 0;
}

int main(int argc, char* argv[]){
    int verbose = 0, s = 0, E = 0, b = 0;
    FILE* fin = NULL;
    getarg(argc, argv, &verbose, &s, &E, &b, &fin);

    int S = 1 << s, B = 1 << b;
    int len = 10 + B; //1 byte for valid, 1 byte for LRU, 8 byte for tag
    void* cache = malloc(len * E * S);
    memset(cache, 0, len * E * S);
    int hits = 0, misses = 0, evictions = 0;
    char op[2];
    unsigned long addr;
    int size;
    while(fscanf(fin, "%s%lx,%d", op, &addr, &size) == 3){
        if(op[0]=='I'){
            continue;
        }
        void* line;
        switch(op[0]){
            case 'L':{
                printf("L %lx,%d ", addr, size);
                if((line = locate(cache, s, E, b, addr)) != NULL){
                    hits++;
                    if(verbose)
                        printf("hit\n");
                }
                else{
                    misses++;
                    if(setline(cache, s, E, b, addr) == 0){
                        evictions++;
                        if(verbose)
                            printf("miss eviction\n");
                    }
                    else{
                        if(verbose)
                            printf("miss\n");
                    }
                }
                break;
            }
            case 'S':{
                printf("S %lx,%d ", addr, size);
                if((line = locate(cache, s, E, b, addr)) != NULL){
                    hits++;
                    if(verbose){
                        printf("hit\n");
                    }
                }
                else{
                    misses++;
                    if(setline(cache, s, E, b, addr) == 0){
                        evictions++;
                        if(verbose)
                            printf("miss eviction\n");
                    }
                    else{
                        if(verbose)
                            printf("miss\n");
                    }
                }
                break;
            }
            case 'M':{
                printf("M %lx,%d ", addr, size);
                if((line = locate(cache, s, E, b, addr)) != NULL){
                    hits+=2;
                    if(verbose){
                        printf("hit hit\n");
                    }
                }
                else{
                    misses++;
                    hits++;
                    if(setline(cache, s, E, b, addr) == 0){
                        evictions++;
                        if(verbose)
                            printf("miss eviction hit\n");
                    }
                    else{
                        if(verbose)
                            printf("miss hit\n");
                    }               
                }
                break;
            }            
        }
    }

    printSummary(hits, misses, evictions);
    free(cache);
    fclose(fin);
    return 0;
}
