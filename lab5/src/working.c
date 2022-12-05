/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 * TODO: race condition present, needs debugging
 ****************************************************************************/
#define _GNU_SOURCE
#include <sched.h>
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
// alive: 2-3 neighbors stay alive, else die
// dead: 3 neighbors and live, else dead
/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])
#define NUM_THREADS 16

static char *in_board;
// contains indices of cells that have been change in prev iteration.
static unsigned int *change_list0;
static unsigned int *change_list1;
static unsigned int *change_list2;
static unsigned int *change_list3;
static unsigned int *change_list4;
static unsigned int *change_list5;
static unsigned int *change_list6;
static unsigned int *change_list7;
static unsigned int *change_list8;
static unsigned int *change_list9;
static unsigned int *change_list10;
static unsigned int *change_list11;
static unsigned int *change_list12;
static unsigned int *change_list13;
static unsigned int *change_list14;
static unsigned int *change_list15;

// x_y: thread x owns a portion of the change list of thread y
static unsigned int *change_list0_15;
static unsigned int *change_list0_1;

static unsigned int *change_list1_0;
static unsigned int *change_list1_2;

static unsigned int *change_list2_1;
static unsigned int *change_list2_3;

static unsigned int *change_list3_2;
static unsigned int *change_list3_4;

static unsigned int *change_list4_3;
static unsigned int *change_list4_5;

static unsigned int *change_list5_4;
static unsigned int *change_list5_6;

static unsigned int *change_list6_5;
static unsigned int *change_list6_7;

static unsigned int *change_list7_6;
static unsigned int *change_list7_8;

static unsigned int *change_list8_7;
static unsigned int *change_list8_9;

static unsigned int *change_list9_8;
static unsigned int *change_list9_10;

static unsigned int *change_list10_9;
static unsigned int *change_list10_11;

static unsigned int *change_list11_10;
static unsigned int *change_list11_12;

static unsigned int *change_list12_11;
static unsigned int *change_list12_13;

static unsigned int *change_list13_12;
static unsigned int *change_list13_14;

static unsigned int *change_list14_13;
static unsigned int *change_list14_15;

static unsigned int *change_list15_14;
static unsigned int *change_list15_0;

static unsigned int *cl_index0;
static unsigned int *cl_index1;
static unsigned int *cl_index2;
static unsigned int *cl_index3;
static unsigned int *cl_index4;
static unsigned int *cl_index5;
static unsigned int *cl_index6;
static unsigned int *cl_index7;
static unsigned int *cl_index8;
static unsigned int *cl_index9;
static unsigned int *cl_index10;
static unsigned int *cl_index11;
static unsigned int *cl_index12;
static unsigned int *cl_index13;
static unsigned int *cl_index14;
static unsigned int *cl_index15;

static unsigned int *cl_index0_15;
static unsigned int *cl_index0_1;

static unsigned int *cl_index1_0;
static unsigned int *cl_index1_2;

static unsigned int *cl_index2_1;
static unsigned int *cl_index2_3;

static unsigned int *cl_index3_2;
static unsigned int *cl_index3_4;

static unsigned int *cl_index4_3;
static unsigned int *cl_index4_5;

static unsigned int *cl_index5_4;
static unsigned int *cl_index5_6;

static unsigned int *cl_index6_5;
static unsigned int *cl_index6_7;

static unsigned int *cl_index7_6;
static unsigned int *cl_index7_8;

static unsigned int *cl_index8_7;
static unsigned int *cl_index8_9;

static unsigned int *cl_index9_8;
static unsigned int *cl_index9_10;

static unsigned int *cl_index10_9;
static unsigned int *cl_index10_11;

static unsigned int *cl_index11_10;
static unsigned int *cl_index11_12;

static unsigned int *cl_index12_11;
static unsigned int *cl_index12_13;

static unsigned int *cl_index13_12;
static unsigned int *cl_index13_14;

static unsigned int *cl_index14_13;
static unsigned int *cl_index14_15;

static unsigned int *cl_index15_14;
static unsigned int *cl_index15_0;


// static pthread_mutex_t cl_lock0;
// static pthread_mutex_t cl_lock1;
// static pthread_mutex_t cl_lock2;
// static pthread_mutex_t cl_lock3;
// static pthread_mutex_t cl_lock4;
// static pthread_mutex_t cl_lock5;
// static pthread_mutex_t cl_lock6;
// static pthread_mutex_t cl_lock7;
// static pthread_mutex_t cl_lock8;
// static pthread_mutex_t cl_lock9;
// static pthread_mutex_t cl_lock10;
// static pthread_mutex_t cl_lock11;
// static pthread_mutex_t cl_lock12;
// static pthread_mutex_t cl_lock13;
// static pthread_mutex_t cl_lock14;
// static pthread_mutex_t cl_lock15;

static pthread_mutex_t *per_thread_element_locks;
static unsigned int num_rows;
static unsigned int num_cols;
static unsigned int num_gens;
static unsigned int gen;
static unsigned int threads_done;
// static pthread_mutex_t update_neighbour_count_mutex;
// static pthread_cond_t update_neighbour_count_cond;
static pthread_mutex_t thread_done_mutex;
static pthread_cond_t thread_done_cond;
static pthread_cond_t thread_go_cond;
static pthread_mutex_t thread_go_mutex;

