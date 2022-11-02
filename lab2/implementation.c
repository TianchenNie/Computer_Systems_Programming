#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "utilities.h"  // DO NOT REMOVE this line
#include "implementation_reference.h"   // DO NOT REMOVE this line

#pragma GCC target ("avx,avx2")
#pragma __attribute__((always_inline))

#define min(_a,_b) (((_a) < (_b)) ? (_a) : (_b))
#define max(_a,_b) (((_a) > (_b)) ? (_a) : (_b))
#define abs(_a) (((_a) >= 0) ? (_a) : (-1*(_a)))
#define abcd 1
#define acbd 2
#define badc 3
#define cadb 4
#define dbca 5
#define dcba 6
#define cdab 7
#define bdac 8


static unsigned int g_width = 0;
static unsigned int g_actual_width = 0;


// TODO: find a way to accumulate rotates, shifts, and mirrors. Currently the problem
// is that rotate then mirror != mirror then rotate && shift then mirror != mirror then shift
// TODO: cache the top left bottom right bounds of image of interest, right now rotating/looping through
// background which is unnecessary
// TODO: find a way to do loop unrolling

/* [row, col] */
/* used by abcd, cdab */
static unsigned char *initial_buffer_abcd;

/* used by acbd, bdac*/
static unsigned char *initial_buffer_acbd;

/* used by badc, dcba*/
static unsigned char *initial_buffer_badc;

/*used by cadb, dbca*/
static unsigned char *initial_buffer_cadb;

static unsigned char *initial_buffer_dbca;

static unsigned char *initial_buffer_dcba;

static unsigned char *initial_buffer_bdac;

static unsigned char *initial_buffer_cdab;

static int top_left_row;
static int top_left_col;
static int bottom_left_row;
static int bottom_left_col;
static int top_right_row;
static int top_right_col;
static int bottom_right_row;
static int bottom_right_col;
/* used for buffers abcd and badc*/
static int initial_top_left_row;
static int initial_top_left_col;
static int initial_bottom_left_row;
static int initial_bottom_left_col;
static int initial_top_right_row;
static int initial_top_right_col;
static int initial_bottom_right_row;
static int initial_bottom_right_col;
/* used for buffers acbd and cadb */
static int initial_top_left_row_rotated;
static int initial_top_left_col_rotated;
static int initial_bottom_left_row_rotated;
static int initial_bottom_left_col_rotated;
static int initial_top_right_row_rotated;
static int initial_top_right_col_rotated;
static int initial_bottom_right_row_rotated;
static int initial_bottom_right_col_rotated;
static int a_row;
static int a_col;
static int b_row;
static int b_col;
static int c_row;
static int c_col;
static int d_row;
static int d_col;
static int found_image;

// void print_bounds() {
//     printf("top_left row col (%d, %d)\n", top_left_row, top_left_col);
//     printf("bottom_left row col (%d, %d)\n", bottom_left_row, bottom_left_col);
//     printf("top_right row col (%d, %d)\n", top_right_row, top_right_col);
//     printf("bottom_right row col (%d, %d)\n", bottom_right_row, bottom_right_col);
// }

// void print_abcd() {
//     printf("a row col (%d, %d)\n", a_row, a_col);
//     printf("b row col (%d, %d)\n", b_row, b_col);
//     printf("c row col (%d, %d)\n", c_row, c_col);
//     printf("d row col (%d, %d)\n", d_row, d_col);
// }

static inline __attribute__((always_inline)) void set_initial_vertices() {
    initial_top_left_row = top_left_row;
    initial_top_left_col = top_left_col;
    initial_bottom_left_row = bottom_left_row;
    initial_bottom_left_col = bottom_left_col;
    initial_top_right_row = top_right_row;
    initial_top_right_col = top_right_col;
    initial_bottom_right_row = bottom_right_row;
    initial_bottom_right_col = bottom_right_col;

    initial_top_left_row_rotated = top_left_col;
    initial_top_left_col_rotated = top_left_row;

    initial_bottom_left_row_rotated = top_right_col;
    initial_bottom_left_col_rotated = top_right_row;

    initial_top_right_row_rotated = bottom_left_col;
    initial_top_right_col_rotated = bottom_left_row;

    initial_bottom_right_row_rotated = bottom_right_col;
    initial_bottom_right_col_rotated = bottom_right_row;
}

