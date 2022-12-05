// rules: Live cell has 2-3 neighbors ? live : die
//        Dead cell has 3 neighbors ? live : die
char* custom_game_of_life(char* outboard, 
        char* inboard,
        const int nrows,
        const int ncols,
        const int gens_max) {
    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */
    // const int LDA = nrows;
    int curgen, i, j;
	const int area = nrows * ncols;
	unsigned int prev_row_start;
	unsigned int row_start;
	unsigned int next_row_start;
	char cell;
    for (curgen = 0; curgen < gens_max; curgen++)
    {
		memset(outboard, 0, area);
		// print_board(outboard, nrows, ncols);
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
        for (i = 0; i < nrows; i++)
        {
			row_start = i * ncols;
            for (j = 0; j < ncols; j++)
            {
				cell = inboard[row_start + j];
				if (cell == 0) {
					continue;
				}
				else if (!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 3) {
					outboard[row_start + j] += cell;
					continue;
				}
				else if (!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) == 3) {
					// this cell can live now
					// it's neighbors in the out board now have 1 extra neighbor
					cell += 0x10;
					outboard[row_start + j] += cell;
					const int inorth = mod(i-1, nrows);
					const int isouth = mod(i+1, nrows);
					const int jwest = mod(j-1, ncols);
					const int jeast = mod(j+1, ncols);
					outboard[row_start + jwest]++;
					outboard[row_start + jeast]++;
					prev_row_start = inorth * ncols;
					outboard[prev_row_start + jwest]++;
					outboard[prev_row_start + j]++;
					outboard[prev_row_start + jeast]++;
					next_row_start = isouth * ncols;
					outboard[next_row_start + jwest]++;
					outboard[next_row_start + j]++;
					outboard[next_row_start + jeast]++;
					continue;
				}
				else if (IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 2 && NUM_NEIGHBOURS(cell) != 3) {
					cell -= 0x10;
					outboard[row_start + j] += cell;
					const int inorth = mod(i-1, nrows);
					const int isouth = mod(i+1, nrows);
					const int jwest = mod(j-1, ncols);
					const int jeast = mod(j+1, ncols);
					outboard[row_start + jwest]--;
					outboard[row_start + jeast]--;
					prev_row_start = inorth * ncols;
					outboard[prev_row_start + jwest]--;
					outboard[prev_row_start + j]--;
					outboard[prev_row_start + jeast]--;
					next_row_start = isouth * ncols;
					outboard[next_row_start + jwest]--;
					outboard[next_row_start + j]--;
					outboard[next_row_start + jeast]--;
					continue;
				}
				else if (IS_ALIVE(cell) && (NUM_NEIGHBOURS(cell) == 2 || NUM_NEIGHBOURS(cell) == 3)) {
					outboard[row_start + j] += cell;
					continue;
				}
            }
        }
		// print_board(outboard, nrows, ncols);
        SWAP_BOARDS(outboard, inboard);
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
	for (i = 0; i < area; i++) {
		inboard[i] >>= 4;
		// fprintf(stdout, "%d\n", inboard[i]);
	}
    return inboard;
}

static inline int get_bottom_row_lock_index(int element_id, int thread_num_rows, int num_cols) {
	return num_cols + (element_id - (thread_num_rows - 1) * num_cols);
}

char*
game_of_life(char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max) {
	out_board = outboard;
	in_board = inboard;
	num_rows = nrows;
	num_cols = ncols;
	num_gens = gens_max;
	change_list = malloc(sizeof(unsigned int) * nrows * ncols);
	memset(change_list, 0, sizeof(unsigned int) * nrows * ncols);
	return custom_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}


			if (unlikely(cell == 0)) {

				continue;
			}
			else if (likely(!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 3)) {
				if (likely(i > start_row && i < (end_row - 1))) {
					outboard[row_start + j] += cell;
					continue;
				}
				else if (unlikely(i == start_row)) {
					pthread_mutex_lock(&per_thread_element_locks[lock_start_index + j]);
					outboard[row_start + j] += cell;
					pthread_mutex_unlock(&per_thread_element_locks[lock_start_index + j]);
					continue;
				}
				else if (unlikely(i == (end_row - 1))) {
					int lock_index = lock_start_index + get_bottom_row_lock_index(row_start + j - my_start_index, num_rows_to_handle, num_cols);
					// fprintf(stdout, "lock_index: %d\n", lock_index);
					// fflush(stdout);
					pthread_mutex_lock(&per_thread_element_locks[lock_index]); 
					outboard[row_start + j] += cell;
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