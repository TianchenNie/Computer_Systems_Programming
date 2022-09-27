#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "utilities.h"  // DO NOT REMOVE this line
#include "implementation_reference.h"   // DO NOT REMOVE this line


#define min(_a,_b) (((_a) < (_b)) ? (_a) : (_b))
#define max(_a,_b) (((_a) > (_b)) ? (_a) : (_b))
#define abs(_a) (((_a) >= 0) ? (_a) : (-1*(_a)))

// TODO: find a way to accumulate rotates, shifts, and mirrors. Currently the problem
// is that rotate then mirror != mirror then rotate && shift then mirror != mirror then shift
// TODO: cache the top left bottom right bounds of image of interest, right now rotating/looping through
// background which is unnecessary
// TODO: find a way to do loop unrolling
void processMoveUp(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveDown(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveLeft(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveRight(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);

/* [row, col] */
static int top_left[2];
static int bottom_left[2];
static int top_right[2];
static int bottom_right[2];

void print_bounds() {
    printf("top_left row col (%d, %d)\n", top_left[0], top_left[1]);
    printf("bottom_left row col (%d, %d)\n", bottom_left[0], bottom_left[1]);
    printf("top_right row col (%d, %d)\n", top_right[0], top_right[1]);
    printf("bottom_right row col (%d, %d)\n", bottom_right[0], bottom_right[1]);
}

void set_image_bounds(unsigned char *buffer_frame, unsigned width, unsigned height) {
    int top_row = -1;
    int left_col = 1e6;
    int bot_row = -1;
    int right_col = -1;
    size_t s = sizeof(int);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            int cell = row * width * 3 + col * 3;
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
            }
        }
    }
    top_left[0] = top_row;
    top_left[1] = left_col;
    bottom_left[0] = bot_row;
    bottom_left[1] = left_col;
    top_right[0] = top_row;
    top_right[1] = right_col;
    bottom_right[0] = bot_row;
    bottom_right[1] = right_col;
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
void processMoveUp(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
    if (offset == 0) return;
    if (offset < 0) {
        processMoveDown(buffer_frame, width, height, -offset);
        return;
    }
    int shift = offset * width * 3;
    for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
        for (int col = top_left[1]; col <= top_right[1]; ++col) {
            int cell = row * width * 3 + col * 3;
            if (buffer_frame[cell] != 255 || buffer_frame[cell + 1] != 255 || buffer_frame[cell + 2] != 255) {
                buffer_frame[cell - shift] = buffer_frame[cell];
                buffer_frame[cell] = 255;
                buffer_frame[cell + 1 - shift] = buffer_frame[cell + 1];
                buffer_frame[cell + 1] = 255;
                buffer_frame[cell + 2 - shift] = buffer_frame[cell + 2];
                buffer_frame[cell + 2] = 255;
            }
        }
    }
    top_left[0] -= offset;
    top_right[0] -= offset;
    bottom_left[0] -= offset;
    bottom_right[0] -= offset;
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
void processMoveDown(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
    if (offset == 0) return;
    if (offset < 0) {
        processMoveUp(buffer_frame, width, height, -offset);
        return;
    }
    int shift = offset * width * 3;
    for (int row = bottom_left[0]; row >= top_left[0]; --row) {
        for (int col = bottom_left[1]; col <= bottom_right[1]; ++col) {
            int cell = row * width * 3 + col * 3;
            if (buffer_frame[cell] != 255 || buffer_frame[cell + 1] != 255 || buffer_frame[cell + 2] != 255) {
                buffer_frame[cell + shift] = buffer_frame[cell];
                buffer_frame[cell] = 255;
                buffer_frame[cell + 1 + shift] = buffer_frame[cell + 1];
                buffer_frame[cell + 1] = 255;
                buffer_frame[cell + 2 + shift] = buffer_frame[cell + 2];
                buffer_frame[cell + 2] = 255;
            }
        }
    }
    top_left[0] += offset;
    top_right[0] += offset;
    bottom_left[0] += offset;
    bottom_right[0] += offset;
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
void processMoveLeft(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
    if (offset == 0) return;
    if (offset < 0) {
        processMoveRight(buffer_frame, width, height, -offset);
        return;
    }
    int shift = offset * 3;
    for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
        for (int col = top_left[1]; col <= top_right[1]; ++col) {
            int cell = row * width * 3 + col * 3;
            if (buffer_frame[cell] != 255 || buffer_frame[cell + 1] != 255 || buffer_frame[cell + 2] != 255) {
                buffer_frame[cell - shift] = buffer_frame[cell];
                buffer_frame[cell] = 255;
                buffer_frame[cell + 1 - shift] = buffer_frame[cell + 1];
                buffer_frame[cell + 1] = 255;
                buffer_frame[cell + 2 - shift] = buffer_frame[cell + 2];
                buffer_frame[cell + 2] = 255;
            }
        }
    }
    top_left[1] -= offset;
    top_right[1] -= offset;
    bottom_left[1] -= offset;
    bottom_right[1] -= offset;
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
void processMoveRight(unsigned char *buffer_frame, unsigned width, unsigned height, int offset) {
    if (offset == 0) return;
    if (offset < 0) {
        processMoveLeft(buffer_frame, width, height, -offset);
        return;
    }
    int shift = offset * 3;
    for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
        for (int col = top_right[1]; col >= top_left[1]; --col) {
            int cell = row * width * 3 + col * 3;
            if (buffer_frame[cell] != 255 || buffer_frame[cell + 1] != 255 || buffer_frame[cell + 2] != 255) {
                buffer_frame[cell + shift] = buffer_frame[cell];
                buffer_frame[cell] = 255;
                buffer_frame[cell + 1 + shift] = buffer_frame[cell + 1];
                buffer_frame[cell + 1] = 255;
                buffer_frame[cell + 2 + shift] = buffer_frame[cell + 2];
                buffer_frame[cell + 2] = 255;
            }
        }
    }
    top_left[1] += offset;
    top_right[1] += offset;
    bottom_left[1] += offset;
    bottom_right[1] += offset;
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorX(unsigned char *buffer_frame, unsigned int width, unsigned int height) {
    int top_row = top_left[0];
    int bottom_row = bottom_left[0];
    float x_axis = (height - 1) * 0.5;
    float distance_top_row = abs(top_row - x_axis);
    float distance_bot_row = abs(bottom_row - x_axis);
    int row = distance_bot_row > distance_top_row ? bottom_row : top_row;
    int mirrored_row = height - 1 - row;
    if (mirrored_row > row) {
        // store shifted pixels to temporary buffer
        for (; row < mirrored_row; ++row, --mirrored_row) {
            for (int col = top_left[1]; col <= top_right[1]; ++col) {
                int my_cell = row * width * 3 + col * 3;
                int mirrored_cell = mirrored_row * width * 3 + col * 3;
                unsigned char temp_1 = buffer_frame[my_cell];
                unsigned char temp_2 = buffer_frame[my_cell + 1];
                unsigned char temp_3 = buffer_frame[my_cell + 2];
                buffer_frame[my_cell] = buffer_frame[mirrored_cell];
                buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
                buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
                buffer_frame[mirrored_cell] = temp_1;
                buffer_frame[mirrored_cell + 1] = temp_2;
                buffer_frame[mirrored_cell + 2] = temp_3;
            }
        }
    }
    else if (mirrored_row < row) {
        // store shifted pixels to temporary buffer
        for (; row > mirrored_row; --row, ++mirrored_row) {
            for (int col = top_left[1]; col <= top_right[1]; ++col) {
                int my_cell = row * width * 3 + col * 3;
                int mirrored_cell = mirrored_row * width * 3 + col * 3;
                unsigned char temp_1 = buffer_frame[my_cell];
                unsigned char temp_2 = buffer_frame[my_cell + 1];
                unsigned char temp_3 = buffer_frame[my_cell + 2];
                buffer_frame[my_cell] = buffer_frame[mirrored_cell];
                buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
                buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
                buffer_frame[mirrored_cell] = temp_1;
                buffer_frame[mirrored_cell + 1] = temp_2;
                buffer_frame[mirrored_cell + 2] = temp_3;
            }
        }
    }
    else {
        return;
    }
    // // store shifted pixels to temporary buffer
    // for (; row < bottom_row; ++row) {
    //     for (int col = top_left[1]; col <= top_right[1]; ++col) {
    //         int my_cell = row * width * 3 + col * 3;
    //         int mirrored_cell = (height - 1 - row) * width * 3 + col * 3;
    //         unsigned char temp_1 = buffer_frame[my_cell];
    //         unsigned char temp_2 = buffer_frame[my_cell + 1];
    //         unsigned char temp_3 = buffer_frame[my_cell + 2];
    //         buffer_frame[my_cell] = buffer_frame[mirrored_cell];
    //         buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
    //         buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
    //         buffer_frame[mirrored_cell] = temp_1;
    //         buffer_frame[mirrored_cell + 1] = temp_2;
    //         buffer_frame[mirrored_cell + 2] = temp_3;
    //     }
    // }
    int temp = top_left[0];
    top_left[0] = bottom_left[0] + (x_axis - bottom_left[0]) * 2;
    top_right[0] = top_left[0];
    bottom_left[0] = temp + (x_axis - temp) * 2;
    bottom_right[0] = bottom_left[0];
    // int old_bottom = bottom_left[0];
    // bottom_left[0] = height - 1 - top_left[0];
    // bottom_right[0] = height - 1 - top_left[0];
    // top_left[0] = height - 1 - old_bottom;
    // top_right[0] = height - 1 - old_bottom;
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorY(unsigned char *buffer_frame, unsigned width, unsigned height) {
    int left_col = top_left[1];
    int right_col = top_right[1];
    float y_axis = (width - 1) * 0.5;
    float distance_left_col = abs(left_col - y_axis);
    float distance_right_col = abs(right_col - y_axis);
    // printf("Distance_left_col: %f\n", abs(left_col - y_axis));
    // printf("Distance_right_col: %f\n", distance_right_col);
    int col = distance_left_col > distance_right_col ? left_col : right_col;
    int mirrored_col = width - 1 - col;
    if (mirrored_col > col) {
        for (; col < mirrored_col; ++col, --mirrored_col) {
            for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
                int my_cell = row * width * 3 + col * 3;
                int mirrored_cell = row * width * 3 + mirrored_col * 3;
                unsigned char temp_1 = buffer_frame[my_cell];
                unsigned char temp_2 = buffer_frame[my_cell + 1];
                unsigned char temp_3 = buffer_frame[my_cell + 2];
                buffer_frame[my_cell] = buffer_frame[mirrored_cell];
                buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
                buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
                buffer_frame[mirrored_cell] = temp_1;
                buffer_frame[mirrored_cell + 1] = temp_2;
                buffer_frame[mirrored_cell + 2] = temp_3;
            }
        }
    }
    else if (mirrored_col < col) {
        for (; col > mirrored_col; --col, ++mirrored_col) {
            for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
                int my_cell = row * width * 3 + col * 3;
                int mirrored_cell = row * width * 3 + mirrored_col * 3;
                unsigned char temp_1 = buffer_frame[my_cell];
                unsigned char temp_2 = buffer_frame[my_cell + 1];
                unsigned char temp_3 = buffer_frame[my_cell + 2];
                buffer_frame[my_cell] = buffer_frame[mirrored_cell];
                buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
                buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
                buffer_frame[mirrored_cell] = temp_1;
                buffer_frame[mirrored_cell + 1] = temp_2;
                buffer_frame[mirrored_cell + 2] = temp_3;
            }
        }
    }
    else {
        return;
    }
    // printf("col %d\n", col);
    // printf("mirrored_col %d\n", mirrored_col);
    // for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
    //     for (int col = left_col; col <= right_col; ++col) {
    //         int my_cell = row * width * 3 + col * 3;
    //         int mirrored_cell = row * width * 3 + (width - 1 - col) * 3;
    //         unsigned char temp_1 = buffer_frame[my_cell];
    //         unsigned char temp_2 = buffer_frame[my_cell + 1];
    //         unsigned char temp_3 = buffer_frame[my_cell + 2];
    //         buffer_frame[my_cell] = buffer_frame[mirrored_cell];
    //         buffer_frame[my_cell + 1] = buffer_frame[mirrored_cell + 1];
    //         buffer_frame[my_cell + 2] = buffer_frame[mirrored_cell + 2];
    //         buffer_frame[mirrored_cell] = temp_1;
    //         buffer_frame[mirrored_cell + 1] = temp_2;
    //         buffer_frame[mirrored_cell + 2] = temp_3;
    //     }
    // }
    int temp = top_left[1];
    top_left[1] = top_right[1] + (y_axis - top_right[1]) * 2;
    bottom_left[1] = top_left[1];
    top_right[1] = temp + (y_axis - temp) * 2;
    bottom_right[1] = top_right[1];
    // int old_left = top_left[1];
    // top_left[1] = width - 1 - top_right[1];
    // bottom_left[1] = top_left[1];
    // top_right[1] = width - 1 - old_left;
    // bottom_right[1] = top_right[1];
    return;
}

void processRotateCW1(unsigned char *buffer_frame, unsigned width, unsigned height) {
    // transpose the image
    // for (int row = 0; row < height; ++row) {
    //     for (int col = row + 1; col < width; ++col) {
    //         int row_col = row * width * 3 + col * 3;
    //         int col_row = col * width * 3 + row * 3;
    //         unsigned char temp_1 = buffer_frame[row_col];
    //         unsigned char temp_2 = buffer_frame[row_col + 1];
    //         unsigned char temp_3 = buffer_frame[row_col + 2];
    //         buffer_frame[row_col] = buffer_frame[col_row];
    //         buffer_frame[row_col + 1] = buffer_frame[col_row + 1];
    //         buffer_frame[row_col + 2] = buffer_frame[col_row + 2];
    //         buffer_frame[col_row] = temp_1;
    //         buffer_frame[col_row + 1] = temp_2;
    //         buffer_frame[col_row + 2] = temp_3;
    //     }
    // }
    

    // printBMP(width, height, buffer_frame);
    int net_col = top_right[1] - top_left[1] + 1;
    int net_row = bottom_left[0] - top_left[0] + 1;
    // transpose the image
    for (int row = 0; row < height; ++row) {
        for (int col = row + 1; col < width; ++col) {
            int row_col = row * width * 3 + col * 3;
            int col_row = col * width * 3 + row * 3;
            unsigned char temp_1 = buffer_frame[row_col];
            unsigned char temp_2 = buffer_frame[row_col + 1];
            unsigned char temp_3 = buffer_frame[row_col + 2];
            buffer_frame[row_col] = buffer_frame[col_row];
            buffer_frame[row_col + 1] = buffer_frame[col_row + 1];
            buffer_frame[row_col + 2] = buffer_frame[col_row + 2];
            buffer_frame[col_row] = temp_1;
            buffer_frame[col_row + 1] = temp_2;
            buffer_frame[col_row + 2] = temp_3;
        }
    }
    // for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
    //     for (int col = row + 1; col <= top_right[1]; ++col) {
    //         int row_col = row * width * 3 + col * 3;
    //         int col_row = col * width * 3 + row * 3;
    //         unsigned char temp_1 = buffer_frame[row_col];
    //         unsigned char temp_2 = buffer_frame[row_col + 1];
    //         unsigned char temp_3 = buffer_frame[row_col + 2];
    //         buffer_frame[row_col] = buffer_frame[col_row];
    //         buffer_frame[row_col + 1] = buffer_frame[col_row + 1];
    //         buffer_frame[row_col + 2] = buffer_frame[col_row + 2];
    //         buffer_frame[col_row] = temp_1;
    //         buffer_frame[col_row + 1] = temp_2;
    //         buffer_frame[col_row + 2] = temp_3;
    //     }
    // }

    // top_left -> top_left 
    // top_right -> bottom_left 
    // bottom_left -> top_right 
    // bottom_right -> bottom_right
    int temp;
    // top_left
    temp = top_left[0];
    top_left[0] = top_left[1];
    top_left[1] = temp;

    // bottom_right
    temp = bottom_right[0];
    bottom_right[0] = bottom_right[1];
    bottom_right[1] = temp;

    // top_right
    temp = top_right[0];
    top_right[0] = top_right[1];
    top_right[1] = temp;

    // bottom_left
    temp = bottom_left[0];
    bottom_left[0] = bottom_left[1];
    bottom_left[1] = temp;

    // swap top_right bottom_left
    int temp1 = top_right[0];
    int temp2 = top_right[1];
    top_right[0] = bottom_left[0];
    top_right[1] = bottom_left[1];
    bottom_left[0] = temp1;
    bottom_left[1] = temp2;
    // bottom_left[0] = top_left[0] + net_col;
    // bottom_right[0] = bottom_left[0];
    // bottom_right[1] = top_right[1];
    // print_bounds();

    // // reflect on y axis
    // for (int row = 0; row < height; ++row) {
    //     int left = 0;
    //     int right = width - 1;
    //     int row_cell = row * width * 3;
    //     for(; left < right; ++left, --right) {
    //         int row_cell_left = row_cell + left * 3;
    //         int row_cell_right = row_cell + right * 3;
    //         unsigned char temp_1 = buffer_frame[row_cell_left];
    //         unsigned char temp_2 = buffer_frame[row_cell_left + 1];
    //         unsigned char temp_3 = buffer_frame[row_cell_left + 2];
    //         buffer_frame[row_cell_left] = buffer_frame[row_cell_right];
    //         buffer_frame[row_cell_left + 1] = buffer_frame[row_cell_right + 1];
    //         buffer_frame[row_cell_left + 2] = buffer_frame[row_cell_right + 2];
    //         buffer_frame[row_cell_right] = temp_1;
    //         buffer_frame[row_cell_right + 1] = temp_2;
    //         buffer_frame[row_cell_right + 2] = temp_3;
    //     }
    // }
    processMirrorY(buffer_frame, width, height);
    // printBMP(width, height, buffer_frame);
    return;
}

void processRotateCW2(unsigned char *buffer_frame, unsigned width, unsigned height) {
    processMirrorX(buffer_frame, width, height);
    processMirrorY(buffer_frame, width, height);
    return;
}

void processRotateCW3(unsigned char *buffer_frame, unsigned width, unsigned height) {
    // transpose the image
    for (int row = 0; row < height; ++row) {
        for (int col = row + 1; col < width; ++col) {
            int row_col = row * width * 3 + col * 3;
            int col_row = col * width * 3 + row * 3;
            unsigned char temp_1 = buffer_frame[row_col];
            unsigned char temp_2 = buffer_frame[row_col + 1];
            unsigned char temp_3 = buffer_frame[row_col + 2];
            buffer_frame[row_col] = buffer_frame[col_row];
            buffer_frame[row_col + 1] = buffer_frame[col_row + 1];
            buffer_frame[row_col + 2] = buffer_frame[col_row + 2];
            buffer_frame[col_row] = temp_1;
            buffer_frame[col_row + 1] = temp_2;
            buffer_frame[col_row + 2] = temp_3;
        }
    }
    // for (int row = top_left[0]; row <= bottom_left[0]; ++row) {
    //     for (int col = row + 1; col <= top_right[1]; ++col) {
    //         int row_col = row * width * 3 + col * 3;
    //         int col_row = col * width * 3 + row * 3;
    //         unsigned char temp_1 = buffer_frame[row_col];
    //         unsigned char temp_2 = buffer_frame[row_col + 1];
    //         unsigned char temp_3 = buffer_frame[row_col + 2];
    //         buffer_frame[row_col] = buffer_frame[col_row];
    //         buffer_frame[row_col + 1] = buffer_frame[col_row + 1];
    //         buffer_frame[row_col + 2] = buffer_frame[col_row + 2];
    //         buffer_frame[col_row] = temp_1;
    //         buffer_frame[col_row + 1] = temp_2;
    //         buffer_frame[col_row + 2] = temp_3;
    //     }
    // }

    // top_left -> top_left 
    // top_right -> bottom_left 
    // bottom_left -> top_right 
    // bottom_right -> bottom_right
    int temp;
    // top_left
    temp = top_left[0];
    top_left[0] = top_left[1];
    top_left[1] = temp;

    // bottom_right
    temp = bottom_right[0];
    bottom_right[0] = bottom_right[1];
    bottom_right[1] = temp;

    // top_right
    temp = top_right[0];
    top_right[0] = top_right[1];
    top_right[1] = temp;

    // bottom_left
    temp = bottom_left[0];
    bottom_left[0] = bottom_left[1];
    bottom_left[1] = temp;

    // swap top_right bottom_left
    int temp1 = top_right[0];
    int temp2 = top_right[1];
    top_right[0] = bottom_left[0];
    top_right[1] = bottom_left[1];
    bottom_left[0] = temp1;
    bottom_left[1] = temp2;
    processMirrorX(buffer_frame, width, height);
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
void processRotateCW(unsigned char *buffer_frame, unsigned width, unsigned height, int rotate_iteration) {
    if (rotate_iteration == 0 || rotate_iteration == 4) {
        return;
    }
    else if (rotate_iteration == 1) {
        processRotateCW1(buffer_frame, width, height);
        return;
    }
    else if (rotate_iteration == 2) {
        processRotateCW2(buffer_frame, width, height);
        return;
    }
    processRotateCW3(buffer_frame, width, height);
    return;
}


/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = ":) (:";
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
    set_image_bounds(frame_buffer, width, height);
    // printf("WIDTH: %d\n", width);
    // printf("top_left: %d %d\n", top_left[0], top_left[1]);
    // printf("top_right: %d %d\n", top_right[0], top_right[1]);
    // printf("bot_left: %d %d\n", bottom_left[0], bottom_left[1]);
    // printf("bot_right: %d %d\n", bottom_right[0], bottom_right[1]);
    for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx++) {
//        printf("Processing sensor value #%d: %s, %d\n", sensorValueIdx, sensor_values[sensorValueIdx].key,
//               sensor_values[sensorValueIdx].value);
        if (!strcmp(sensor_values[sensorValueIdx].key, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "S")) {
            net_shift_ver -= sensor_values[sensorValueIdx].value;

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx].value;
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate += value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(frame_buffer, width, height, net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(frame_buffer, width, height, -net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(frame_buffer, width, height, net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(frame_buffer, width, height, -net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CCW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate -= value;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(frame_buffer, width, height, net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(frame_buffer, width, height, -net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(frame_buffer, width, height, net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(frame_buffer, width, height, -net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MX")) {
            net_mirror_x = net_mirror_x ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(frame_buffer, width, height, net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(frame_buffer, width, height, -net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(frame_buffer, width, height, net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(frame_buffer, width, height, -net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MY")) {
            net_mirror_y = net_mirror_y ^ 1;
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(frame_buffer, width, height, net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(frame_buffer, width, height, -net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(frame_buffer, width, height, net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(frame_buffer, width, height, -net_shift_ver);
                net_shift_ver = 0;
            }
            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;

        if (processed_frames % 25 == 0){
            /* ---------Clear shift cache--------- */
            if (net_shift_hor > 0) {
                processMoveRight(frame_buffer, width, height, net_shift_hor);
                net_shift_hor = 0;
            }
            else if (net_shift_hor < 0) {
                processMoveLeft(frame_buffer, width, height, -net_shift_hor);
                net_shift_hor = 0;
            }
            if (net_shift_ver > 0) {
                processMoveUp(frame_buffer, width, height, net_shift_ver);
                net_shift_ver = 0;
            }
            else if (net_shift_ver < 0) {
                processMoveDown(frame_buffer, width, height, -net_shift_ver);
                net_shift_ver = 0;
            }

            /* ---------Clear rotate cache--------- */
            if (net_rotate > 0) {
                processRotateCW(frame_buffer, width, height, net_rotate % 4);
                net_rotate = 0;
            }
            else if (net_rotate < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
                net_rotate = 0;
            }

            /* ---------Clear mirror cache--------- */
            if (net_mirror_x == 1) {
                processMirrorX(frame_buffer, width, height);
                net_mirror_x = 0;
            }
            
            if (net_mirror_y == 1) {
                processMirrorY(frame_buffer, width, height);
                net_mirror_y = 0;
            }
            verifyFrame(frame_buffer, width, height, grading_mode);
        }
    }
    return;
}