static inline void initialize_change_lists(unsigned int nrows, unsigned int ncols) {
	int thread_area = nrows / NUM_THREADS * ncols;
	change_list0 = calloc(thread_area, sizeof(unsigned int));
	change_list1 = calloc(thread_area, sizeof(unsigned int));
	change_list2 = calloc(thread_area, sizeof(unsigned int));
	change_list3 = calloc(thread_area, sizeof(unsigned int));
	change_list4 = calloc(thread_area, sizeof(unsigned int));
	change_list5 = calloc(thread_area, sizeof(unsigned int));
	change_list6 = calloc(thread_area, sizeof(unsigned int));
	change_list7 = calloc(thread_area, sizeof(unsigned int));
	change_list8 = calloc(thread_area, sizeof(unsigned int));
	change_list9 = calloc(thread_area, sizeof(unsigned int));
	change_list10 = calloc(thread_area, sizeof(unsigned int));
	change_list11 = calloc(thread_area, sizeof(unsigned int));
	change_list12 = calloc(thread_area, sizeof(unsigned int));
	change_list13 = calloc(thread_area, sizeof(unsigned int));
	change_list14 = calloc(thread_area, sizeof(unsigned int));
	change_list15 = calloc(thread_area, sizeof(unsigned int));

	cl_index0 = calloc(1, sizeof(unsigned int));
	cl_index1 = calloc(1, sizeof(unsigned int));
	cl_index2 = calloc(1, sizeof(unsigned int));
	cl_index3 = calloc(1, sizeof(unsigned int));
	cl_index4 = calloc(1, sizeof(unsigned int));
	cl_index5 = calloc(1, sizeof(unsigned int));
	cl_index6 = calloc(1, sizeof(unsigned int));
	cl_index7 = calloc(1, sizeof(unsigned int));
	cl_index8 = calloc(1, sizeof(unsigned int));
	cl_index9 = calloc(1, sizeof(unsigned int));
	cl_index10 = calloc(1, sizeof(unsigned int));
	cl_index11 = calloc(1, sizeof(unsigned int));
	cl_index12 = calloc(1, sizeof(unsigned int));
	cl_index13 = calloc(1, sizeof(unsigned int));
	cl_index14 = calloc(1, sizeof(unsigned int));
	cl_index15 = calloc(1, sizeof(unsigned int));

	change_list0_15 = calloc(ncols, sizeof(unsigned int));
	change_list0_1 = calloc(ncols, sizeof(unsigned int));

	change_list1_0 = calloc(ncols, sizeof(unsigned int));
	change_list1_2 = calloc(ncols, sizeof(unsigned int));

	change_list2_1 = calloc(ncols, sizeof(unsigned int));
	change_list2_3 = calloc(ncols, sizeof(unsigned int));

	change_list3_2 = calloc(ncols, sizeof(unsigned int));
	change_list3_4 = calloc(ncols, sizeof(unsigned int));

	change_list4_3 = calloc(ncols, sizeof(unsigned int));
	change_list4_5 = calloc(ncols, sizeof(unsigned int));

	change_list5_4 = calloc(ncols, sizeof(unsigned int));
	change_list5_6 = calloc(ncols, sizeof(unsigned int));

	change_list6_5 = calloc(ncols, sizeof(unsigned int));
	change_list6_7 = calloc(ncols, sizeof(unsigned int));

	change_list7_6 = calloc(ncols, sizeof(unsigned int));
	change_list7_8 = calloc(ncols, sizeof(unsigned int));

	change_list8_7 = calloc(ncols, sizeof(unsigned int));
	change_list8_9 = calloc(ncols, sizeof(unsigned int));

	change_list9_8 = calloc(ncols, sizeof(unsigned int));
	change_list9_10 = calloc(ncols, sizeof(unsigned int));

	change_list10_9 = calloc(ncols, sizeof(unsigned int));
	change_list10_11 = calloc(ncols, sizeof(unsigned int));

	change_list11_10 = calloc(ncols, sizeof(unsigned int));
	change_list11_12 = calloc(ncols, sizeof(unsigned int));

	change_list12_11 = calloc(ncols, sizeof(unsigned int));
	change_list12_13 = calloc(ncols, sizeof(unsigned int));

	change_list13_12 = calloc(ncols, sizeof(unsigned int));
	change_list13_14 = calloc(ncols, sizeof(unsigned int));

	change_list14_13 = calloc(ncols, sizeof(unsigned int));
	change_list14_15 = calloc(ncols, sizeof(unsigned int));

	change_list15_14 = calloc(ncols, sizeof(unsigned int));
	change_list15_0 = calloc(ncols, sizeof(unsigned int));


	cl_index0_15 = calloc(1, sizeof(unsigned int));
	cl_index0_1 = calloc(1, sizeof(unsigned int));

	cl_index1_0 = calloc(1, sizeof(unsigned int));
	cl_index1_2 = calloc(1, sizeof(unsigned int));

	cl_index2_1 = calloc(1, sizeof(unsigned int));
	cl_index2_3 = calloc(1, sizeof(unsigned int));

	cl_index3_2 = calloc(1, sizeof(unsigned int));
	cl_index3_4 = calloc(1, sizeof(unsigned int));

	cl_index4_3 = calloc(1, sizeof(unsigned int));
	cl_index4_5 = calloc(1, sizeof(unsigned int));

	cl_index5_4 = calloc(1, sizeof(unsigned int));
	cl_index5_6 = calloc(1, sizeof(unsigned int));

	cl_index6_5 = calloc(1, sizeof(unsigned int));
	cl_index6_7 = calloc(1, sizeof(unsigned int));

	cl_index7_6 = calloc(1, sizeof(unsigned int));
	cl_index7_8 = calloc(1, sizeof(unsigned int));

	cl_index8_7 = calloc(1, sizeof(unsigned int));
	cl_index8_9 = calloc(1, sizeof(unsigned int));

	cl_index9_8 = calloc(1, sizeof(unsigned int));
	cl_index9_10 = calloc(1, sizeof(unsigned int));

	cl_index10_9 = calloc(1, sizeof(unsigned int));
	cl_index10_11 = calloc(1, sizeof(unsigned int));

	cl_index11_10 = calloc(1, sizeof(unsigned int));
	cl_index11_12 = calloc(1, sizeof(unsigned int));

	cl_index12_11 = calloc(1, sizeof(unsigned int));
	cl_index12_13 = calloc(1, sizeof(unsigned int));

	cl_index13_12 = calloc(1, sizeof(unsigned int));
	cl_index13_14 = calloc(1, sizeof(unsigned int));

	cl_index14_13 = calloc(1, sizeof(unsigned int));
	cl_index14_15 = calloc(1, sizeof(unsigned int));

	cl_index15_14 = calloc(1, sizeof(unsigned int));
	cl_index15_0 = calloc(1, sizeof(unsigned int));
	// pthread_mutex_init(&cl_lock0, NULL);
	// pthread_mutex_init(&cl_lock1, NULL);
	// pthread_mutex_init(&cl_lock2, NULL);
	// pthread_mutex_init(&cl_lock3, NULL);
	// pthread_mutex_init(&cl_lock4, NULL);
	// pthread_mutex_init(&cl_lock5, NULL);
	// pthread_mutex_init(&cl_lock6, NULL);
	// pthread_mutex_init(&cl_lock7, NULL);
	// pthread_mutex_init(&cl_lock8, NULL);
	// pthread_mutex_init(&cl_lock9, NULL);
	// pthread_mutex_init(&cl_lock10, NULL);
	// pthread_mutex_init(&cl_lock11, NULL);
	// pthread_mutex_init(&cl_lock12, NULL);
	// pthread_mutex_init(&cl_lock13, NULL);
	// pthread_mutex_init(&cl_lock14, NULL);
	// pthread_mutex_init(&cl_lock15, NULL);
}