void set_initial_buffers() {
    /* set acbd */
    memset(initial_buffer_acbd, 255, g_width * g_actual_width);
    register int i = 0;
    register int j = 0;
    register unsigned int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
    bound_i = bound_i << 2;
    register unsigned int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
    bound_j = bound_j << 2;
    register int initial_pixel_addr0;
    register int initial_pixel_addr1;
    register int initial_pixel_addr2;
    register int initial_pixel_addr3;
    register int dest_pixel_addr0;
    register int dest_pixel_addr1;
    register int dest_pixel_addr2;
    register int dest_pixel_addr3;
    register int initial_offset;
    register int dest_offset;
    for (; i < bound_i; i += 4) {
        initial_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        initial_pixel_addr1 = (initial_top_left_row + i + 1) * g_actual_width;
        initial_pixel_addr2 = (initial_top_left_row + i + 2) * g_actual_width;
        initial_pixel_addr3 = (initial_top_left_row + i + 3) * g_actual_width;
        dest_pixel_addr0 = (initial_top_left_col_rotated + i) * 3;
        dest_pixel_addr1 = (initial_top_left_col_rotated + i + 1) * 3;
        dest_pixel_addr2 = (initial_top_left_col_rotated + i + 2) * 3;
        dest_pixel_addr3 = (initial_top_left_col_rotated + i + 3) * 3;
        initial_offset = initial_top_left_col * 3 - 3;
        dest_offset = initial_top_left_row_rotated * g_actual_width - g_actual_width;
        for (j = 0; j <= bound_j; j += 4) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_left_row + j) * g_actual_width;
            initial_offset += 3;
            dest_offset += g_actual_width;
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 1
            // initial_offset = (initial_top_left_col + j + 1) * 3;
            // dest_offset = (top_left_row + j + 1) * g_actual_width;
            initial_offset += 3;
            dest_offset += g_actual_width;
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 2
            // initial_offset = (initial_top_left_col + j + 2) * 3;
            // dest_offset = (top_left_row + j + 2) * g_actual_width;
            initial_offset += 3;
            dest_offset += g_actual_width;
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 3
            // initial_offset = (initial_top_left_col + j + 3) * 3;
            initial_offset += 3;
            dest_offset += g_actual_width;
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);
        }
        for (; j <= initial_top_right_col - initial_top_left_col; j++) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_left_row + j) * g_actual_width;
            initial_offset += 3;
            dest_offset += g_actual_width;
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_acbd[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);
        }
    }

    for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
        initial_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        dest_pixel_addr0 = (initial_top_left_col_rotated + i) * 3;
        for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
            memcpy(&initial_buffer_acbd[dest_pixel_addr0 + (initial_top_left_row_rotated + j) * g_actual_width], &initial_buffer_abcd[initial_pixel_addr0 + (initial_top_left_col + j) * 3], 3);
        }
    }
    /* set badc */
    // printf("BADC!!!!");
    // TODO: update copy row to row
    memset(initial_buffer_badc, 255, g_width * g_actual_width);
    i = 0;
    j = 0;
    for (; i < bound_i; i += 4) {
        initial_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        initial_pixel_addr1 = (initial_top_left_row + i + 1) * g_actual_width;
        initial_pixel_addr2 = (initial_top_left_row + i + 2) * g_actual_width;
        initial_pixel_addr3 = (initial_top_left_row + i + 3) * g_actual_width;
        dest_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        dest_pixel_addr1 = (initial_top_left_row + i + 1) * g_actual_width;
        dest_pixel_addr2 = (initial_top_left_row + i + 2) * g_actual_width;
        dest_pixel_addr3 = (initial_top_left_row + i + 3) * g_actual_width;
        initial_offset = initial_top_left_col * 3 - 3;
        dest_offset = initial_top_right_col * 3 + 3;
        for (j = 0; j < bound_j; j += 4) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_right_col - j) * 3;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 1
            // initial_offset = (initial_top_left_col + j + 1) * 3;
            // dest_offset = (top_right_col - (j + 1)) * 3;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 2
            // initial_offset = (initial_top_left_col + j + 2) * 3;
            // dest_offset = (top_right_col - (j + 2)) * 3;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 3
            // initial_offset = (initial_top_left_col + j + 3) * 3;
            // dest_offset = (top_right_col - (j + 3)) * 3;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);
        }
        for (; j <= initial_top_right_col - initial_top_left_col; j++) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_right_col - j) * 3;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + dest_offset], &initial_buffer_abcd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr1 + dest_offset], &initial_buffer_abcd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr2 + dest_offset], &initial_buffer_abcd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_badc[dest_pixel_addr3 + dest_offset], &initial_buffer_abcd[initial_pixel_addr3 + initial_offset], 3);
        }
    }
    for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
        initial_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        dest_pixel_addr0 = (initial_top_left_row + i) * g_actual_width;
        for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
            memcpy(&initial_buffer_badc[dest_pixel_addr0 + (initial_top_right_col - j) * 3], &initial_buffer_abcd[initial_pixel_addr0 + (initial_top_left_col + j) * 3], 3);
        }
    }
    /* set cadb */
    memset(initial_buffer_cadb, 255, g_width * g_actual_width);
    i = 0;
    j = 0;
    bound_i = (initial_bottom_left_row_rotated - initial_top_left_row_rotated) >> 2;
    bound_i = bound_i << 2;
    bound_j = (initial_top_right_col_rotated - initial_top_left_col_rotated) >> 2;
    bound_j = bound_j << 2;
    for (; i < bound_i; i += 4) {
        initial_pixel_addr0 = (initial_top_left_row_rotated + i) * g_actual_width;
        initial_pixel_addr1 = (initial_top_left_row_rotated + i + 1) * g_actual_width;
        initial_pixel_addr2 = (initial_top_left_row_rotated + i + 2) * g_actual_width;
        initial_pixel_addr3 = (initial_top_left_row_rotated + i + 3) * g_actual_width;
        dest_pixel_addr0 = initial_pixel_addr0;
        dest_pixel_addr1 = initial_pixel_addr1;
        dest_pixel_addr2 = initial_pixel_addr2;
        dest_pixel_addr3 = initial_pixel_addr3;
        initial_offset = initial_top_left_col_rotated * 3 - 3;
        dest_offset = initial_top_right_col_rotated * 3 + 3;
        for (j = 0; j < bound_j; j += 4) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_left_row + j) * g_actual_width;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + dest_offset], &initial_buffer_acbd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr1 + dest_offset], &initial_buffer_acbd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr2 + dest_offset], &initial_buffer_acbd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr3 + dest_offset], &initial_buffer_acbd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 1
            // initial_offset = (initial_top_left_col + j + 1) * 3;
            // dest_offset = (top_left_row + j + 1) * g_actual_width;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + dest_offset], &initial_buffer_acbd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr1 + dest_offset], &initial_buffer_acbd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr2 + dest_offset], &initial_buffer_acbd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr3 + dest_offset], &initial_buffer_acbd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 2
            // initial_offset = (initial_top_left_col + j + 2) * 3;
            // dest_offset = (top_left_row + j + 2) * g_actual_width;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + dest_offset], &initial_buffer_acbd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr1 + dest_offset], &initial_buffer_acbd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr2 + dest_offset], &initial_buffer_acbd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr3 + dest_offset], &initial_buffer_acbd[initial_pixel_addr3 + initial_offset], 3);

            // unroll j 3
            // initial_offset = (initial_top_left_col + j + 3) * 3;
            // dest_offset = (top_left_row + j + 3) * g_actual_width;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + dest_offset], &initial_buffer_acbd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr1 + dest_offset], &initial_buffer_acbd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr2 + dest_offset], &initial_buffer_acbd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr3 + dest_offset], &initial_buffer_acbd[initial_pixel_addr3 + initial_offset], 3);
        }
        for (; j <= initial_top_right_col_rotated - initial_top_left_col_rotated; j++) {
            // initial_offset = (initial_top_left_col + j) * 3;
            // dest_offset = (top_left_row + j) * g_actual_width;
            initial_offset += 3;
            dest_offset -= 3;
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + dest_offset], &initial_buffer_acbd[initial_pixel_addr0 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr1 + dest_offset], &initial_buffer_acbd[initial_pixel_addr1 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr2 + dest_offset], &initial_buffer_acbd[initial_pixel_addr2 + initial_offset], 3);
            memcpy(&initial_buffer_cadb[dest_pixel_addr3 + dest_offset], &initial_buffer_acbd[initial_pixel_addr3 + initial_offset], 3);
        }
    }
    for (; i <= initial_bottom_left_row_rotated - initial_top_left_row_rotated; i++) {
        initial_pixel_addr0 = (initial_top_left_row_rotated + i) * g_actual_width;
        dest_pixel_addr0 = initial_pixel_addr0;
        for (j = 0; j <= initial_top_right_col_rotated - initial_top_left_col_rotated; j++) {
            memcpy(&initial_buffer_cadb[dest_pixel_addr0 + (initial_top_right_col_rotated - j) * 3], &initial_buffer_acbd[initial_pixel_addr0 + (initial_top_left_col_rotated + j) * 3], 3);
        }
    }
    /* set dbca */
    memset(initial_buffer_dbca, 255, g_width * g_actual_width);
    i = 0;
    bound_i = (initial_bottom_left_row_rotated - initial_top_left_row_rotated) >> 2;
    bound_i = bound_i << 2;
    register int width = (initial_top_right_col_rotated + 1) * 3 - initial_top_left_col_rotated * 3;
    register int dest = initial_bottom_left_row_rotated * g_actual_width + g_actual_width + initial_bottom_left_col_rotated * 3;
    register int initial = initial_top_left_row_rotated * g_actual_width - g_actual_width + initial_top_left_col_rotated * 3;
    for (; i < bound_i; i += 4) {
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dbca[dest], &initial_buffer_cadb[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dbca[dest], &initial_buffer_cadb[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dbca[dest], &initial_buffer_cadb[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dbca[dest], &initial_buffer_cadb[initial], width);
    }
    for (; i <= initial_bottom_left_row_rotated - initial_top_left_row_rotated; i++) {
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dbca[dest], &initial_buffer_cadb[initial], width);
    }
    /* set dcba */
    memset(initial_buffer_dcba, 255, g_width * g_actual_width);
    i = 0;
    bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
    bound_i = bound_i << 2;
    width = (initial_top_right_col + 1) * 3 - initial_top_left_col * 3;
    dest = initial_bottom_left_row * g_actual_width + g_actual_width + initial_bottom_left_col * 3;
    initial = initial_top_left_row * g_actual_width - g_actual_width + initial_top_left_col * 3;
    for (; i < bound_i; i += 4) {
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dcba[dest], &initial_buffer_badc[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dcba[dest], &initial_buffer_badc[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dcba[dest], &initial_buffer_badc[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dcba[dest], &initial_buffer_badc[initial], width);
    }
    for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_dcba[dest], &initial_buffer_badc[initial], width);
    }
    /* set cdab */
    memset(initial_buffer_cdab, 255, g_width * g_actual_width);
    i = 0;
    bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
    bound_i = bound_i << 2;
    width = (initial_top_right_col + 1) * 3 - initial_top_left_col * 3;
    dest = initial_bottom_left_row * g_actual_width + g_actual_width + initial_bottom_left_col * 3;
    initial = initial_top_left_row * g_actual_width - g_actual_width + initial_top_left_col * 3;
    for (; i < bound_i; i += 4) {
        dest -= g_actual_width;
        initial += g_actual_width;
        // printf("i: %d\n", i);
        memcpy(&initial_buffer_cdab[dest], &initial_buffer_abcd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_cdab[dest], &initial_buffer_abcd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_cdab[dest], &initial_buffer_abcd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_cdab[dest], &initial_buffer_abcd[initial], width);
    }
    for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
        dest -= g_actual_width;
        initial += g_actual_width;
        // printf("NEW LOOP i: %d\n", i);
        memcpy(&initial_buffer_cdab[dest], &initial_buffer_abcd[initial], width);
    }
    /* set bdac */
    memset(initial_buffer_bdac, 255, g_width * g_actual_width);
    i = 0;
    bound_i = (initial_top_right_row_rotated - initial_top_left_row_rotated) >> 2;
    bound_i = bound_i << 2;
    width = (initial_top_right_col_rotated + 1) * 3 - initial_top_left_col_rotated * 3;
    dest = initial_bottom_left_row_rotated * g_actual_width + g_actual_width + initial_bottom_left_col_rotated * 3;
    initial = initial_top_left_row_rotated * g_actual_width - g_actual_width + initial_top_left_col_rotated * 3;
    for (; i < bound_i; i += 4) {
        dest -= g_actual_width;
        initial += g_actual_width;
        // printf("i: %d\n", i);
        memcpy(&initial_buffer_bdac[dest], &initial_buffer_acbd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_bdac[dest], &initial_buffer_acbd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_bdac[dest], &initial_buffer_acbd[initial], width);
        dest -= g_actual_width;
        initial += g_actual_width;
        memcpy(&initial_buffer_bdac[dest], &initial_buffer_acbd[initial], width);
    }
    for (; i <= initial_bottom_left_row_rotated - initial_top_left_row_rotated; i++) {
        dest -= g_actual_width;
        initial += g_actual_width;
        // printf("NEW LOOP i: %d\n", i);
        memcpy(&initial_buffer_bdac[dest], &initial_buffer_acbd[initial], width);
    }
}
/*
case1
A B
C D
*/
static inline __attribute__((always_inline)) int ABCD() {
    return (a_row == top_left_row && a_col == top_left_col &&
        b_row == top_right_row && b_col == top_right_col && 
        c_row == bottom_left_row && c_col == bottom_left_col && 
        d_row == bottom_right_row && d_col == bottom_right_col) ? abcd : 0;
}

