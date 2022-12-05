#include "load.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BOARD( __board, __i, __j )  (__board[(__i) * LDA + (__j)])

char* make_board (const int nrows, const int ncols)
{
  char* board = NULL;

  unsigned int area = nrows * ncols;
  board = calloc(area, 1);
  assert(board != NULL);
  return board;
}

// TODO: may parallelize this big loop or tilize.
static char* load_board_values(FILE* input, const int nrows, const int ncols)
{
  char* board = NULL;
  int i = 0;
  int j = 0;
  char c = 'a';
  unsigned int area = nrows * ncols;
  int row_start;
  int prev_row_start;
  int next_row_start;
  /* Make a new board */
  board = calloc(sizeof(char), area);
  /* Fill in the board with values from the input file */
  for (; i < nrows; i++) {
    row_start = i * ncols;
    for (j = 0; j < ncols; j++) {
      fscanf(input, "%c\n", &c);
      // fprintf(stdout, "%c\n", c);
      /* ASCII '0' is not zero; do the conversion */
      // assert(val == 0x10 || val == 0);
      board[row_start + j] += ((c - '0') << 4);
      if (!IS_ALIVE(board[row_start + j])) {
        continue;
      }
      // increment the neighbour count of the cells neighbors
      const int inorth = mod(i-1, nrows);
      const int isouth = mod(i+1, nrows);
      const int jwest = mod(j-1, ncols);
      // printf("%d\n", jwest);
      const int jeast = mod(j+1, ncols);
      board[row_start + jwest]++;
      board[row_start + jeast]++;
      prev_row_start = inorth * ncols;
      board[prev_row_start + jwest]++;
      board[prev_row_start + j]++;
      board[prev_row_start + jeast]++;
      next_row_start = isouth * ncols;
      board[next_row_start + jwest]++;
      board[next_row_start + j]++;
      board[next_row_start + jeast]++;
    }
  }

  // print_board(board, nrows, ncols);
  return board;
}

// load the board for the input file.
char* load_board(FILE* input, int* nrows, int* ncols) {
  // load the dimensions of the board from the input file.
  fscanf(input, "P1\n%d %d\n", nrows, ncols);
  // exit gracefully if nrows and ncols > 10000
  if (*nrows > 10000 || *ncols > 10000) {
    fprintf(stdout, "World size larger than 10000x10000, can't handle this, killing myself...\n");
    exit(0);
  }
  // load the values of the board into 
  return load_board_values(input, *nrows, *ncols);
}