static inline unsigned int *get_change_list(int id) {
	if (id == 0) return change_list0;
	else if (id == 1) return change_list1;
	else if (id == 2) return change_list2;
	else if (id == 3) return change_list3;
	else if (id == 4) return change_list4;
	else if (id == 5) return change_list5;
	else if (id == 6) return change_list6;
	else if (id == 7) return change_list7;
	else if (id == 8) return change_list8;
	else if (id == 9) return change_list9;
	else if (id == 10) return change_list10;
	else if (id == 11) return change_list11;
	else if (id == 12) return change_list12;
	else if (id == 13) return change_list13;
	else if (id == 14) return change_list14;
	else if (id == 15) return change_list15;
	return NULL;
}

static inline unsigned int *get_prev_thread_change_list_to_write(int id) {
	if (id == 0) return change_list0_15;
	else if (id == 1) return change_list1_0;
	else if (id == 2) return change_list2_1;
	else if (id == 3) return change_list3_2;
	else if (id == 4) return change_list4_3;
	else if (id == 5) return change_list5_4;
	else if (id == 6) return change_list6_5;
	else if (id == 7) return change_list7_6;
	else if (id == 8) return change_list8_7;
	else if (id == 9) return change_list9_8;
	else if (id == 10) return change_list10_9;
	else if (id == 11) return change_list11_10;
	else if (id == 12) return change_list12_11;
	else if (id == 13) return change_list13_12;
	else if (id == 14) return change_list14_13;
	else if (id == 15) return change_list15_14;
	return NULL;
}

static inline unsigned int *get_next_thread_change_list_to_write(int id) {
	if (id == 0) return change_list0_1;
	else if (id == 1) return change_list1_2;
	else if (id == 2) return change_list2_3;
	else if (id == 3) return change_list3_4;
	else if (id == 4) return change_list4_5;
	else if (id == 5) return change_list5_6;
	else if (id == 6) return change_list6_7;
	else if (id == 7) return change_list7_8;
	else if (id == 8) return change_list8_9;
	else if (id == 9) return change_list9_10;
	else if (id == 10) return change_list10_11;
	else if (id == 11) return change_list11_12;
	else if (id == 12) return change_list12_13;
	else if (id == 13) return change_list13_14;
	else if (id == 14) return change_list14_15;
	else if (id == 15) return change_list15_0;
	return NULL;
}

static inline unsigned int *get_prev_thread_my_change_list_to_read(int id) {
	if (id == 0) return change_list15_0;
	else if (id == 1) return change_list0_1;
	else if (id == 2) return change_list1_2;
	else if (id == 3) return change_list2_3;
	else if (id == 4) return change_list3_4;
	else if (id == 5) return change_list4_5;
	else if (id == 6) return change_list5_6;
	else if (id == 7) return change_list6_7;
	else if (id == 8) return change_list7_8;
	else if (id == 9) return change_list8_9;
	else if (id == 10) return change_list9_10;
	else if (id == 11) return change_list10_11;
	else if (id == 12) return change_list11_12;
	else if (id == 13) return change_list12_13;
	else if (id == 14) return change_list13_14;
	else if (id == 15) return change_list14_15;
	return NULL;
}

static inline unsigned int *get_next_thread_my_change_list_to_read(int id) {
	if (id == 0) return change_list1_0;
	else if (id == 1) return change_list2_1;
	else if (id == 2) return change_list3_2;
	else if (id == 3) return change_list4_3;
	else if (id == 4) return change_list5_4;
	else if (id == 5) return change_list6_5;
	else if (id == 6) return change_list7_6;
	else if (id == 7) return change_list8_7;
	else if (id == 8) return change_list9_8;
	else if (id == 9) return change_list10_9;
	else if (id == 10) return change_list11_10;
	else if (id == 11) return change_list12_11;
	else if (id == 12) return change_list13_12;
	else if (id == 13) return change_list14_13;
	else if (id == 14) return change_list15_14;
	else if (id == 15) return change_list0_15;
	return NULL;
}


static inline unsigned int *get_cl_index(int id) {
	if (id == 0) return cl_index0;
	else if (id == 1) return cl_index1;
	else if (id == 2) return cl_index2;
	else if (id == 3) return cl_index3;
	else if (id == 4) return cl_index4;
	else if (id == 5) return cl_index5;
	else if (id == 6) return cl_index6;
	else if (id == 7) return cl_index7;
	else if (id == 8) return cl_index8;
	else if (id == 9) return cl_index9;
	else if (id == 10) return cl_index10;
	else if (id == 11) return cl_index11;
	else if (id == 12) return cl_index12;
	else if (id == 13) return cl_index13;
	else if (id == 14) return cl_index14;
	else if (id == 15) return cl_index15;
	return NULL;
}

static inline unsigned int *get_prev_thread_cl_index_to_write(int id) {
	if (id == 0) return cl_index0_15;
	else if (id == 1) return cl_index1_0;
	else if (id == 2) return cl_index2_1;
	else if (id == 3) return cl_index3_2;
	else if (id == 4) return cl_index4_3;
	else if (id == 5) return cl_index5_4;
	else if (id == 6) return cl_index6_5;
	else if (id == 7) return cl_index7_6;
	else if (id == 8) return cl_index8_7;
	else if (id == 9) return cl_index9_8;
	else if (id == 10) return cl_index10_9;
	else if (id == 11) return cl_index11_10;
	else if (id == 12) return cl_index12_11;
	else if (id == 13) return cl_index13_12;
	else if (id == 14) return cl_index14_13;
	else if (id == 15) return cl_index15_14;
	return NULL;
}