/*
case2
A C
B D
*/
static inline __attribute__((always_inline)) int ACBD() {
    return (a_row == top_left_row && a_col == top_left_col &&
        c_row == top_right_row && c_col == top_right_col && 
        b_row == bottom_left_row && b_col == bottom_left_col && 
        d_row == bottom_right_row && d_col == bottom_right_col) ? acbd : 0;
}

/*
case3
B A
D C
*/
static inline __attribute__((always_inline)) int BADC() {
    return (b_row == top_left_row && b_col == top_left_col &&
        a_row == top_right_row && a_col == top_right_col && 
        d_row == bottom_left_row && d_col == bottom_left_col && 
        c_row == bottom_right_row && c_col == bottom_right_col) ? badc : 0;
}

/*
case4
C A
D B
*/
static inline __attribute__((always_inline)) int CADB() {
    return (c_row == top_left_row && c_col == top_left_col &&
        a_row == top_right_row && a_col == top_right_col && 
        d_row == bottom_left_row && d_col == bottom_left_col && 
        b_row == bottom_right_row && b_col == bottom_right_col) ? cadb : 0;
}

/*
case5
D B
C A
*/
static inline __attribute__((always_inline)) int DBCA() {
    return (d_row == top_left_row && d_col == top_left_col &&
        b_row == top_right_row && b_col == top_right_col && 
        c_row == bottom_left_row && c_col == bottom_left_col && 
        a_row == bottom_right_row && a_col == bottom_right_col) ? dbca : 0;
}

