#ifndef _util_h
#define _util_h

#include <stdio.h>
#include <assert.h>
// cell: xxx alive=1,dead=0 num_neighbors
#define NUM_NEIGHBOURS(cell) ((cell) & 0xF)
#define IS_ALIVE(cell) ((cell) & 0x10)
/**
 * C's mod ('%') operator is mathematically correct, but it may return
 * a negative remainder even when both inputs are nonnegative.  This
 * function always returns a nonnegative remainder (x mod m), as long
 * as x and m are both positive.  This is helpful for computing
 * toroidal boundary conditions.
 */
static inline int 
mod(const int x, const int m) {
  return (x >= 0) ? (x % m) : ((x % m) + m);
}


/**
 * Given neighbor count and current state, return zero if cell will be
 * dead, or nonzero if cell will be alive, in the next round.
 */
static inline char alivep(char count, char cell) {
  // If cell is dead but has exactly 3 neighbors, it rises from the dead
  // if cell is alive but has less than 2 neighbors or more than 3 neighbors, it dies
  return (!cell && (count == (char) 3)) || (cell && (count >= 2) && (count <= 3));
}

static inline void print_board(char *board, int nrows, int ncols) {
  int area = nrows * ncols;
  int i = 0;
  for(; i < area; i++) {
    fprintf(stdout, "Board value: %d\t", board[i]);
    fflush(stdout);
    assert(board[i] >= 0 && board[i] <= 24);
    
  }
}

#endif /* _util_h */