static inline unsigned int *get_next_thread_cl_index_to_write(int id) {
	if (id == 0) return cl_index0_1;
	else if (id == 1) return cl_index1_2;
	else if (id == 2) return cl_index2_3;
	else if (id == 3) return cl_index3_4;
	else if (id == 4) return cl_index4_5;
	else if (id == 5) return cl_index5_6;
	else if (id == 6) return cl_index6_7;
	else if (id == 7) return cl_index7_8;
	else if (id == 8) return cl_index8_9;
	else if (id == 9) return cl_index9_10;
	else if (id == 10) return cl_index10_11;
	else if (id == 11) return cl_index11_12;
	else if (id == 12) return cl_index12_13;
	else if (id == 13) return cl_index13_14;
	else if (id == 14) return cl_index14_15;
	else if (id == 15) return cl_index15_0;
	return NULL;
}

static inline unsigned int *get_prev_thread_my_cl_index_to_read(int id) {
	if (id == 0) return cl_index15_0;
	else if (id == 1) return cl_index0_1;
	else if (id == 2) return cl_index1_2;
	else if (id == 3) return cl_index2_3;
	else if (id == 4) return cl_index3_4;
	else if (id == 5) return cl_index4_5;
	else if (id == 6) return cl_index5_6;
	else if (id == 7) return cl_index6_7;
	else if (id == 8) return cl_index7_8;
	else if (id == 9) return cl_index8_9;
	else if (id == 10) return cl_index9_10;
	else if (id == 11) return cl_index10_11;
	else if (id == 12) return cl_index11_12;
	else if (id == 13) return cl_index12_13;
	else if (id == 14) return cl_index13_14;
	else if (id == 15) return cl_index14_15;
	return NULL;
}

static inline unsigned int *get_next_thread_my_cl_index_to_read(int id) {
	if (id == 0) return cl_index1_0;
	else if (id == 1) return cl_index2_1;
	else if (id == 2) return cl_index3_2;
	else if (id == 3) return cl_index4_3;
	else if (id == 4) return cl_index5_4;
	else if (id == 5) return cl_index6_5;
	else if (id == 6) return cl_index7_6;
	else if (id == 7) return cl_index8_7;
	else if (id == 8) return cl_index9_8;
	else if (id == 9) return cl_index10_9;
	else if (id == 10) return cl_index11_10;
	else if (id == 11) return cl_index12_11;
	else if (id == 12) return cl_index13_12;
	else if (id == 13) return cl_index14_13;
	else if (id == 14) return cl_index15_14;
	else if (id == 15) return cl_index0_15;
	return NULL;
}

// static inline pthread_mutex_t *get_cl_lock(int id) {
// 	if (id == 0) return &cl_lock0;
// 	else if (id == 1) return &cl_lock1;
// 	else if (id == 2) return &cl_lock2;
// 	else if (id == 3) return &cl_lock3;
// 	else if (id == 4) return &cl_lock4;
// 	else if (id == 5) return &cl_lock5;
// 	else if (id == 6) return &cl_lock6;
// 	else if (id == 7) return &cl_lock7;
// 	else if (id == 8) return &cl_lock8;
// 	else if (id == 9) return &cl_lock9;
// 	else if (id == 10) return &cl_lock10;
// 	else if (id == 11) return &cl_lock11;
// 	else if (id == 12) return &cl_lock12;
// 	else if (id == 13) return &cl_lock13;
// 	else if (id == 14) return &cl_lock14;
// 	else if (id == 15) return &cl_lock15;
// 	return NULL;
// }