/*
case6
D C
B A
*/
static inline __attribute__((always_inline)) int DCBA() {
    return (d_row == top_left_row && d_col == top_left_col &&
        c_row == top_right_row && c_col == top_right_col && 
        b_row == bottom_left_row && b_col == bottom_left_col && 
        a_row == bottom_right_row && a_col == bottom_right_col) ? dcba : 0;
}

/*
case7
C D
A B
*/
static inline __attribute__((always_inline)) int CDAB() {
    return (c_row == top_left_row && c_col == top_left_col &&
        d_row == top_right_row && d_col == top_right_col && 
        a_row == bottom_left_row && a_col == bottom_left_col && 
        b_row == bottom_right_row && b_col == bottom_right_col) ? cdab : 0;
}

/*
case8
B D
A C
*/
static inline __attribute__((always_inline)) int BDAC() {
    return (b_row == top_left_row && b_col == top_left_col &&
        d_row == top_right_row && d_col == top_right_col && 
        a_row == bottom_left_row && a_col == bottom_left_col && 
        c_row == bottom_right_row && c_col == bottom_right_col) ? bdac : 0;
}

/*
case1
A B
C D
*/
static inline __attribute__((always_inline)) void assign_ABCD() {
    a_row = top_left_row; a_col = top_left_col;
    b_row = top_right_row; b_col = top_right_col;
    c_row = bottom_left_row; c_col = bottom_left_col; 
    d_row = bottom_right_row; d_col = bottom_right_col;
}

