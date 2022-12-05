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

char *game_of_life(char* inboard, const int nrows, const int ncols, const int gens_max) {
	int i, j, prev_row_start, row_start, next_row_start, curgen, row, northrow, southrow, col, westcol, eastcol;
	int change_list_index = 0;
	int change_list2_index = 0;
	int *change_list = malloc(sizeof(int) * nrows * ncols);
	int *change_list2 = malloc(sizeof(int) * nrows * ncols);
	char wcell, cell, ecell, nwcell, ncell, necell, swcell, scell, secell;
	for (i = 0; i < nrows; i++) {
		row_start = i * ncols;
		for (j = 0; j < ncols; j++) {
			if (!IS_ALIVE(inboard[row_start + j])) {
				continue;
			}
			change_list[change_list_index++] = SET_ROW_COL(i, j);
		}
	}

	for (curgen = 0; curgen < gens_max; curgen++) {
		for (i = 0; i < change_list_index; i++) {
			row = GET_ROW(change_list[i]);
			col = GET_COL(change_list[i]);
			// fprintf(stdout, "row %d... col %d....\n", row, col);
			// fflush(stdout);
			westcol = mod(col-1, ncols);
			eastcol = mod(col+1, ncols);
			northrow = mod(row-1, nrows);
			southrow = mod(row+1, nrows);
			row_start = row * ncols;
			prev_row_start = northrow * ncols;
			next_row_start = southrow * ncols;
			wcell = inboard[row_start + westcol];
			cell = inboard[row_start + col];
			ecell = inboard[row_start + eastcol];
			nwcell = inboard[prev_row_start + westcol];
			ncell = inboard[prev_row_start + col];
			necell = inboard[prev_row_start + eastcol];
			swcell = inboard[next_row_start + westcol];
			scell = inboard[next_row_start + col];
			secell = inboard[next_row_start + eastcol];
			if (SHOULD_CHANGE_STATE(wcell)) {
				inboard[row_start + westcol] = CHANGE_STATE(wcell);
				change_list2[change_list2_index++] = SET_ROW_COL(row, westcol);
			}
			if (SHOULD_CHANGE_STATE(cell)) {
				inboard[row_start + col] = CHANGE_STATE(cell);
				change_list2[change_list2_index++] = SET_ROW_COL(row, col);
			}
			if (SHOULD_CHANGE_STATE(ecell)) {
				inboard[row_start + eastcol] = CHANGE_STATE(ecell);
				change_list2[change_list2_index++] = SET_ROW_COL(row, eastcol);
			}
			if (SHOULD_CHANGE_STATE(nwcell)) {
				inboard[prev_row_start + westcol] = CHANGE_STATE(nwcell);
				change_list2[change_list2_index++] = SET_ROW_COL(northrow, westcol);
			}
			if (SHOULD_CHANGE_STATE(ncell)) {
				inboard[prev_row_start + col] = CHANGE_STATE(ncell);
				change_list2[change_list2_index++] = SET_ROW_COL(northrow, col);
			}
			if (SHOULD_CHANGE_STATE(necell)) {
				inboard[prev_row_start + eastcol] = CHANGE_STATE(necell);
				change_list2[change_list2_index++] = SET_ROW_COL(northrow, eastcol);
			}
			if (SHOULD_CHANGE_STATE(swcell)) {
				inboard[next_row_start + westcol] = CHANGE_STATE(swcell);
				change_list2[change_list2_index++] = SET_ROW_COL(southrow, westcol);
			}
			if (SHOULD_CHANGE_STATE(scell)) {
				inboard[next_row_start + col] = CHANGE_STATE(scell);
				change_list2[change_list2_index++] = SET_ROW_COL(southrow, col);
			}
			if (SHOULD_CHANGE_STATE(secell)) {
				inboard[next_row_start + eastcol] = CHANGE_STATE(ecell);
				change_list2[change_list2_index++] = SET_ROW_COL(southrow, eastcol);
			}
		}

		for (i = 0; i < change_list2_index; i++) {
			row = GET_ROW(change_list2[i]);
			col = GET_COL(change_list2[i]);
			// fprintf(stdout, "row %d... col %d....\n", row, col);
			// fflush(stdout);
			westcol = mod(col-1, ncols);
			eastcol = mod(col+1, ncols);
			northrow = mod(row-1, nrows);
			southrow = mod(row+1, nrows);
			row_start = row * ncols;
			prev_row_start = northrow * ncols;
			next_row_start = southrow * ncols;
			cell = inboard[row_start + col];
			if (IS_ALIVE(cell)) {
				inboard[row_start + westcol]++;
				inboard[row_start + eastcol]++;
				inboard[prev_row_start + westcol]++;
				inboard[prev_row_start + col]++;
				inboard[prev_row_start + eastcol]++;
				inboard[next_row_start + westcol]++;
				inboard[next_row_start + col]++;
				inboard[next_row_start + eastcol]++;
			}
			else if (!IS_ALIVE(cell)) {
				inboard[row_start + westcol]--;
				inboard[row_start + eastcol]--;
				inboard[prev_row_start + westcol]--;
				inboard[prev_row_start + col]--;
				inboard[prev_row_start + eastcol]--;
				inboard[next_row_start + westcol]--;
				inboard[next_row_start + col]--;
				inboard[next_row_start + eastcol]--;
			}
		}
		int *temp = change_list;
		change_list = change_list2;
		change_list2 = temp;
		change_list_index = change_list2_index;
		change_list2_index = 0;
	}
	for (i = 0; i < nrows; i++) {
		row_start = i * ncols;
		for (j = 0; j < ncols; j++) { 
			inboard[row_start + j] = (inboard[row_start + j] >> 4) + '0';
		}
	}
	// printf("%d\n", sizeof(p
	return inboard;
}