void *game_of_life_thread(void *id) {
	int myid = *(int *)id;
	free(id);
	char *inboard = in_board;
	cpu_set_t my_core;
	CPU_ZERO(&my_core);
	CPU_SET(myid, &my_core);
	pthread_setaffinity_np(pthread_self(), sizeof(my_core), &my_core);
	unsigned int *prev_change_list_to_write = get_prev_thread_change_list_to_write(myid);
	unsigned int *next_change_list_to_write = get_next_thread_change_list_to_write(myid);
	unsigned int *my_change_list = get_change_list(myid);
	unsigned int *my_change_list_to_read_from_prev_thread = get_prev_thread_my_change_list_to_read(myid);
	unsigned int *my_change_list_to_read_from_next_thread = get_next_thread_my_change_list_to_read(myid);

	unsigned int *prev_cl_index_to_write = get_prev_thread_cl_index_to_write(myid);
	unsigned int *next_cl_index_to_write = get_next_thread_cl_index_to_write(myid);
	unsigned int *my_cl_index = get_cl_index(myid);
	unsigned int *prev_cl_index_to_read = get_prev_thread_my_cl_index_to_read(myid);
	unsigned int *next_cl_index_to_read = get_next_thread_my_cl_index_to_read(myid);
	
	int num_rows_to_handle = num_rows / NUM_THREADS;
	// fprintf(stdout, "NUM ROWS TO HANDLE: %d\n", num_rows_to_handle);
	// fflush(stdout);
	int start_row = myid * num_rows_to_handle;
	int end_row = start_row + num_rows_to_handle;
	unsigned int my_area = num_rows_to_handle * num_cols;

	// the index in the per thread element lock array where the lock to my first element exists.
	int my_lock_start_index = myid * num_cols * 2;
	int last_thread_lock_start_index = (NUM_THREADS - 1) * num_cols * 2;

	unsigned int prev_row_start;
	unsigned int row_start;
	unsigned int next_row_start;
	char cell, wcell, ecell, nwcell, ncell, necell, swcell, scell, secell;
	int westcol, eastcol, northrow, southrow;
	int curgen, i, j, row, col;
	int gens_max = num_gens;
	unsigned int *swap_change_list = calloc(my_area, sizeof(unsigned int));
	unsigned int swap_cl_index = 0;
	// initialize
	for (i = start_row; i < end_row; i++) {
		row_start = i * num_cols;
		for (j = 0; j < num_cols; j++) {
			if (!IS_ALIVE(inboard[row_start + j])) {
				continue;
			}
			swap_change_list[swap_cl_index++] = SET_ROW_COL(i, j);
		}
	}

	pthread_mutex_lock(&thread_done_mutex);
	threads_done++;

	if (likely(threads_done != NUM_THREADS)) {
		pthread_cond_wait(&thread_done_cond, &thread_done_mutex);
		pthread_mutex_unlock(&thread_done_mutex);
	}
	else if (unlikely(threads_done == NUM_THREADS)) {
		threads_done = 0;
		pthread_cond_broadcast(&thread_done_cond);
		pthread_mutex_unlock(&thread_done_mutex);
	}

	for (curgen = 0; curgen < gens_max; curgen++) {
		// fprintf(stdout, "On gen %d... with swap_cl_index %d.... with prev thread write index: %d... with next thread write index: %d...\n", curgen, swap_cl_index, *prev_cl_index_to_write, *next_cl_index_to_write);
		// fflush(stdout);
		// fprintf(stdout, "outboard: 0x%x\n", outboard);
		// fflush(stdout);
		for (i = 0; i < swap_cl_index; i++) {
			row = GET_ROW(swap_change_list[i]);
			col = GET_COL(swap_change_list[i]);
			// fprintf(stdout, "row %d... col %d....\n", row, col);
			// fflush(stdout);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;

			/* if the cell lies within the boundaries of this thread's region*/
			if (likely(row > start_row && row < (end_row - 1))) {
				wcell = inboard[row_start + westcol];
				cell = inboard[row_start + col];
				ecell = inboard[row_start + eastcol];
				/* check horizontally */
				if (SHOULD_CHANGE_STATE(wcell)) {
					inboard[row_start + westcol] = CHANGE_STATE(wcell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, westcol);
				}
				if (SHOULD_CHANGE_STATE(cell)) {
					inboard[row_start + col] = CHANGE_STATE(cell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, col);
				}
				if (SHOULD_CHANGE_STATE(ecell)) {
					inboard[row_start + eastcol] = CHANGE_STATE(ecell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, eastcol);
				}

				/* check north neighbours */
				if (likely(northrow > start_row)) {
					nwcell = inboard[prev_row_start + westcol];
					ncell = inboard[prev_row_start + col];
					necell = inboard[prev_row_start + eastcol];
					if (SHOULD_CHANGE_STATE(nwcell)) {
						inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, westcol);
					}
					if (SHOULD_CHANGE_STATE(ncell)) {
						inboard[prev_row_start + col] = CHANGE_STATE(ncell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, col);
					}
					if (SHOULD_CHANGE_STATE(necell)) {
						inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, eastcol);
					}
				}
				else if (unlikely(northrow == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
					nwcell = inboard[prev_row_start + westcol];
					if (SHOULD_CHANGE_STATE(nwcell)) {
						inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, westcol);
					}
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + col]);
					ncell = inboard[prev_row_start + col];
					if (SHOULD_CHANGE_STATE(ncell)) {
						inboard[prev_row_start + col] = CHANGE_STATE(ncell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, col);
					}
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + col]);

					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
					necell = inboard[prev_row_start + eastcol];
					if (SHOULD_CHANGE_STATE(necell)) {
						inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, eastcol);
					}
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);
				}

				/* check south neighbours */
				if (likely(southrow < (end_row - 1))) {
					swcell = inboard[next_row_start + westcol];
					scell  = inboard[next_row_start + col];
					secell = inboard[next_row_start + eastcol];
					if (SHOULD_CHANGE_STATE(swcell)) {
						inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, westcol);
					}
					if (SHOULD_CHANGE_STATE(scell)) {
						inboard[next_row_start + col] = CHANGE_STATE(scell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, col);
					}
					if (SHOULD_CHANGE_STATE(secell)) {
						inboard[next_row_start + eastcol] = CHANGE_STATE(secell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, eastcol);
					}
				}
				else if (unlikely(southrow == (end_row - 1))) {
					const int lock_index_south_west = my_lock_start_index + num_cols + westcol;
					const int lock_index_south = my_lock_start_index + num_cols + col;
					const int lock_index_south_east = my_lock_start_index + num_cols + eastcol;

					pthread_mutex_lock(&per_thread_element_locks[lock_index_south_west]);
					swcell = inboard[next_row_start + westcol];
					if (SHOULD_CHANGE_STATE(swcell)) {
						inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, westcol);
					}
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_west]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_south]);
					scell  = inboard[next_row_start + col];
					if (SHOULD_CHANGE_STATE(scell)) {
						inboard[next_row_start + col] = CHANGE_STATE(scell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, col);
					}
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_south]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_south_east]);
					secell = inboard[next_row_start + eastcol];
					if (SHOULD_CHANGE_STATE(secell)) {
						inboard[next_row_start + eastcol] = CHANGE_STATE(secell);
						my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, eastcol);
					}
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_east]);
				}
			}

			/* if the cell lies on the top edge of the thread */
			else if (unlikely(row == start_row)) {
				/* handle horizontal elements */
				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
				wcell = inboard[row_start + westcol];
				if (SHOULD_CHANGE_STATE(wcell)) {
					inboard[row_start + westcol] = CHANGE_STATE(wcell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, westcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + col]);
				cell = inboard[row_start + col];
				if (SHOULD_CHANGE_STATE(cell)) {
					inboard[row_start + col] = CHANGE_STATE(cell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, col);
				}
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + col]);


				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
				ecell = inboard[row_start + eastcol];
				if (SHOULD_CHANGE_STATE(ecell)) {
					inboard[row_start + eastcol] = CHANGE_STATE(ecell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, eastcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);

				/* handle north elements */
				int prev_thread_lock_index;
				if (likely(start_row > 0)) {
					prev_thread_lock_index = my_lock_start_index - num_cols;
				}
				else if (unlikely(start_row == 0)) {
					// assert(myid == 0);
					// assert(northrow == num_rows - 1);
					prev_thread_lock_index = last_thread_lock_start_index + num_cols;
					// assert(prev_thread_lock_index + 1024 == num_cols * NUM_THREADS * 2);
				}
				
				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + westcol]);
				nwcell = inboard[prev_row_start + westcol];
				if (SHOULD_CHANGE_STATE(nwcell)) {
					inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
					prev_change_list_to_write[(*prev_cl_index_to_write)++] = SET_ROW_COL(northrow, westcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + col]);
				ncell = inboard[prev_row_start + col];
				if (SHOULD_CHANGE_STATE(ncell)) {
					inboard[prev_row_start + col] = CHANGE_STATE(ncell);
					prev_change_list_to_write[(*prev_cl_index_to_write)++] = SET_ROW_COL(northrow, col);
				}
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);
				necell = inboard[prev_row_start + eastcol];
				if (SHOULD_CHANGE_STATE(necell)) {
					inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
					prev_change_list_to_write[(*prev_cl_index_to_write)++] = SET_ROW_COL(northrow, eastcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);

				swcell = inboard[next_row_start + westcol];
				scell  = inboard[next_row_start + col];
				secell = inboard[next_row_start + eastcol];
				/* handle south elements */
				if (SHOULD_CHANGE_STATE(swcell)) {
					inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, westcol);
				}
				if (SHOULD_CHANGE_STATE(scell)) {
					inboard[next_row_start + col] = CHANGE_STATE(scell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, col);
				}
				if (SHOULD_CHANGE_STATE(secell)) {
					inboard[next_row_start + eastcol] = CHANGE_STATE(secell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(southrow, eastcol);
				}
			}
			/* if the cell lies on the bottom edge of this thread's region */
			else if (unlikely(row == (end_row - 1))) {

				/* handle horizontal elements */
				const int lock_index_west = my_lock_start_index + num_cols + westcol;
				const int lock_index_middle = my_lock_start_index + num_cols + col;
				const int lock_index_east = my_lock_start_index + num_cols + eastcol;

				pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
				wcell = inboard[row_start + westcol];
				if (SHOULD_CHANGE_STATE(wcell)) {
					inboard[row_start + westcol] = CHANGE_STATE(wcell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, westcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

				pthread_mutex_lock(&per_thread_element_locks[lock_index_middle]);
				cell = inboard[row_start + col];
				if (SHOULD_CHANGE_STATE(cell)) {
					inboard[row_start + col] = CHANGE_STATE(cell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, col);
				}
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_middle]);

				pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
				ecell = inboard[row_start + eastcol];
				if (SHOULD_CHANGE_STATE(ecell)) {
					inboard[row_start + eastcol] = CHANGE_STATE(ecell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(row, eastcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);

				nwcell = inboard[prev_row_start + westcol];
				ncell = inboard[prev_row_start + col];
				necell = inboard[prev_row_start + eastcol];
				/* handle north elements */
				if (SHOULD_CHANGE_STATE(nwcell)) {
					inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, westcol);
				}
				if (SHOULD_CHANGE_STATE(ncell)) {
					inboard[prev_row_start + col] = CHANGE_STATE(ncell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, col);
				}
				if (SHOULD_CHANGE_STATE(necell)) {
					inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
					my_change_list[(*my_cl_index)++] = SET_ROW_COL(northrow, eastcol);
				}

				/* handle south elements */
				int next_thread_lock_index;
				if (likely(row < (num_rows - 1))) {
					next_thread_lock_index = my_lock_start_index + num_cols + num_cols;
				}
				else if (unlikely(row == (num_rows - 1))) {
					next_thread_lock_index = 0;
				}
				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + westcol]);
				swcell = inboard[next_row_start + westcol];
				if (SHOULD_CHANGE_STATE(swcell)) {
					inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
					next_change_list_to_write[(*next_cl_index_to_write)++] = SET_ROW_COL(southrow, westcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + col]);
				scell  = inboard[next_row_start + col];
				if (SHOULD_CHANGE_STATE(scell)) {
					inboard[next_row_start + col] = CHANGE_STATE(scell);
					next_change_list_to_write[(*next_cl_index_to_write)++] = SET_ROW_COL(southrow, col);
				}
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
				secell = inboard[next_row_start + eastcol];
				if (SHOULD_CHANGE_STATE(secell)) {
					inboard[next_row_start + eastcol] = CHANGE_STATE(secell);
					next_change_list_to_write[(*next_cl_index_to_write)++] = SET_ROW_COL(southrow, eastcol);
				}
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
			}
		}
		// increment number of threads done.
		pthread_mutex_lock(&thread_done_mutex);
		threads_done++;

		if (likely(threads_done != NUM_THREADS)) {
			pthread_cond_wait(&thread_done_cond, &thread_done_mutex);
			pthread_mutex_unlock(&thread_done_mutex);
		}
		else if (unlikely(threads_done == NUM_THREADS)) {
			threads_done = 0;
			pthread_cond_broadcast(&thread_done_cond);
			pthread_mutex_unlock(&thread_done_mutex);
		}

		// update all neighbour counts of change list neighbors
		int change_list_right = *my_cl_index;
		for (j = 0; j < change_list_right; j++) {
			row = GET_ROW(my_change_list[j]);
			col = GET_COL(my_change_list[j]);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;
			cell = inboard[row_start + col];
			// since this cell is in the change list, if it is alive, then it means
			// the cell changed from dead to alive, and thus all of its neighbours now have 1 more neighbour
			if (IS_ALIVE(cell)) {
				// if the cell is within the threads board
				if (likely(row > start_row && row < (end_row - 1))) {
					inboard[row_start + westcol]++;
					inboard[row_start + eastcol]++;
					if (likely(northrow > start_row)) {
						inboard[prev_row_start + westcol]++;
						inboard[prev_row_start + col]++;
						inboard[prev_row_start + eastcol]++;
					}
					else if (unlikely(northrow == start_row)) {
						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
						inboard[prev_row_start + westcol]++;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + col]);
						inboard[prev_row_start + col]++;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + col]);

						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
						inboard[prev_row_start + eastcol]++;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);
					}
					if (likely(southrow < (end_row - 1))) {
						inboard[next_row_start + westcol]++;
						inboard[next_row_start + col]++;
						inboard[next_row_start + eastcol]++;
					}
					else if (unlikely(southrow == (end_row - 1))) {
						const int lock_index_south_west = my_lock_start_index + num_cols + westcol;
						const int lock_index_south = my_lock_start_index + num_cols + col;
						const int lock_index_south_east = my_lock_start_index + num_cols + eastcol;

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_west]);
						inboard[next_row_start + westcol]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_west]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south]);
						inboard[next_row_start + col]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_east]);
						inboard[next_row_start + eastcol]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_east]);
					}
				}
				// if the cell lies on the top edge of the thread's board
				else if (unlikely(row == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
					inboard[row_start + westcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
					inboard[row_start + eastcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);


					int prev_thread_lock_index;
					if (likely(start_row > 0)) {
						prev_thread_lock_index = my_lock_start_index - num_cols;
					}
					else if (unlikely(start_row == 0)) {
						prev_thread_lock_index = last_thread_lock_start_index + num_cols;
					}
					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + westcol]);
					inboard[prev_row_start + westcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + col]);
					inboard[prev_row_start + col]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + col]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);
					inboard[prev_row_start + eastcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);

					inboard[next_row_start + westcol]++;
					inboard[next_row_start + col]++;
					inboard[next_row_start + eastcol]++;
				}
				// if the cell lies on the bottom edge of the thread's board
				else if (unlikely(row == (end_row - 1))) {
					const int lock_index_west = my_lock_start_index + num_cols + westcol;
					const int lock_index_east = my_lock_start_index + num_cols + eastcol;

					pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
					inboard[row_start + westcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
					inboard[row_start + eastcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);


					inboard[prev_row_start + westcol]++;
					inboard[prev_row_start + col]++;
					inboard[prev_row_start + eastcol]++;


					int next_thread_lock_index;
					if (likely(row < (num_rows - 1))) {
						next_thread_lock_index = my_lock_start_index + num_cols + num_cols;
					}
					else if (unlikely(row == (num_rows - 1))) {
						next_thread_lock_index = 0;
					}
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + westcol]);
					inboard[next_row_start + westcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + col]);
					inboard[next_row_start + col]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + col]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
					inboard[next_row_start + eastcol]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
 
				}
			}
			// since this cell is in the change list, if it is dead, then it means
			// the cell changed from alive to dead, and thus all of its neighbours now have 1 less neighbour
			else if (!IS_ALIVE(cell)) {
				// if the cell is within the threads board
				if (likely(row > start_row && row < (end_row - 1))) {
					inboard[row_start + westcol]--;
					inboard[row_start + eastcol]--;
					if (likely(northrow > start_row)) {
						inboard[prev_row_start + westcol]--;
						inboard[prev_row_start + col]--;
						inboard[prev_row_start + eastcol]--;
					}
					else if (unlikely(northrow == start_row)) {
						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
						inboard[prev_row_start + westcol]--;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + col]);
						inboard[prev_row_start + col]--;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + col]);

						pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
						inboard[prev_row_start + eastcol]--;
						pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);
					}
					if (likely(southrow < (end_row - 1))) {
						inboard[next_row_start + westcol]--;
						inboard[next_row_start + col]--;
						inboard[next_row_start + eastcol]--;
					}
					else if (unlikely(southrow == (end_row - 1))) {
						const int lock_index_south_west = my_lock_start_index + num_cols + westcol;
						const int lock_index_south = my_lock_start_index + num_cols + col;
						const int lock_index_south_east = my_lock_start_index + num_cols + eastcol;

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_west]);
						inboard[next_row_start + westcol]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_west]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south]);
						inboard[next_row_start + col]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_east]);
						inboard[next_row_start + eastcol]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_east]);
					}
				}
				// if the cell lies on the top edge of the thread's board
				else if (unlikely(row == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
					inboard[row_start + westcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
					inboard[row_start + eastcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);


					int prev_thread_lock_index;
					if (likely(start_row > 0)) {
						prev_thread_lock_index = my_lock_start_index - num_cols;
					}
					else if (unlikely(start_row == 0)) {
						prev_thread_lock_index = last_thread_lock_start_index + num_cols;
					}
					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + westcol]);
					inboard[prev_row_start + westcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + col]);
					inboard[prev_row_start + col]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + col]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);
					inboard[prev_row_start + eastcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);

					inboard[next_row_start + westcol]--;
					inboard[next_row_start + col]--;
					inboard[next_row_start + eastcol]--;
				}
				// if the cell lies on the bottom edge of the thread's board
				else if (unlikely(row == (end_row - 1))) {
					const int lock_index_west = my_lock_start_index + num_cols + westcol;
					const int lock_index_east = my_lock_start_index + num_cols + eastcol;

					pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
					inboard[row_start + westcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
					inboard[row_start + eastcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);


					inboard[prev_row_start + westcol]--;
					inboard[prev_row_start + col]--;
					inboard[prev_row_start + eastcol]--;


					int next_thread_lock_index;
					if (likely(row < (num_rows - 1))) {
						next_thread_lock_index = my_lock_start_index + num_cols + num_cols;
					}
					else if (unlikely(row == (num_rows - 1))) {
						next_thread_lock_index = 0;
					}
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + westcol]);
					inboard[next_row_start + westcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + westcol]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + col]);
					inboard[next_row_start + col]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + col]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
					inboard[next_row_start + eastcol]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
				}
			}
		}
		

		int prev_thread_change_list_to_read_right = *prev_cl_index_to_read;
		// fprintf()
		// update changes provided by prev_thread
		for (j = 0; j < prev_thread_change_list_to_read_right; j++) {
			int row_col = my_change_list_to_read_from_prev_thread[j];
			// append this change caught by previous thread to my change list
			my_change_list[(*my_cl_index)++] = row_col;

			row = GET_ROW(row_col);
			// assert(row == start_row);
			col = GET_COL(row_col);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;
			cell = inboard[row_start + col];
			// since this cell is provided by the change list written by the prev thread
			// it must lie at the top row of my thread region
			if (IS_ALIVE(cell)) {
				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
				inboard[row_start + westcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
				inboard[row_start + eastcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);


				int prev_thread_lock_index;
				if (likely(start_row > 0)) {
					prev_thread_lock_index = my_lock_start_index - num_cols;
				}
				else if (unlikely(start_row == 0)) {
					prev_thread_lock_index = last_thread_lock_start_index + num_cols;
				}
				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + westcol]);
				inboard[prev_row_start + westcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + col]);
				inboard[prev_row_start + col]++;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);
				inboard[prev_row_start + eastcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);

				inboard[next_row_start + westcol]++;
				inboard[next_row_start + col]++;
				inboard[next_row_start + eastcol]++;
			}
			// since this cell is in the change list, if it is dead, then it means
			// the cell changed from alive to dead, and thus all of its neighbours now have 1 less neighbour
			else if (!IS_ALIVE(cell)) {
				// if the cell lies on the top edge of the thread's board
				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + westcol]);
				inboard[row_start + westcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[my_lock_start_index + eastcol]);
				inboard[row_start + eastcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[my_lock_start_index + eastcol]);


				int prev_thread_lock_index;
				if (likely(start_row > 0)) {
					prev_thread_lock_index = my_lock_start_index - num_cols;
				}
				else if (unlikely(start_row == 0)) {
					prev_thread_lock_index = last_thread_lock_start_index + num_cols;
				}
				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + westcol]);
				inboard[prev_row_start + westcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + col]);
				inboard[prev_row_start + col]--;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);
				inboard[prev_row_start + eastcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + eastcol]);

				inboard[next_row_start + westcol]--;
				inboard[next_row_start + col]--;
				inboard[next_row_start + eastcol]--;
			}
		}


		int next_thread_change_list_to_read_right = *next_cl_index_to_read;
		// update changes provided by prev_thread
		for (j = 0; j < next_thread_change_list_to_read_right; j++) {
			int row_col = my_change_list_to_read_from_next_thread[j];
			my_change_list[(*my_cl_index)++] = row_col;
			row = GET_ROW(row_col);
			// assert(row != (end_row - 1));
			col = GET_COL(row_col);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;
			cell = inboard[row_start + col];
			// since this cell is it being read from the changelist written to by the next thread
			// it must lie on the bottom edge of this thread's region
			if (IS_ALIVE(cell)) {
				const int lock_index_west = my_lock_start_index + num_cols + westcol;
				const int lock_index_east = my_lock_start_index + num_cols + eastcol;

				pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
				inboard[row_start + westcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

				pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
				inboard[row_start + eastcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);


				inboard[prev_row_start + westcol]++;
				inboard[prev_row_start + col]++;
				inboard[prev_row_start + eastcol]++;


				int next_thread_lock_index;
				if (likely(row < (num_rows - 1))) {
					next_thread_lock_index = my_lock_start_index + num_cols + num_cols;
				}
				else if (unlikely(row == (num_rows - 1))) {
					next_thread_lock_index = 0;
				}
				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + westcol]);
				inboard[next_row_start + westcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + col]);
				inboard[next_row_start + col]++;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
				inboard[next_row_start + eastcol]++;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
			}
			else if (!IS_ALIVE(cell)) {
				const int lock_index_west = my_lock_start_index + num_cols + westcol;
				const int lock_index_east = my_lock_start_index + num_cols + eastcol;

				pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
				inboard[row_start + westcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

				pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
				inboard[row_start + eastcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);


				inboard[prev_row_start + westcol]--;
				inboard[prev_row_start + col]--;
				inboard[prev_row_start + eastcol]--;


				int next_thread_lock_index;
				if (likely(row < (num_rows - 1))) {
					next_thread_lock_index = my_lock_start_index + num_cols + num_cols;
				}
				else if (unlikely(row == (num_rows - 1))) {
					next_thread_lock_index = 0;
				}
				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + westcol]);
				inboard[next_row_start + westcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + westcol]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + col]);
				inboard[next_row_start + col]--;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + col]);

				pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
				inboard[next_row_start + eastcol]--;
				pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_index + eastcol]);
			}
		}

	
		swap_cl_index = *my_cl_index;
		// reset change lists to read
		memcpy(swap_change_list, my_change_list, sizeof(int) * swap_cl_index);
		*my_cl_index = 0;
		*prev_cl_index_to_read = 0;
		*next_cl_index_to_read = 0;
		pthread_mutex_lock(&thread_done_mutex);
		threads_done++;

		if (unlikely(threads_done == NUM_THREADS)) {
			threads_done = 0;
			pthread_cond_broadcast(&thread_done_cond);
			pthread_mutex_unlock(&thread_done_mutex);
			continue;
		}
		pthread_cond_wait(&thread_done_cond, &thread_done_mutex);
		pthread_mutex_unlock(&thread_done_mutex);
	}
	for (i = start_row; i < end_row; i++) {
		row_start = i * num_cols;
		for (j = 0; j < num_cols; j++) {
			inboard[row_start + j] = (inboard[row_start + j] >> 4) + '0';
		}
	}
	return NULL;
}