/*
case2
A C
B D
*/
static inline __attribute__((always_inline)) void assign_ACBD() {
    a_row = top_left_row; a_col = top_left_col;
    c_row = top_right_row; c_col = top_right_col;
    b_row = bottom_left_row; b_col = bottom_left_col; 
    d_row = bottom_right_row; d_col = bottom_right_col;
}

/*
case3
B A
D C
*/
static inline __attribute__((always_inline)) void assign_BADC() {
    b_row = top_left_row; b_col = top_left_col;
    a_row = top_right_row; a_col = top_right_col;
    d_row = bottom_left_row; d_col = bottom_left_col; 
    c_row = bottom_right_row; c_col = bottom_right_col;
}

/*
case4
C A
D B
*/
static inline __attribute__((always_inline)) void assign_CADB() {
    c_row = top_left_row; c_col = top_left_col;
    a_row = top_right_row; a_col = top_right_col;
    d_row = bottom_left_row; d_col = bottom_left_col; 
    b_row = bottom_right_row; b_col = bottom_right_col;
}

/*
case5
D B
C A
*/
static inline __attribute__((always_inline)) void assign_DBCA() {
    d_row = top_left_row; d_col = top_left_col;
    b_row = top_right_row; b_col = top_right_col;
    c_row = bottom_left_row; c_col = bottom_left_col; 
    a_row = bottom_right_row; a_col = bottom_right_col;
}

/*
case6
D C
B A
*/
static inline __attribute__((always_inline)) void assign_DCBA() {
    d_row = top_left_row; d_col = top_left_col;
    c_row = top_right_row; c_col = top_right_col;
    b_row = bottom_left_row; b_col = bottom_left_col; 
    a_row = bottom_right_row; a_col = bottom_right_col;
}

/*
case7
C D
A B
*/
static inline __attribute__((always_inline)) void assign_CDAB() {
    c_row = top_left_row; c_col = top_left_col;
    d_row = top_right_row; d_col = top_right_col;
    a_row = bottom_left_row; a_col = bottom_left_col; 
    b_row = bottom_right_row; b_col = bottom_right_col;
}

/*
case8
B D
A C
*/
static inline __attribute__((always_inline)) void assign_BDAC() {
    b_row = top_left_row; b_col = top_left_col;
    d_row = top_right_row; d_col = top_right_col;
    a_row = bottom_left_row; a_col = bottom_left_col; 
    c_row = bottom_right_row; c_col = bottom_right_col;
}

