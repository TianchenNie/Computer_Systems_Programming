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

static char *in_board;
static char *out_board;
// contains indices of cells that have been change in prev iteration.
static pthread_mutex_t *per_thread_element_locks;
static unsigned int num_rows;
static unsigned int num_cols;
static unsigned int num_gens;
static unsigned int gen;
static unsigned int threads_done;
static pthread_mutex_t thread_done_mutex;
static pthread_cond_t thread_done_cond;
static pthread_cond_t thread_go_cond;
static pthread_mutex_t thread_go_mutex;

static inline int get_bottom_row_lock_index(int element_id, int thread_num_rows, int num_cols) {
	return num_cols + (element_id - (thread_num_rows - 1) * num_cols);
}

void *game_of_life_thread(void *id) {
	int myid = *(int *)id;
	free(id);
	char *inboard = in_board;
	char *outboard = out_board;
	int num_rows_to_handle = num_rows / NUM_THREADS;
	int start_row = myid * num_rows_to_handle;
	int end_row = start_row + num_rows_to_handle;
	int second_row_start = (start_row + 1) * num_cols;
	int last_row_start = (end_row - 1) * num_cols;

	unsigned int *change_list1 = calloc(num_rows_to_handle * num_cols, sizeof(unsigned int));
	unsigned int *change_list2 = calloc(num_rows_to_handle * num_cols, sizeof(unsigned int));
	unsigned int change_list1_end = 0;
	unsigned int change_list2_end = 0;
	int lock_start_index = myid * num_cols * 2;
	int my_start_index = start_row * num_cols;
	int last_thread_lock_start_index = (NUM_THREADS - 1) * num_cols * 2;
	unsigned int prev_row_start, row_start, next_row_start;
	char cell;
	int curgen, cell_index, change_list_index, change_list_value, i, j;
	int gens_max = num_gens;

	// populate change list for this thread
	for (i = start_row; i < end_row; i++) {
		row_start = i * num_cols;
		for (j = 0; j < num_cols; j++) {
			if (!IS_ALIVE(inboard[row_start + j])) {
				continue;
			}
			curr_change_list1[change_list1_end++] = SET_ROW_COL(i, j);
		}
	}

	for (curgen = 0; curgen < gens_max; curgen++) {
		// fprintf(stdout, "outboard: 0x%x\n", outboard);
		// fflush(stdout);
		// loop through change list cells and update them and their neighbours
		for (change_list_index = 0; change_list_index < change_list1_end; change_list_index++) {
			change_list_value = change_list1[change_list_index];
			i = GET_ROW(change_list_value);
			j = GET_COL(change_list_value);
			row_start = i * num_cols;
			cell = inboard[row_start + j];
			// if a cell died last generation and should stay dead in this generation
			if (!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 3) {
				// scan every neighbor and check if that neighbor should live
				if (likely(i > start_row && i < (end_row - 1))) {
					int inorth = i - 1;
					int isouth i + 1;
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);
					// handle update west east neighbors
					change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, i, jwest, num_cols);
					change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, i, jeast, num_cols);
					if (likely(inorth > start_row)) {
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, inorth, jwest, num_cols);
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, inorth, j, num_cols);
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, inorth, jeast, num_cols);
					}
					else if (unlikely(inorth == start_row)) {
						
					}
					if (likely(isouth < (end_row - 1))) {
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, isouth, jwest, num_cols);
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, isouth, j, num_cols);
						change_list2_end += handle_update_neighbor(inboard, change_list2, change_list2_end, isouth, jeast, num_cols);
					}

					inboard[currindex] &= WAS_ALIVE_MASK;
					continue;
				}
				else if (unlikely(i == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
					inboard[currindex] &= WAS_ALIVE_MASK;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);
					continue;
				}
				else if (unlikely(i == (end_row - 1))) {
					int lock_index = lock_start_index + get_bottom_row_lock_index(row_start + j - my_start_index, num_rows_to_handle, num_cols);
					// fprintf(stdout, "lock_index: %d\n", lock_index);
					// fflush(stdout);
					pthread_mutex_lock(&per_thread_element_locks[lock_index]); 
					inboard[currindex] &= WAS_ALIVE_MASK;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index]);
					continue;
				}
			}
			else if (unlikely(!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) == 3)) {
				// this cell can live now	
				// its neighbors in the out board now have 1 extra neighbor
				cell += 0x10;
				// if the row to handle is between the top and bottom row of this thread
				if (likely(i > start_row && i < (end_row - 1))) {
					const int inorth = i - 1;
					const int isouth = i + 1;
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);

					// handle west and east neighbors
					outboard[row_start + j] += cell;
					outboard[row_start + jwest]++;
					outboard[row_start + jeast]++;

					// handle north neighbors, may access other regions, but not likely
					prev_row_start = row_start - num_cols;
					if (likely(inorth != start_row)) {
						outboard[prev_row_start + j]++;
						outboard[prev_row_start + jwest]++;
						outboard[prev_row_start + jeast]++;
					}
					else if (unlikely(inorth == start_row)) {
						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
						outboard[prev_row_start + j]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);

						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jwest]);
						outboard[prev_row_start + jwest]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jwest]);

						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jeast]);
						outboard[prev_row_start + jeast]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jeast]);
					}

					next_row_start = row_start + num_cols;
					if (likely(isouth < (end_row - 1))) {
						outboard[next_row_start + j]++;
						outboard[next_row_start + jwest]++;
						outboard[next_row_start + jeast]++;
					}
					else if (unlikely(isouth == (end_row - 1))) {
						const int lock_index_south = lock_start_index + get_bottom_row_lock_index(next_row_start + j - my_start_index, num_rows_to_handle, num_cols);
						const int lock_index_south_west = lock_start_index + get_bottom_row_lock_index(next_row_start + jwest - my_start_index, num_rows_to_handle, num_cols);
						const int lock_index_south_east = lock_start_index + get_bottom_row_lock_index(next_row_start + jeast - my_start_index, num_rows_to_handle, num_cols);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south]);
						outboard[next_row_start + j]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_west]);
						outboard[next_row_start + jwest]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_west]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_east]);
						outboard[next_row_start + jeast]++;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_east]);
					}
				}
				else if (unlikely(i == start_row)) {
					int inorth;
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);
					// handle incrementing self, west and east neighbors
					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jwest]);
					outboard[row_start + jwest]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jwest]);

					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jeast]);
					outboard[row_start + jeast]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jeast]);

					// handle north neighbors
					int prev_thread_lock_index;
					if (likely(i != 0)) {
						inorth = i - 1;
						prev_row_start = row_start - num_cols;
						prev_thread_lock_index = lock_start_index - num_cols;
					}
					else if (unlikely(i == 0)) {
						inorth = num_rows - 1;
						prev_row_start = inorth * num_cols;
						prev_thread_lock_index = last_thread_lock_start_index + num_cols;
					}

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + j]);
					outboard[prev_row_start + j]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + jwest]);
					outboard[prev_row_start + jwest]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + jwest]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + jeast]);
					outboard[prev_row_start + jeast]++;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + jeast]);

					// south neighbours won't access other thread regions (assuming num rows > 16), don't need to lock
					next_row_start = row_start + num_cols;
					outboard[next_row_start + j]++;
					outboard[next_row_start + jwest]++;
					outboard[next_row_start + jeast]++;
				}
				else if (unlikely(i == (end_row - 1))) {
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);

					// handle east west neighbors and self
					const int lock_index_self = lock_start_index + get_bottom_row_lock_index(row_start + j - my_start_index, num_rows_to_handle, num_cols);
					const int lock_index_west = lock_start_index + get_bottom_row_lock_index(row_start + jwest - my_start_index, num_rows_to_handle, num_cols);
					const int lock_index_east = lock_start_index + get_bottom_row_lock_index(row_start + jeast - my_start_index, num_rows_to_handle, num_cols);
					pthread_mutex_lock(&per_thread_element_locks[lock_index_self]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_self]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
					outboard[row_start + jwest]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
					outboard[row_start + jeast]++;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);

					// north neighbours won't access other thread regions (assuming num rows > 16), don't need to lock
					prev_row_start = row_start - num_cols;
					outboard[prev_row_start + j]++;
					outboard[prev_row_start + jwest]++;
					outboard[prev_row_start + jeast]++;

					// handle south neighbors
					int next_thread_lock_start_index = 0;
					if (likely(i < (num_rows - 1))) {
						next_row_start = row_start + num_cols;
						next_thread_lock_start_index = lock_start_index + num_cols + num_cols;
					}
					else if (unlikely(i == (num_rows - 1))) {
						next_row_start = 0;
						next_thread_lock_start_index = 0;
					}
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + j]);
					outboard[next_row_start + j]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + jwest]);
					outboard[next_row_start + jwest]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + jwest]);
					
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + jeast]);
					outboard[next_row_start + jeast]++;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + jeast]);
				}
			}
			else if (unlikely(IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 2 && NUM_NEIGHBOURS(cell) != 3)) {
				// This cell should die
				// it's neighbors in the out board now have 1 less neighbor
				cell -= 0x10;
				// if the row to handle is between the top and bottom row of this thread
				if (likely(i > start_row && i < (end_row - 1))) {
					const int inorth = i - 1;
					const int isouth = i + 1;
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);

					// handle west and east neighbors
					outboard[row_start + j] += cell;
					outboard[row_start + jwest]--;
					outboard[row_start + jeast]--;

					// handle north neighbors, may access other regions, but not likely
					prev_row_start = row_start - num_cols;
					if (likely(inorth != start_row)) {
						outboard[prev_row_start + j]--;
						outboard[prev_row_start + jwest]--;
						outboard[prev_row_start + jeast]--;
					}
					else if (unlikely(inorth == start_row)) {
						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
						outboard[prev_row_start + j]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);

						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jwest]);
						outboard[prev_row_start + jwest]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jwest]);

						pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jeast]);
						outboard[prev_row_start + jeast]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jeast]);
					}

					next_row_start = row_start + num_cols;
					if (likely(isouth < (end_row - 1))) {
						outboard[next_row_start + j]--;
						outboard[next_row_start + jwest]--;
						outboard[next_row_start + jeast]--;
					}
					else if (unlikely(isouth == (end_row - 1))) {
						const int lock_index_south = lock_start_index + get_bottom_row_lock_index(next_row_start + j - my_start_index, num_rows_to_handle, num_cols);
						const int lock_index_south_west = lock_start_index + get_bottom_row_lock_index(next_row_start + jwest - my_start_index, num_rows_to_handle, num_cols);
						const int lock_index_south_east = lock_start_index + get_bottom_row_lock_index(next_row_start + jeast - my_start_index, num_rows_to_handle, num_cols);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south]);
						outboard[next_row_start + j]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_west]);
						outboard[next_row_start + jwest]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_west]);

						pthread_mutex_lock(&per_thread_element_locks[lock_index_south_east]);
						outboard[next_row_start + jeast]--;
						pthread_mutex_unlock(&per_thread_element_locks[lock_index_south_east]);
					}
				}
				else if (unlikely(i == start_row)) {
					int inorth;
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);
					// handle incrementing self, west and east neighbors
					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jwest]);
					outboard[row_start + jwest]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jwest]);

					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + jeast]);
					outboard[row_start + jeast]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + jeast]);

					// handle north neighbors
					int prev_thread_lock_index;
					if (likely(i != 0)) {
						inorth = i - 1;
						prev_row_start = row_start - num_cols;
						prev_thread_lock_index = lock_start_index - num_cols;
					}
					else if (unlikely(i == 0)) {
						inorth = num_rows - 1;
						prev_row_start = inorth * num_cols;
						prev_thread_lock_index = last_thread_lock_start_index + num_cols;
					}

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + j]);
					outboard[prev_row_start + j]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + jwest]);
					outboard[prev_row_start + jwest]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + jwest]);

					pthread_mutex_lock(&per_thread_element_locks[prev_thread_lock_index + jeast]);
					outboard[prev_row_start + jeast]--;
					pthread_mutex_unlock(&per_thread_element_locks[prev_thread_lock_index + jeast]);

					// south neighbours won't access other thread regions (assuming num rows > 16), don't need to lock
					next_row_start = row_start + num_cols;
					outboard[next_row_start + j]--;
					outboard[next_row_start + jwest]--;
					outboard[next_row_start + jeast]--;
				}
				else if (unlikely(i == (end_row - 1))) {
					const int jwest = mod(j-1, num_cols);
					const int jeast = mod(j+1, num_cols);

					// handle east west neighbors and self
					const int lock_index_self = lock_start_index + get_bottom_row_lock_index(row_start + j - my_start_index, num_rows_to_handle, num_cols);
					const int lock_index_west = lock_start_index + get_bottom_row_lock_index(row_start + jwest - my_start_index, num_rows_to_handle, num_cols);
					const int lock_index_east = lock_start_index + get_bottom_row_lock_index(row_start + jeast - my_start_index, num_rows_to_handle, num_cols);
					pthread_mutex_lock(&per_thread_element_locks[lock_index_self]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_self]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_west]);
					outboard[row_start + jwest]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_west]);

					pthread_mutex_lock(&per_thread_element_locks[lock_index_east]);
					outboard[row_start + jeast]--;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index_east]);

					// north neighbours won't access other thread regions (assuming num rows > 16), don't need to lock
					prev_row_start = row_start - num_cols;
					outboard[prev_row_start + j]--;
					outboard[prev_row_start + jwest]--;
					outboard[prev_row_start + jeast]--;

					// handle south neighbors
					int next_thread_lock_start_index = 0;
					if (likely(i < (num_rows - 1))) {
						next_row_start = row_start + num_cols;
						next_thread_lock_start_index = lock_start_index + num_cols + num_cols;
					}
					else if (unlikely(i == (num_rows - 1))) {
						next_row_start = 0;
						next_thread_lock_start_index = 0;
					}
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + j]);
					outboard[next_row_start + j]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + j]);

					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + jwest]);
					outboard[next_row_start + jwest]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + jwest]);
					
					pthread_mutex_lock(&per_thread_element_locks[next_thread_lock_start_index + jeast]);
					outboard[next_row_start + jeast]--;
					pthread_mutex_unlock(&per_thread_element_locks[next_thread_lock_start_index + jeast]);
				}
			}
			else if (unlikely(IS_ALIVE(cell) && (NUM_NEIGHBOURS(cell) == 2 || NUM_NEIGHBOURS(cell) == 3))) {
				// fprintf(stdout, "cell: %d\n", outboard[row_start + j]);
				// fflush(stdout);
				if (likely(i > start_row && i < (end_row - 1))) {
					outboard[row_start + j] += cell;
					continue;
				}
				if (unlikely(i == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);
					continue;
				}
				else if (unlikely(i == end_row - 1)) {
					int lock_index = lock_start_index + get_bottom_row_lock_index(row_start + j - my_start_index, num_rows_to_handle, num_cols);
					pthread_mutex_lock(&per_thread_element_locks[lock_index]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_index]);
					continue;
				}
			}
			
		}
		// char *temp = inboard;
		// inboard = outboard;
		// outboard = temp;
		// SWAP_BOARDS(inboard, outboard);
		// increment number of threads done.
		// signal main thread to check if all threads are done.
		pthread_mutex_lock(&thread_done_mutex);
		threads_done++;
		pthread_mutex_lock(&thread_go_mutex);
		if (threads_done == NUM_THREADS) {
			pthread_cond_broadcast(&thread_done_cond);
		}
		pthread_mutex_unlock(&thread_done_mutex);
		// wait till main thread finishes and signals a go.
		pthread_cond_wait(&thread_go_cond, &thread_go_mutex);
		pthread_mutex_unlock(&thread_go_mutex);
	}
	for (i = start_row; i < end_row; i++) {
		row_start = i * num_cols;
		for (j = 0; j < num_cols; j++) {
			out_board[row_start + j] = ((out_board[row_start + j] & WAS_ALIVE_MASK) >> 4) + '0';
		}
	}
	free(change_list);
	return NULL;
}

