/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
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
#define NEXT_CHANGE_LIST(id) (((id) + 1) % NUM_THREADS)

typedef struct lock_t {
    pthread_mutex_t lock;
    int padding[6];
} lock_t;


static char *in_board;
// contains indices of cells that have been change in prev iteration.
static unsigned int curr_change_list_index;
static unsigned int change_list_lens[NUM_THREADS] __attribute__ ((aligned(64)));
static unsigned int *change_lists[NUM_THREADS] __attribute__ ((aligned(64)));
static unsigned int *total_change_list;
static unsigned int total_change_list_index;
static unsigned int num_rows;
static unsigned int num_cols;
static unsigned int num_gens;
static unsigned int gen;
static unsigned int threads_done;

static pthread_mutex_t *element_locks;
pthread_mutex_t change_list_lock;
pthread_mutex_t total_change_list_lock;
static pthread_mutex_t update_neighbour_count_mutex;
static pthread_cond_t update_neighbour_count_cond;
static pthread_mutex_t thread_done_mutex;
static pthread_cond_t thread_done_cond;
static pthread_cond_t thread_go_cond;
static pthread_mutex_t thread_go_mutex;

static inline void initialize_change_lists(unsigned int nrows, unsigned int ncols) {
	int area = nrows * ncols / NUM_THREADS;
	int row_start;
	pthread_mutex_init(&change_list_lock, NULL);
	pthread_mutex_init(&total_change_list_lock, NULL);
	total_change_list_index = 0;
	total_change_list = malloc(nrows * ncols * sizeof(int));
	curr_change_list_index = 0;
	for (int i = 0; i < NUM_THREADS; i++) {
		change_lists[i] = calloc(area, sizeof(int));
		change_list_lens[i] = 0;
	}

	unsigned int change_list_id = 0;
	for (int i = 0; i < nrows; i++) {
		row_start = i * num_cols;
		for (int j = 0; j < num_cols; j++) {
			if (!IS_ALIVE(in_board[row_start + j])) {
				continue;
			}
			change_lists[change_list_id][(change_list_lens[change_list_id])++] = SET_ROW_COL(i, j);
			change_list_id = NEXT_CHANGE_LIST(change_list_id);
		}
	}
}