void set_image_bounds(unsigned char *buffer_frame) {
    int top_row = -1;
    int left_col = 1e6;
    int bot_row = -1;
    int right_col = -1;
    int found = 0;
    register int row = 0;
    register int col = 0;
    register int cell;
    for (; row < g_width; ++row) {
        cell = row * g_actual_width - 3;
        for (col = 0; col < g_width; ++col) {
            cell += 3;
            if (initial_buffer_abcd[cell] != 255 || initial_buffer_abcd[cell + 1] != 255 || initial_buffer_abcd[cell + 2] != 255) {
                if (__builtin_expect(top_row < 0, 0)) {
                    top_row = row;
                }
                left_col = min(col, left_col);
                bot_row = row;
                right_col = max(col, right_col);
                found = 1;
            }
        }
    }
    if (found) {
        top_left_row = top_row;
        top_left_col = left_col;
        bottom_left_row = bot_row;
        bottom_left_col = left_col;
        top_right_row = top_row;
        top_right_col = right_col;
        bottom_right_row = bot_row;
        bottom_right_col = right_col;
        found_image = 1;
    }
    else if (!found) {
        top_left_row = 0;
        top_left_col = 0;
        bottom_left_row = 0;
        bottom_left_col = 0;
        top_right_row = 0;
        top_right_col = 0;
        bottom_right_row = 0;
        bottom_right_col = 0;
        found_image = 0;
    }
    assign_ABCD();
    set_initial_vertices();
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param offset - number of pixels to shift the object in bitmap image up
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
void processMoveUp(int offset) {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    top_left_row -= offset;
    top_right_row -= offset;
    bottom_left_row -= offset;
    bottom_right_row -= offset;
    if (c == abcd) assign_ABCD();
    else if (c == acbd) assign_ACBD();
    else if (c == badc) assign_BADC();
    else if (c == cadb) assign_CADB();
    else if (c == dbca) assign_DBCA();
    else if (c == dcba) assign_DCBA();
    else if (c == cdab) assign_CDAB();
    else if (c == bdac) assign_BDAC();
    // else printf("CASE NOT FOUND!!!! DEBUG PLEASE!!!\n");
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param offset - number of pixels to shift the object in bitmap image up
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
void processMoveDown(int offset) {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    top_left_row += offset;
    top_right_row += offset;
    bottom_left_row += offset;
    bottom_right_row += offset;
    if (c == abcd) assign_ABCD();
    else if (c == acbd) assign_ACBD();
    else if (c == badc) assign_BADC();
    else if (c == cadb) assign_CADB();
    else if (c == dbca) assign_DBCA();
    else if (c == dcba) assign_DCBA();
    else if (c == cdab) assign_CDAB();
    else if (c == bdac) assign_BDAC();
    // else printf("CASE NOT FOUND!!!! DEBUG PLEASE!!!\n");
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param offset - number of pixels to shift the object in bitmap image right
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
void processMoveLeft(int offset) {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    top_left_col -= offset;
    top_right_col -= offset;
    bottom_left_col -= offset;
    bottom_right_col -= offset;
    if (c == abcd) assign_ABCD();
    else if (c == acbd) assign_ACBD();
    else if (c == badc) assign_BADC();
    else if (c == cadb) assign_CADB();
    else if (c == dbca) assign_DBCA();
    else if (c == dcba) assign_DCBA();
    else if (c == cdab) assign_CDAB();
    else if (c == bdac) assign_BDAC();
    // else printf("CASE NOT FOUND!!!! DEBUG PLEASE!!!\n");
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param offset - number of pixels to shift the object in bitmap image left
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
void processMoveRight(int offset) {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    top_left_col += offset;
    top_right_col += offset;
    bottom_left_col += offset;
    bottom_right_col += offset;
    if (c == abcd) assign_ABCD();
    else if (c == acbd) assign_ACBD();
    else if (c == badc) assign_BADC();
    else if (c == cadb) assign_CADB();
    else if (c == dbca) assign_DBCA();
    else if (c == dcba) assign_DCBA();
    else if (c == cdab) assign_CDAB();
    else if (c == bdac) assign_BDAC();
    // else printf("CASE NOT FOUND!!!! DEBUG PLEASE!!!\n");
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorX() {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    float x_axis = (g_width - 1) * 0.5;
    int temp = top_left_row;
    top_left_row = bottom_left_row + (x_axis - bottom_left_row) * 2;
    top_right_row = top_left_row;
    bottom_left_row = temp + (x_axis - temp) * 2;
    bottom_right_row = bottom_left_row;
    if (c == abcd) assign_CDAB();
    else if (c == acbd) assign_BDAC();
    else if (c == badc) assign_DCBA();
    else if (c == cadb) assign_DBCA();
    else if (c == dbca) assign_CADB();
    else if (c == dcba) assign_BADC();
    else if (c == cdab) assign_ABCD();
    else if (c == bdac) assign_ACBD();
    // else printf("ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorY() {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    float y_axis = (g_width - 1) * 0.5;
    int temp = top_left_col;
    top_left_col = top_right_col + (y_axis - top_right_col) * 2;
    bottom_left_col = top_left_col;
    top_right_col = temp + (y_axis - temp) * 2;
    bottom_right_col = top_right_col;
    if (c == abcd) assign_BADC();
    else if (c == acbd) assign_CADB();
    else if (c == badc) assign_ABCD();
    else if (c == cadb) assign_ACBD();
    else if (c == dbca) assign_BDAC();
    else if (c == dcba) assign_CDAB();
    else if (c == cdab) assign_DCBA();
    else if (c == bdac) assign_DBCA();
    // else printf("ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");
    return;
}

void processRotateCW1() {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    int temp;
    // top_left
    temp = top_left_row;
    top_left_row = top_left_col;
    top_left_col = temp;

    // bottom_right
    temp = bottom_right_row;
    bottom_right_row = bottom_right_col;
    bottom_right_col = temp;

    // top_right
    temp = top_right_row;
    top_right_row = top_right_col;
    top_right_col = temp;

    // bottom_left
    temp = bottom_left_row;
    bottom_left_row = bottom_left_col;
    bottom_left_col = temp;

    // swap top_right bottom_left
    int temp1 = top_right_row;
    int temp2 = top_right_col;
    top_right_row = bottom_left_row;
    top_right_col = bottom_left_col;
    bottom_left_row = temp1;
    bottom_left_col = temp2;

    if (c == abcd) assign_ACBD();
    else if (c == acbd) assign_ABCD();
    else if (c == badc) assign_BDAC();
    else if (c == cadb) assign_CDAB();
    else if (c == dbca) assign_DCBA();
    else if (c == dcba) assign_DBCA();
    else if (c == cdab) assign_CADB();
    else if (c == bdac) assign_BADC();
    // else printf("ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");

    processMirrorY();
    // printBMP(width, height, buffer_frame);
    return;
}

void processRotateCW2() {
    processMirrorX();
    processMirrorY();
    return;
}

void processRotateCW3() {
    int c = 0;
    if (ABCD()) c = abcd;
    else if (ACBD()) c = acbd;
    else if (BADC()) c = badc;
    else if (CADB()) c = cadb;
    else if (DBCA()) c = dbca;
    else if (DCBA()) c = dcba;
    else if (CDAB()) c = cdab;
    else if (BDAC()) c = bdac;
    int temp;
    // top_left
    temp = top_left_row;
    top_left_row = top_left_col;
    top_left_col = temp;

    // bottom_right
    temp = bottom_right_row;
    bottom_right_row = bottom_right_col;
    bottom_right_col = temp;

    // top_right
    temp = top_right_row;
    top_right_row = top_right_col;
    top_right_col = temp;

    // bottom_left
    temp = bottom_left_row;
    bottom_left_row = bottom_left_col;
    bottom_left_col = temp;

    // swap top_right bottom_left
    int temp1 = top_right_row;
    int temp2 = top_right_col;
    top_right_row = bottom_left_row;
    top_right_col = bottom_left_col;
    bottom_left_row = temp1;
    bottom_left_col = temp2;

    if (c == abcd) assign_ACBD();
    else if (c == acbd) assign_ABCD();
    else if (c == badc) assign_BDAC();
    else if (c == cadb) assign_CDAB();
    else if (c == dbca) assign_DCBA();
    else if (c == dcba) assign_DBCA();
    else if (c == cdab) assign_CADB();
    else if (c == bdac) assign_BADC();
    // else printf("ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");

    processMirrorX();
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param rotate_iteration - rotate object inside frame buffer clockwise by 90 degrees, <iteration> times
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
void processRotateCW(int rotate_iteration) {
    if (rotate_iteration == 0 || rotate_iteration == 4) {
        return;
    }
    else if (rotate_iteration == 1) {
        processRotateCW1();
        return;
    }
    else if (rotate_iteration == 2) {
        processRotateCW2();
        return;
    }
    processRotateCW3();
    return;
}

void update_frame_buffer(unsigned char *new_frame) {
    if (ABCD()) {
        // printf("ABCD!!!!!\n");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row * g_actual_width + initial_top_left_col * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row * g_actual_width + (initial_bottom_right_col + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_abcd[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    else if (ACBD()) { // needs testing
        // printf("ACBD!!!!!\n");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row_rotated * g_actual_width + initial_top_left_col_rotated * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row_rotated * g_actual_width + (initial_bottom_right_col_rotated + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_acbd[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    else if (BADC()) {
        // printf("BADC!!!!!\n");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row * g_actual_width + initial_top_left_col * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row * g_actual_width + (initial_bottom_right_col + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_badc[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    else if (CADB()) { // rotate CW1
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row_rotated * g_actual_width + initial_top_left_col_rotated * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row_rotated * g_actual_width + (initial_bottom_right_col_rotated + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_cadb[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    else if (DBCA()) {
        // printf("DBCA!!!!");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row_rotated * g_actual_width + initial_top_left_col_rotated * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row_rotated * g_actual_width + (initial_bottom_right_col_rotated + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_dbca[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    else if (DCBA()) {
        // printf("DCBA!!!!");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row * g_actual_width + initial_top_left_col * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row * g_actual_width + (initial_bottom_right_col + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_dcba[initial_start], width);
        return;
    }
    else if (CDAB()) {        
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row * g_actual_width + initial_top_left_col * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row * g_actual_width + (initial_bottom_right_col + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_cdab[initial_start], width);
        return;
    }
    else if (BDAC()) {
        // printf("BDAC!!!!");
        memset(new_frame, 255, g_width * g_actual_width);
        register int initial_start = initial_top_left_row_rotated * g_actual_width + initial_top_left_col_rotated * 3;
        register int dest_start = top_left_row * g_actual_width + top_left_col * 3;
        register int width = initial_bottom_right_row_rotated * g_actual_width + (initial_bottom_right_col_rotated + 1) * 3 - initial_start;
        memcpy(&new_frame[dest_start], &initial_buffer_bdac[initial_start], width);
        // printBMP(g_width, g_width, new_frame);
        return;
    }
    // else {
    //     printf("GET UPDATED FRAME ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");
    // }
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = "吞吞爸";
    // Please fill in your information
    char student_first_name[] = "Jackson";
    char student_last_name[] = "Nie";
    char student_student_number[] = "1005282409";

    // Printing out team information
    printf("*******************************************************************************************************\n");
    printf("Team Information:\n");
    printf("\tteam_name: %s\n", team_name);
    printf("\tstudent_first_name: %s\n", student_first_name);
    printf("\tstudent_last_name: %s\n", student_last_name);
    printf("\tstudent_student_number: %s\n", student_student_number);
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          You can modify anything else in this file
 ***********************************************************************************************************************
 * @param sensor_values - structure stores parsed key value pairs of program instructions
 * @param sensor_values_count - number of valid sensor values parsed from sensor log file or commandline console
 * @param frame_buffer - pointer pointing to a buffer storing the imported  24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param grading_mode - turns off verification and turn on instrumentation
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void implementation_driver(struct kv *sensor_values, int sensor_values_count, unsigned char *frame_buffer,
                           unsigned int width, unsigned int height, bool grading_mode) {
    int processed_frames = 0;
    int net_shift_hor = 0;
    int net_shift_ver = 0;
    int net_rotate = 0;
    int net_mirror_x = 0;
    int net_mirror_y = 0;
    char x_dir = 1;
    char y_dir = 1;
    g_width = width;
    g_actual_width = width * 3;

    /* these share the same corner coordinates*/
    initial_buffer_abcd = frame_buffer;

    initial_buffer_badc = allocateFrame(g_width, g_width);

    /* these share another set of the same corner coordinates */
    initial_buffer_acbd = allocateFrame(g_width, g_width);

    initial_buffer_cadb = allocateFrame(g_width, g_width);

    initial_buffer_dbca = allocateFrame(g_width, g_width);

    initial_buffer_dcba = allocateFrame(g_width, g_width);
    
    initial_buffer_bdac = allocateFrame(g_width, g_width);

    initial_buffer_cdab = allocateFrame(g_width, g_width);

    /* buffer we use to verify frame */
    frame_buffer = allocateFrame(g_width, g_width);

    set_image_bounds(initial_buffer_abcd);
    set_initial_buffers();
    if (!found_image) {
        for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx += 25) {
             verifyFrame(initial_buffer_abcd, g_width, g_width, grading_mode);
        }
        return;
    }
    // printf("WIDTH: %d\n", width);
    // printf("top_left: %d %d\n", top_left_row, top_left_col);
    // printf("top_right: %d %d\n", top_right_row, top_right_col);
    // printf("bot_left: %d %d\n", bottom_left_row, bottom_left_col);
    // printf("bot_right: %d %d\n", bottom_right_row, bottom_right_col);
    register char *sk;
    register int sensorValueIdx = 0;

    sensor_values_count *= 0.04;
    sensor_values_count *= 25;

    for (; sensorValueIdx < sensor_values_count; sensorValueIdx += 5) {
        // printf("hi.\n");
//        printf("Processing sensor value #%d: %s, %d\n", sensorValueIdx, sensor_values[sensorValueIdx].key,
//               sensor_values[sensorValueIdx].value);
        sk = sensor_values[sensorValueIdx].key;
        if (!strcmp(sk, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CCW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        // ******************* Unroll 1
        sk = sensor_values[sensorValueIdx + 1].key;
        if (!strcmp(sk, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx + 1].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx + 1].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx + 1].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx + 1].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CW")) {
            int value = sensor_values[sensorValueIdx + 1].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CCW")) {
            int value = sensor_values[sensorValueIdx + 1].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        // ******************* Unroll 2
        sk = sensor_values[sensorValueIdx + 2].key;
        if (!strcmp(sk, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx + 2].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx + 2].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx + 2].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx + 2].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CW")) {
            int value = sensor_values[sensorValueIdx + 2].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CCW")) {
            int value = sensor_values[sensorValueIdx + 2].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        // ******************* Unroll 3
        sk = sensor_values[sensorValueIdx + 3].key;
        if (!strcmp(sk, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx + 3].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx + 3].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx + 3].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx + 3].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CW")) {
            int value = sensor_values[sensorValueIdx + 3].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CCW")) {
            int value = sensor_values[sensorValueIdx + 3].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        // ******************* Unroll 4
        sk = sensor_values[sensorValueIdx + 4].key;
        if (!strcmp(sk, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx + 4].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx + 4].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx + 4].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx + 4].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CW")) {
            int value = sensor_values[sensorValueIdx + 4].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "CCW")) {
            int value = sensor_values[sensorValueIdx + 4].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sk, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        if (processed_frames % 25 == 0) {
            // printf("Verifying!!!!\n");
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(-net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(-net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX();
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY();
                net_mirror_y = 0;
            }
            update_frame_buffer(frame_buffer);
            verifyFrame(frame_buffer, g_width, g_width, grading_mode);
        }
    }
    deallocateFrame(frame_buffer);
    frame_buffer = NULL;
    deallocateFrame(initial_buffer_badc);
    initial_buffer_badc = NULL;
    deallocateFrame(initial_buffer_acbd);
    initial_buffer_acbd = NULL;
    deallocateFrame(initial_buffer_cadb);
    initial_buffer_cadb = NULL;
    deallocateFrame(initial_buffer_dbca);
    initial_buffer_dbca = NULL;
    deallocateFrame(initial_buffer_dcba);
    initial_buffer_dcba = NULL; 
    deallocateFrame(initial_buffer_bdac);
    initial_buffer_bdac = NULL; 
    deallocateFrame(initial_buffer_cdab);
    initial_buffer_cdab = NULL;
    return;
}
