/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

// contains indices of cells that have been change in prev iteration.
unsigned int* change_list;

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
					outboard[row_start + j] += cell;
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

char*
game_of_life(char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max) {
	change_list = malloc(sizeof(unsigned int) * nrows * ncols);
	memset(change_list, 0, sizeof(unsigned int) * nrows * ncols);
	return custom_game_of_life(outboard, inboard, nrows, ncols, gens_max);
}
