
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "defs.h"
#include "hash.h"

#define SAMPLES_TO_COLLECT   10000000
#define RAND_NUM_UPPER_BOUND   100000
#define NUM_SEED_STREAMS            4

/* 
 * ECE454 Students: 
 * Please fill in the following team struct 
 */
team_t team = {
    "Team Name",                  /* Team name */

    "Jackson Nie",                    /* Member full name */
    "1005282409",                 /* Member student number */
    "tianchen.nie@mail.utoronto.ca",                 /* Member email address */
};

// Implemented without modifying the hash class (stated in Q2 of handout)
unsigned num_threads;
unsigned samples_to_skip;
pthread_mutex_t l_mutexes[1 << 14];

class sample;

class sample {
  unsigned my_key;
 public:
  sample *next;
  unsigned count;

  sample(unsigned the_key){my_key = the_key; count = 0;};
  unsigned key(){return my_key;}
  void print(FILE *f){printf("%d %d\n",my_key,count);}
};

// This instantiates an empty hash table
// it is a C++ template, which means we define the types for
// the element and key value here: element is "class sample" and
// key value is "unsigned".  
hash<sample,unsigned> h;


void *collect(void *arg) {
    int i,j,k;
    int rnum;
    int thread_index = * (int *)arg;
    unsigned key;
    sample *s;
    int num_seeds = NUM_SEED_STREAMS / num_threads;
    free(arg);
    if (num_threads == 2) {
      thread_index <<= 1;
    }
    // fprintf(stdout, "RNUM: %d", rnum);
    for (i = 0; i < num_seeds; i++) {
      rnum = i + thread_index;
      // collect a number of samples
      for (j=0; j<SAMPLES_TO_COLLECT; j++){
        // skip a number of samples
        for (k=0; k<samples_to_skip; k++){
          rnum = rand_r((unsigned int*)&rnum);
        }

        // force the sample to be within the range of 0..RAND_NUM_UPPER_BOUND-1
        key = rnum % RAND_NUM_UPPER_BOUND;

        int index = HASH_INDEX(key, (1 << 14) - 1);
        pthread_mutex_lock(&l_mutexes[index]);
        // if this sample has not been counted before
        if (!(s = h.lookup(key))){
          // insert a new element for it into the hash table
          s = new sample(key);
          h.insert(s);
        }
        // increment the count for the sample
        s->count++;
        pthread_mutex_unlock(&l_mutexes[index]);
      }
    }
    return NULL;
}

int main (int argc, char* argv[]){
  int i,j,k;
  int rnum;
  unsigned key;
  sample *s;

  // Print out team information
  printf( "Team Name: %s\n", team.team );
  printf( "\n" );
  printf( "Student 1 Name: %s\n", team.name1 );
  printf( "Student 1 Student Number: %s\n", team.number1 );
  printf( "Student 1 Email: %s\n", team.email1 );
  printf( "\n" );

  // Parse program arguments
  if (argc != 3){
    printf("Usage: %s <num_threads> <samples_to_skip>\n", argv[0]);
    exit(1);  
  }
  sscanf(argv[1], " %d", &num_threads);
  sscanf(argv[2], " %d", &samples_to_skip);

  pthread_t threads[num_threads];
  // initialize a 16K-entry (2**14) hash of empty lists
  h.setup(14);

  for (i = 0; i < (1 << 14); i++) {
    pthread_mutex_init(&l_mutexes[i], NULL);
  }
  // process streams starting with different initial numbers
  for (i=0; i<num_threads; i++){
    int *index = (int *) malloc(sizeof(int));
    *index = i;
    pthread_t stream;
    threads[i] = stream;
    pthread_create(&threads[i], NULL, &collect, index);
  }

  for (i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
    
  }
  // print a list of the frequency of all samples
  h.print();
}
