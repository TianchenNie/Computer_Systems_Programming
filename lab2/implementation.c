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


// TODO: find a way to accumulate rotates, shifts, and mirrors. Currently the problem
// is that rotate then mirror != mirror then rotate && shift then mirror != mirror then shift
// TODO: cache the top left bottom right bounds of image of interest, right now rotating/looping through
// background which is unnecessary
// TODO: find a way to do loop unrolling

/* [row, col] */
static unsigned char *initial_buffer;
static short int top_left_row;
static short int top_left_col;
static short int bottom_left_row;
static short int bottom_left_col;
static short int top_right_row;
static short int top_right_col;
static short int bottom_right_row;
static short int bottom_right_col;
static short int initial_top_left_row;
static short int initial_top_left_col;
static short int initial_bottom_left_row;
static short int initial_bottom_left_col;
static short int initial_top_right_row;
static short int initial_top_right_col;
static short int initial_bottom_right_row;
static short int initial_bottom_right_col;
static short int a_row;
static short int a_col;
static short int b_row;
static short int b_col;
static short int c_row;
static short int c_col;
static short int d_row;
static short int d_col;
static short int found_image;

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
}

/*
case1
A B
C D
*/
static inline __attribute__((always_inline)) short int ABCD() {
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
static inline __attribute__((always_inline)) short int ACBD() {
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
static inline __attribute__((always_inline)) short int BADC() {
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
static inline __attribute__((always_inline)) short int CADB() {
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
static inline __attribute__((always_inline)) short int DBCA() {
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
static inline __attribute__((always_inline)) short int DCBA() {
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
static inline __attribute__((always_inline)) short int CDAB() {
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
static inline __attribute__((always_inline)) short int BDAC() {
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
    size_t s = sizeof(int);
    int found = 0;
    for (int row = 0; row < g_width; ++row) {
        for (int col = 0; col < g_width; ++col) {
            int cell = row * g_width * 3 + col * 3;
            if (buffer_frame[cell] != 255 || buffer_frame[cell + 1] != 255 || buffer_frame[cell + 2] != 255) {
                // printf("CELL 1: %d\n", buffer_frame[cell]);
                // printf("CELL 2: %d\n", buffer_frame[cell + 1]);
                // printf("CELL 3: %d\n", buffer_frame[cell + 2]);
                if (top_row < 0) {
                    top_row = row;
                }
                left_col = min(col, left_col);
                bot_row = max(row, bot_row);
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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
    short int c = 0;
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

unsigned char *get_frame_from_vertices() {
    // printf("Getting frame...\n");
    // print_bounds();
    // print_abcd();
    if (ABCD()) {
        // printf("ABCD!!!!!\n");
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register unsigned short int bound = (bottom_left_row - top_left_row) >> 2;
        bound = bound << 2;
        register short int width = (top_right_col + 1) * 3 - top_left_col * 3;
        for (; i < bound; i += 4) {
            memcpy(&new_frame[(top_left_row + i) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(top_left_row + i + 1) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 1) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(top_left_row + i + 2) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 2) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(top_left_row + i + 3) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 3) * g_width * 3 + initial_top_left_col * 3],
                width);
        }
        for (; i <= bottom_left_row - top_left_row; i++) {
            memcpy(&new_frame[(top_left_row + i) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i) * g_width * 3 + initial_top_left_col * 3],
                width);
        }
        // printBMP(g_width, g_width, new_frame);
        return new_frame;
    }
    else if (ACBD()) { // needs testing
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j <= bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 1) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 2) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 3) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 1
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_left_col + i + 1) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_left_col + i + 2) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_left_col + i + 3) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_left_col + i + 1) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_left_col + i + 2) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_left_col + i + 3) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_left_col + i + 1) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_left_col + i + 2) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_left_col + i + 3) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 1) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 2) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i + 3) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }

        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    else if (BADC()) {
        // printf("BADC!!!!");
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j < bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 1) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 2) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 3) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 1
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (top_left_row + i + 1) * g_width * 3 + (top_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (top_left_row + i + 2) * g_width * 3 + (top_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (top_left_row + i + 3) * g_width * 3 + (top_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (top_left_row + i + 1) * g_width * 3 + (top_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (top_left_row + i + 2) * g_width * 3 + (top_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (top_left_row + i + 3) * g_width * 3 + (top_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (top_left_row + i + 1) * g_width * 3 + (top_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (top_left_row + i + 2) * g_width * 3 + (top_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (top_left_row + i + 3) * g_width * 3 + (top_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 1) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 1) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 2) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 2) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + i + 3) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i + 3) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + i) * g_width * 3 + (top_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    else if (CADB()) { // rotate CW1
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j < bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 1
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j + 1) * 3;
                dest_pixel_addr = (top_left_row + j + 1) * g_width * 3 + (top_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j + 2) * 3;
                dest_pixel_addr = (top_left_row + j + 2) * g_width * 3 + (top_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);

                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j + 3) * 3;
                dest_pixel_addr = (top_left_row + j + 3) * g_width * 3 + (top_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (top_left_row + j) * g_width * 3 + (top_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    else if (DBCA()) {
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j < bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 1
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_right_col - i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    else if (DCBA()) {
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j < bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 1)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 2)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 3)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 1
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 1)) * g_width * 3 + (bottom_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 2)) * g_width * 3 + (bottom_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 3)) * g_width * 3 + (bottom_right_col - (j + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 1)) * g_width * 3 + (bottom_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 2)) * g_width * 3 + (bottom_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 3)) * g_width * 3 + (bottom_right_col - (j + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 1)) * g_width * 3 + (bottom_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 2)) * g_width * 3 + (bottom_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 3)) * g_width * 3 + (bottom_right_col - (j + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 1)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 2)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - (i + 3)) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - i) * g_width * 3 + (bottom_right_col - j) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    else if (CDAB()) {        
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register unsigned short int bound = (bottom_left_row - top_left_row) >> 2;
        bound = bound << 2;
        register short int width = (top_right_col + 1) * 3 - top_left_col * 3;
        for (; i < bound; i += 4) {
            // printf("i: %d\n", i);
            memcpy(&new_frame[(bottom_left_row - i) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(bottom_left_row - i - 1) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 1) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(bottom_left_row - i - 2) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 2) * g_width * 3 + initial_top_left_col * 3],
                width);
            memcpy(&new_frame[(bottom_left_row - i - 3) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i + 3) * g_width * 3 + initial_top_left_col * 3],
                width);
        }
        for (; i <= bottom_left_row - top_left_row; i++) {
            // printf("NEW LOOP i: %d\n", i);
            memcpy(&new_frame[(bottom_left_row - i) * g_width * 3 + top_left_col * 3], 
                &initial_buffer[(initial_top_left_row + i) * g_width * 3 + initial_top_left_col * 3],
                width);
        }
        return new_frame;
    }
    else if (BDAC()) {
        unsigned char *new_frame = allocateFrame(g_width, g_width);
        memset(new_frame, 255, g_width * g_width * 3);
        register short int i = 0;
        register short int j = 0;
        register unsigned short int bound_i = (initial_bottom_left_row - initial_top_left_row) >> 2;
        bound_i = bound_i << 2;
        register unsigned short int bound_j = (initial_top_right_col - initial_top_left_col) >> 2;
        bound_j = bound_j << 2;
        register int initial_pixel_addr;
        register int dest_pixel_addr;
        for (; i < bound_i; i += 4) {
            for (j = 0; j < bound_j; j += 4) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                // unroll j 1                
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_left_col + (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_left_col + (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 1)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 1)) * g_width * 3 + (bottom_left_col + (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                // unroll j 2
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_left_col + (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_left_col + (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 2)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 2)) * g_width * 3 + (bottom_left_col + (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                
                // unroll j 3
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_left_col + (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_left_col + (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + (j + 3)) * 3;
                dest_pixel_addr = (bottom_left_row - (j + 3)) * g_width * 3 + (bottom_left_col + (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
            for (; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 1)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 1)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 2)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 2)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
                initial_pixel_addr = (initial_top_left_row + (i + 3)) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + (i + 3)) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        for (; i <= initial_bottom_left_row - initial_top_left_row; i++) {
            for (j = 0; j <= initial_top_right_col - initial_top_left_col; j++) {
                initial_pixel_addr = (initial_top_left_row + i) * g_width * 3 + (initial_top_left_col + j) * 3;
                dest_pixel_addr = (bottom_left_row - j) * g_width * 3 + (bottom_left_col + i) * 3;
                memcpy(&new_frame[dest_pixel_addr], &initial_buffer[initial_pixel_addr], 3);
            }
        }
        return new_frame;
    }
    // else {
    //     printf("GET UPDATED FRAME ERROR!!!!!!!!!!!!! UNKNOWN CASE, DEBUG PLEASE\n");
    //     return NULL;
    // }
    return NULL;
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = "alwidubalsdh";
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
    g_width = width;
    initial_buffer = frame_buffer;
    frame_buffer = allocateFrame(g_width, g_width);
    memcpy(frame_buffer, initial_buffer, g_width * g_width * 3);
    set_image_bounds(initial_buffer);
    if (!found_image) {
        for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx += 25) {
             verifyFrame(frame_buffer, g_width, g_width, grading_mode);
        }
        return;
    }
    // printf("WIDTH: %d\n", width);
    // printf("top_left: %d %d\n", top_left_row, top_left_col);
    // printf("top_right: %d %d\n", top_right_row, top_right_col);
    // printf("bot_left: %d %d\n", bottom_left_row, bottom_left_col);
    // printf("bot_right: %d %d\n", bottom_right_row, bottom_right_col);
    for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx++) {
//        printf("Processing sensor value #%d: %s, %d\n", sensorValueIdx, sensor_values[sensorValueIdx].key,
//               sensor_values[sensorValueIdx].value);
        if (!strcmp(sensor_values[sensorValueIdx].key, "W")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "A")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "S")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "D")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CW")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CCW")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MX")) {
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
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MY")) {
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
            deallocateFrame(frame_buffer);
            frame_buffer = get_frame_from_vertices();
            verifyFrame(frame_buffer, g_width, g_width, grading_mode);
        }
    }
    return;
}
