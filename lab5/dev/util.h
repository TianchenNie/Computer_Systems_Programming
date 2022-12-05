#ifndef _util_h
#define _util_h

#include <stdio.h>
#include <assert.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
/**
 * 8 bits per cell
 * |3 bit unused |1 bit current_state (alive or dead) | 4 bits current_num_neighbours
 * Every change list val is an integer 32 bits, the row info is stored in the top 18 bits, column info bottom 14 bits.
**/
#define IS_ALIVE(cell) ((cell) & 0x10)

// alive: 2-3 neighbors stay alive, else die
// dead: 3 neighbors and live, else dead
#define SHOULD_CHANGE_STATE(cell) ((IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) != 2 && NUM_NEIGHBOURS(cell) != 3) || (!IS_ALIVE(cell) && NUM_NEIGHBOURS(cell) == 3))

#define CHANGE_STATE(cell) ((cell) ^ 0x10)
#define NUM_NEIGHBOURS(cell) ((cell) & 0xF)
#define GET_ROW(val) ((val) >> 14)
#define GET_COL(val) ((val) & 0x3FFF)
#define SET_ROW_COL(row, col) (((row) << 14) + (col))


/**
 * C's mod ('%') operator is mathematically correct, but it may return
 * a negative remainder even when both inputs are nonnegative.  This
 * function always returns a nonnegative remainder (x mod m), as long
 * as x and m are both positive.  This is helpful for computing
 * toroidal boundary conditions.
 */
static inline int mod(const int x, const int m) {
  return x >= 0 ? (x % m) : (x % m + m);
}

// static inline int 
// mod(const int x, const int m) {
//   if (likely(x >= 0) && x != m) {
//     return x;
//   } 
//   else if (unlikely(x < 0)) {
//     return x + m;
//   }
//   else if (unlikely(x == m)) {
//     return 0;
//   }
// }

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
    assert((board[i] - '0') >= 0 && (board[i] - '0') <= 24);
    // fprintf(stdout, "Board value: %d\t", board[i]);
    // fflush(stdout);
    
  }
}

#endif /* _util_h */