void *game_of_life_thread(void *id) {
	int myid = *(int *)id;
	free(id);
	char *inboard = in_board;
	unsigned int *my_change_list = change_lists[myid];
	unsigned int my_len = change_list_lens[myid];
	int num_rows_to_handle = num_rows / NUM_THREADS;
	int start_row = myid * num_rows_to_handle;
	int end_row = start_row + num_rows_to_handle;
	unsigned int my_area = num_rows_to_handle * num_cols;

	unsigned int prev_row_start;
	unsigned int row_start;
	unsigned int next_row_start;
	char cell, wcell, ecell, nwcell, ncell, necell, swcell, scell, secell;
	int westcol, eastcol, northrow, southrow;
	int curgen, i, j, row, col;
	int gens_max = num_gens;
	unsigned int *local_change_list = calloc(my_area, sizeof(unsigned int));
	unsigned int local_change_list_index = 0;
	// initialize
	for (curgen = 0; curgen < gens_max; curgen++) {
		// fprintf(stdout, "outboard: 0x%x\n", outboard);
		// fflush(stdout);
		// fprintf(stdout, "On gen %d...", curgen);
		// fflush(stdout);
		for (i = 0; i < my_len; i++) {
			row = GET_ROW(my_change_list[i]);
			col = GET_COL(my_change_list[i]);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;

			pthread_mutex_lock(&(element_locks[row_start + westcol]));
			wcell = inboard[row_start + westcol];
			if (SHOULD_CHANGE_STATE(wcell)) {
				inboard[row_start + westcol] = CHANGE_STATE(wcell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(row, westcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(row, westcol);
			}
			pthread_mutex_unlock(&(element_locks[row_start + westcol]));

			pthread_mutex_lock(&(element_locks[row_start + eastcol]));
			ecell = inboard[row_start + eastcol];
			if (SHOULD_CHANGE_STATE(ecell)) {
				inboard[row_start + eastcol] = CHANGE_STATE(ecell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(row, eastcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(row, eastcol);
			}
			pthread_mutex_unlock(&(element_locks[row_start + eastcol]));

			pthread_mutex_lock(&(element_locks[prev_row_start + westcol]));
			nwcell = inboard[prev_row_start + westcol];
			if (SHOULD_CHANGE_STATE(nwcell)) {
				inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(northrow, westcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(northrow, westcol);
			}
			pthread_mutex_unlock(&(element_locks[prev_row_start + westcol]));

			pthread_mutex_lock(&(element_locks[prev_row_start + col]));
			ncell = inboard[prev_row_start + col];
			if (SHOULD_CHANGE_STATE(ncell)) {
				inboard[prev_row_start + col] = CHANGE_STATE(ncell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(northrow, col);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(northrow, col);
			}
			pthread_mutex_unlock(&(element_locks[prev_row_start + col]));

			pthread_mutex_lock(&(element_locks[prev_row_start + eastcol]));
			necell = inboard[prev_row_start + eastcol];
			if (SHOULD_CHANGE_STATE(necell)) {
				inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(northrow, eastcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(northrow, eastcol);
			}
			pthread_mutex_unlock(&(element_locks[prev_row_start + eastcol]));

			pthread_mutex_lock(&(element_locks[next_row_start + westcol]));
			swcell = inboard[next_row_start + westcol];
			if (SHOULD_CHANGE_STATE(swcell)) {
				inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(southrow, westcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(southrow, westcol);
			}
			pthread_mutex_unlock(&(element_locks[next_row_start + westcol]));

			pthread_mutex_lock(&(element_locks[next_row_start + col]));
			scell = inboard[next_row_start + col];
			if (SHOULD_CHANGE_STATE(scell)) {
				inboard[next_row_start + col] = CHANGE_STATE(scell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(southrow, col);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(southrow, col);
			}
			pthread_mutex_unlock(&(element_locks[next_row_start + col]));

			pthread_mutex_lock(&(element_locks[next_row_start + eastcol]));
			secell = inboard[next_row_start + eastcol];
			if (SHOULD_CHANGE_STATE(secell)) {
				inboard[next_row_start + eastcol] = CHANGE_STATE(secell);
				pthread_mutex_lock(&total_change_list_lock);
				total_change_list[total_change_list_index++] = SET_ROW_COL(southrow, eastcol);
				pthread_mutex_unlock(&total_change_list_lock);
				local_change_list[local_change_list_index++] = SET_ROW_COL(southrow, eastcol);
			}
			pthread_mutex_unlock(&(element_locks[next_row_start + eastcol]));
		}
		// fprintf(stdout, "On gen %d...", curgen);
		// fflush(stdout);
		// increment number of threads done.
		pthread_mutex_lock(&update_neighbour_count_mutex);
		threads_done++;

		if (likely(threads_done != NUM_THREADS)) {
			pthread_cond_wait(&update_neighbour_count_cond, &update_neighbour_count_mutex);
			pthread_mutex_unlock(&update_neighbour_count_mutex);
		}
		else if (unlikely(threads_done == NUM_THREADS)) {
			threads_done = 0;
			pthread_cond_broadcast(&update_neighbour_count_cond);
			pthread_mutex_unlock(&update_neighbour_count_mutex);
		}

		for (i = 0; i < local_change_list_index; i++) {
			row = GET_ROW(local_change_list[i]);
			col = GET_COL(local_change_list[i]);
			westcol = mod(col-1, num_cols);
			eastcol = mod(col+1, num_cols);
			northrow = mod(row-1, num_rows);
			southrow = mod(row+1, num_rows);
			row_start = row * num_cols;
			prev_row_start = northrow * num_cols;
			next_row_start = southrow * num_cols;
			
			cell = inboard[row_start + col];
			if (IS_ALIVE(cell)) {
				pthread_mutex_lock(&(element_locks[row_start + westcol]));
				inboard[row_start + westcol]++;
				pthread_mutex_unlock(&(element_locks[row_start + westcol]));

				pthread_mutex_lock(&(element_locks[row_start + eastcol]));
				inboard[row_start + eastcol]++;
				pthread_mutex_unlock(&(element_locks[row_start + eastcol]));

				pthread_mutex_lock(&(element_locks[prev_row_start + westcol]));
				inboard[prev_row_start + westcol]++;
				pthread_mutex_unlock(&(element_locks[prev_row_start + westcol]));

				pthread_mutex_lock(&(element_locks[prev_row_start + col]));
				inboard[prev_row_start + col]++;
				pthread_mutex_unlock(&(element_locks[prev_row_start + col]));

				pthread_mutex_lock(&(element_locks[prev_row_start + eastcol]));
				inboard[prev_row_start + eastcol]++;
				pthread_mutex_unlock(&(element_locks[prev_row_start + eastcol]));

				pthread_mutex_lock(&(element_locks[next_row_start + westcol]));
				inboard[next_row_start + westcol]++;
				pthread_mutex_unlock(&(element_locks[next_row_start + westcol]));

				pthread_mutex_lock(&(element_locks[next_row_start + col]));
				inboard[next_row_start + col]++;
				pthread_mutex_unlock(&(element_locks[next_row_start + col]));

				pthread_mutex_lock(&(element_locks[next_row_start + eastcol]));
				inboard[next_row_start + eastcol]++;
				pthread_mutex_unlock(&(element_locks[next_row_start + eastcol]));
			}
			else if (!IS_ALIVE(cell)) {
				pthread_mutex_lock(&(element_locks[row_start + westcol]));
				inboard[row_start + westcol]--;
				pthread_mutex_unlock(&(element_locks[row_start + westcol]));

				pthread_mutex_lock(&(element_locks[row_start + eastcol]));
				inboard[row_start + eastcol]--;
				pthread_mutex_unlock(&(element_locks[row_start + eastcol]));

				pthread_mutex_lock(&(element_locks[prev_row_start + westcol]));
				inboard[prev_row_start + westcol]--;
				pthread_mutex_unlock(&(element_locks[prev_row_start + westcol]));

				pthread_mutex_lock(&(element_locks[prev_row_start + col]));
				inboard[prev_row_start + col]--;
				pthread_mutex_unlock(&(element_locks[prev_row_start + col]));

				pthread_mutex_lock(&(element_locks[prev_row_start + eastcol]));
				inboard[prev_row_start + eastcol]--;
				pthread_mutex_unlock(&(element_locks[prev_row_start + eastcol]));

				pthread_mutex_lock(&(element_locks[next_row_start + westcol]));
				inboard[next_row_start + westcol]--;
				pthread_mutex_unlock(&(element_locks[next_row_start + westcol]));

				pthread_mutex_lock(&(element_locks[next_row_start + col]));
				inboard[next_row_start + col]--;
				pthread_mutex_unlock(&(element_locks[next_row_start + col]));

				pthread_mutex_lock(&(element_locks[next_row_start + eastcol]));
				inboard[next_row_start + eastcol]--;
				pthread_mutex_unlock(&(element_locks[next_row_start + eastcol]));
			}
		}
	
		local_change_list_index = 0;
		if (likely(myid != (NUM_THREADS - 1))) {
			my_len = total_change_list_index / NUM_THREADS;
			int start = myid * my_len;
			for (i = 0; i < my_len; i++) {
				my_change_list[i] = total_change_list[start + i];
			}
		}
		else if (unlikely(myid == (NUM_THREADS - 1))) {
			int their_len = total_change_list_index / NUM_THREADS;
			my_len = total_change_list_index - (NUM_THREADS - 1) * their_len;
			int start = myid * their_len;
			for (i = 0; i < my_len; i++) {
				my_change_list[i] = total_change_list[start + i];
			}
		}
		
		pthread_mutex_lock(&thread_done_mutex);
		threads_done++;

		if (unlikely(threads_done == NUM_THREADS)) {
			threads_done = 0;
			total_change_list_index = 0;
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
	int area = nrows * ncols;
	gen = 0;
	threads_done = 0;
	// printf("%d\n", sizeof(pthread_mutex_t));
	pthread_mutex_init(&thread_done_mutex, NULL);
	pthread_cond_init(&thread_done_cond, NULL);
	pthread_mutex_init(&thread_go_mutex, NULL);
	pthread_cond_init(&thread_go_cond, NULL);
	pthread_mutex_init(&update_neighbour_count_mutex, NULL);
	pthread_cond_init(&update_neighbour_count_cond, NULL);

	element_locks = malloc(sizeof(pthread_mutex_t) * area);

	for (i = 0; i < area; i++) {
		pthread_mutex_init(&element_locks[i], NULL);
	}

	initialize_change_lists(nrows, ncols);
	// fprintf(stdout, "Finished lock init.\n");
	// fflush(stdout);
	pthread_t threads[NUM_THREADS];
	cpu_set_t cpusets[NUM_THREADS];
	// pthread_mutex_lock(&thread_done_mutex);

	for (i = 0; i < NUM_THREADS; i++) {
		CPU_ZERO(&cpusets[i]);
		CPU_SET(i, &cpusets[i]);
		
		int *id = malloc(sizeof(int));
		*id = i;
		pthread_create(&threads[i], NULL, &game_of_life_thread, id);
		pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpusets[i]);
	}
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	print_board(inboard, nrows, ncols);
	// fprintf(stdout, "Done.\n");
	// fflush(stdout);
	return inboard;
}