char *game_of_life(char* inboard, const int nrows, const int ncols, const int gens_max) {
	int i;
	in_board = inboard;
	num_cols = ncols;
	num_rows = nrows;
	num_gens = gens_max;
	threads_done = 0;
	gen = 0;
	// printf("%d\n", sizeof(pthread_mutex_t));
	pthread_mutex_init(&thread_done_mutex, NULL);
	pthread_cond_init(&thread_done_cond, NULL);
	pthread_mutex_init(&thread_go_mutex, NULL);
	pthread_cond_init(&thread_go_cond, NULL);
	// pthread_mutex_init(&update_neighbour_count_mutex, NULL);
	// pthread_cond_init(&update_neighbour_count_cond, NULL);

	int total_num_locks = NUM_THREADS * 2 * ncols;
	per_thread_element_locks = malloc(sizeof(pthread_mutex_t) * total_num_locks);

	for (i = 0; i < total_num_locks; i++) {
		pthread_mutex_init(&per_thread_element_locks[i], NULL);
	}

	initialize_change_lists(nrows, ncols);
	// fprintf(stdout, "Finished lock init.\n");
	// fflush(stdout);
	pthread_t threads[NUM_THREADS];
	// cpu_set_t cpusets[NUM_THREADS];
	// pthread_mutex_lock(&thread_done_mutex);

	for (i = 0; i < NUM_THREADS; i++) {
		// CPU_ZERO(&cpusets[i]);
		// CPU_SET(i, &cpusets[i]);
		
		int *id = malloc(sizeof(int));
		*id = i;
		pthread_create(&threads[i], NULL, &game_of_life_thread, id);
		// pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpusets[i]);
	}
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	// print_board(inboard, nrows, ncols);
	// fprintf(stdout, "Done.\n");
	// fflush(stdout);
	return inboard;
}