char *game_of_life(char* outboard, char* inboard, const int nrows, const int ncols, const int gens_max) {
	int i;
	in_board = inboard;
	out_board = outboard;
	num_cols = ncols;
	num_rows = nrows;
	const int area = nrows * ncols;
	num_gens = gens_max;
	gen = 0;
	char *temp;

	pthread_mutex_init(&thread_done_mutex, NULL);
	pthread_cond_init(&thread_done_cond, NULL);
	pthread_mutex_init(&thread_go_mutex, NULL);
	pthread_cond_init(&thread_go_cond, NULL);

	int total_num_locks = NUM_THREADS * 2 * ncols;
	per_thread_element_locks = malloc(sizeof(pthread_mutex_t) * total_num_locks);

	for (i = 0; i < total_num_locks; i++) {
		pthread_mutex_init(&per_thread_element_locks[i], NULL);
	}
	// fprintf(stdout, "Finished lock init.\n");
	// fflush(stdout);
	pthread_t threads[NUM_THREADS];
	cpu_set_t cpusets[NUM_THREADS];
	pthread_mutex_lock(&thread_done_mutex);
	for (int i = 0; i < NUM_THREADS; i++) {
		CPU_ZERO(&cpusets[i]);
		CPU_SET(i, &cpusets[i]);
		
		int *id = malloc(sizeof(int));
		*id = i;
		pthread_create(&threads[i], NULL, &game_of_life_thread, id);
		pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpusets[i]);
	}
	while (true) {
		while (threads_done != NUM_THREADS) {
			pthread_cond_wait(&thread_done_cond, &thread_done_mutex);
		}
		gen++;
		// fprintf(stdout, "GEN: %d\n", gen);
		if (unlikely(gen == gens_max)) {
			pthread_mutex_lock(&thread_go_mutex);
			pthread_cond_broadcast(&thread_go_cond);
			pthread_mutex_unlock(&thread_go_mutex);
			for (int i = 0; i < NUM_THREADS; i++) {
				pthread_join(threads[i], NULL);
			}
			free(per_thread_element_locks);
			return out_board;
		}
		threads_done = 0;
		temp = out_board;
		out_board = in_board;
		in_board = temp;
		memset(out_board, 0, area);
		// fprintf(stdout, "OUT BOARD MAIN: 0x%x", out_board);
		pthread_mutex_lock(&thread_go_mutex);
		pthread_cond_broadcast(&thread_go_cond);
		pthread_mutex_unlock(&thread_go_mutex);
	}
	// shouldn't reach here
	return NULL;
}
