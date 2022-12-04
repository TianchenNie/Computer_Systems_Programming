#ifndef _util_h
#define _util_h

#include <stdio.h>
#include <assert.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
/**
 * 8 bits per cell
 * |2 bit unused|1 bit old_state (alive or dead)|1 bit current_state (alive or dead)| 4 bits urrent_num_neighbours
 * Every change list val is an integer 32 bits, the row info is stored in the top 18 bits, column info bottom 14 bits.
**/
#define WAS_ALIVE(cell) ((cell) & 0x20)
#define IS_ALIVE(cell) ((cell) & 0x10)
#define NUM_NEIGHBOURS(cell) ((cell) & 0xF)
#define OLD_STATE_MASK (0x1F)
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
  return x >= 0 ? (x % m) : (x + m);
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

static inline char handle_update_neighbor(char *board, char *change_list, const unsigned int change_list_index, const int i, const int j, const int num_cols) {
  const unsigned int row_start = i * num_cols;
  const char cell = board[row_start + j];
  if (IS_ALIVE(cell)) {
    // if the cell is alive and should stay alive
    if (NUM_NEIGHBOURS(cell) == 2 || NUM_NEIGHBOURS(cell) == 3) {
      return 0;
    }
    // if the cell is alive and should die
    else {
      board[row_start + j] -= 0x10;
      change_list[change_list_index] = SET_ROW_COL(i, j);
      return 1;
    }
  }
  else if (!IS_ALIVE(cell)) {
    // if the cell is dead and should stay dead, do nothing
    if (likely(NUM_NEIGHBOURS(cell)) != 3) {
      return 0;
    }
    // if the cell is dead and should live, update and push to change list
    else if (unlikely(NUM_NEIGHBOURS(cell) == 3)) {
      board[row_start + j] += 0x10;
      change_list[change_list_index] = SET_ROW_COL(i, j);
      return 1;
    }
  }
  return 0;
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
