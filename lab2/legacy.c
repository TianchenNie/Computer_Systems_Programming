
/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param offset - number of pixels to shift the object in bitmap image up
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note1: White pixels RGB(255,255,255) are treated as background. Object in the image refers to non-white pixels.
 * Note2: You can assume the object will never be moved off the screen
 **********************************************************************************************************************/
unsigned char *processMoveUp(unsigned char *buffer_frame, int offset) {
    // print_bounds();
    int shift = offset * g_width * 3;
    unsigned char *new_frame = allocateFrame(g_width, g_width);
    memset(new_frame, 255, g_width * g_width * 3);
    int begin = top_left_row * g_width * 3;
    memcpy(&new_frame[begin - shift], &buffer_frame[begin], (bottom_left_row - top_left_row + 1) * g_width * 3);
    deallocateFrame(buffer_frame);
    top_left_row -= offset;
    top_right_row -= offset;
    bottom_left_row -= offset;
    bottom_right_row -= offset;
    return new_frame;
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
unsigned char *processMoveDown(unsigned char *buffer_frame, int offset) {
    int shift = offset * g_width * 3;
    unsigned char *new_frame = allocateFrame(g_width, g_width);
    memset(new_frame, 255, g_width * g_width * 3);
    int begin = top_left_row * g_width * 3;
    memcpy(&new_frame[begin + shift], &buffer_frame[begin], (bottom_left_row - top_left_row + 1) * g_width * 3);

    top_left_row += offset;
    top_right_row += offset;
    bottom_left_row += offset;
    bottom_right_row += offset;
    deallocateFrame(buffer_frame);
    // printBMP(g_width, g_width, new_frame);
    return new_frame;
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
void processMoveLeft(unsigned char *buffer_frame, int offset) {
    int shift = offset * 3;
    unsigned char *new_frame = allocateFrame(g_width, g_width);
    memset(new_frame, 255, g_width * g_width * 3);
    for (int row = top_left_row; row <= bottom_left_row; ++row) {

    }
    top_left_col -= offset;
    top_right_col -= offset;
    bottom_left_col -= offset;
    bottom_right_col -= offset;
    deallocateFrame(buffer_frame);
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
void processMoveRight(unsigned char *buffer_frame, int offset) {
    if (offset == 0) return;
    if (offset < 0) {
        processMoveLeft(buffer_frame, -offset);
        return;
    }
    int shift = offset * 3;
    for (int row = top_left_row; row <= bottom_left_row; ++row) {
        for (int col = top_right_col; col >= top_left_col; --col) {
            int cell = row * g_width * 3 + col * 3;
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
    top_left_col += offset;
    top_right_col += offset;
    bottom_left_col += offset;
    bottom_right_col += offset;
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorX(unsigned char *buffer_frame) {
    int top_row = top_left_row;
    int bottom_row = bottom_left_row;
    float x_axis = (g_width - 1) * 0.5;
    float distance_top_row = abs(top_row - x_axis);
    float distance_bot_row = abs(bottom_row - x_axis);
    int row = distance_bot_row > distance_top_row ? bottom_row : top_row;
    int mirrored_row = g_width - 1 - row;
    if (mirrored_row > row) {
        // store shifted pixels to temporary buffer
        for (; row < mirrored_row; ++row, --mirrored_row) {
            for (int col = top_left_col; col <= top_right_col; ++col) {
                int my_cell = row * g_width * 3 + col * 3;
                int mirrored_cell = mirrored_row * g_width * 3 + col * 3;
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
            for (int col = top_left_col; col <= top_right_col; ++col) {
                int my_cell = row * g_width * 3 + col * 3;
                int mirrored_cell = mirrored_row * g_width * 3 + col * 3;
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
    int temp = top_left_row;
    top_left_row = bottom_left_row + (x_axis - bottom_left_row) * 2;
    top_right_row = top_left_row;
    bottom_left_row = temp + (x_axis - temp) * 2;
    bottom_right_row = bottom_left_row;
    return;
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorY(unsigned char *buffer_frame) {
    int left_col = top_left_col;
    int right_col = top_right_col;
    float y_axis = (g_width - 1) * 0.5;
    float distance_left_col = abs(left_col - y_axis);
    float distance_right_col = abs(right_col - y_axis);
    // printf("Distance_left_col: %f\n", abs(left_col - y_axis));
    // printf("Distance_right_col: %f\n", distance_right_col);
    int col = distance_left_col > distance_right_col ? left_col : right_col;
    int mirrored_col = g_width - 1 - col;
    if (mirrored_col > col) {
        for (; col < mirrored_col; ++col, --mirrored_col) {
            for (int row = top_left_row; row <= bottom_left_row; ++row) {
                int my_cell = row * g_width * 3 + col * 3;
                int mirrored_cell = row * g_width * 3 + mirrored_col * 3;
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
            for (int row = top_left_row; row <= bottom_left_row; ++row) {
                int my_cell = row * g_width * 3 + col * 3;
                int mirrored_cell = row * g_width * 3 + mirrored_col * 3;
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
    int temp = top_left_col;
    top_left_col = top_right_col + (y_axis - top_right_col) * 2;
    bottom_left_col = top_left_col;
    top_right_col = temp + (y_axis - temp) * 2;
    bottom_right_col = top_right_col;
    return;
}

void processRotateCW1(unsigned char *buffer_frame) {
    int net_col = top_right_col - top_left_col + 1;
    int net_row = bottom_left_row - top_left_row + 1;
    unsigned char *transpose = allocateFrame(net_row, net_col);
    for (int row = top_left_row; row <= bottom_left_row; ++row) {
        for (int col = top_left_col; col <= top_right_col; ++col) {
            int row_col = row * g_width * 3 + col * 3;
            int col_row = (col - top_left_col) * net_row * 3 + (row - top_left_row) * 3;
            // printf("COL ROW: %d\n", col_row);
            transpose[col_row] = buffer_frame[row_col];
            transpose[col_row + 1] = buffer_frame[row_col + 1];
            transpose[col_row + 2] = buffer_frame[row_col + 2];
            buffer_frame[row_col] = 255;
            buffer_frame[row_col + 1] = 255;
            buffer_frame[row_col + 2] = 255;
        }
    }
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

    for (int row = top_left_row; row <= bottom_left_row; ++row) {
        for (int col = top_left_col; col <= top_right_col; ++col) {
            int cell = row * g_width * 3 + col * 3;
            int r = row - top_left_row;
            int c = col - top_left_col;
            int tcell = r * net_row * 3 + c * 3;
            buffer_frame[cell] = transpose[tcell];
            buffer_frame[cell + 1] = transpose[tcell + 1];
            buffer_frame[cell + 2] = transpose[tcell + 2];
        }
    }
    deallocateFrame(transpose);
    processMirrorY(buffer_frame);
    // printBMP(width, height, buffer_frame);
    return;
}

void processRotateCW2(unsigned char *buffer_frame) {
    processMirrorX(buffer_frame);
    processMirrorY(buffer_frame);
    return;
}

void processRotateCW3(unsigned char *buffer_frame) {
    int net_col = top_right_col - top_left_col + 1;
    int net_row = bottom_left_row - top_left_row + 1;
    unsigned char *transpose = allocateFrame(net_row, net_col);
    for (int row = top_left_row; row <= bottom_left_row; ++row) {
        for (int col = top_left_col; col <= top_right_col; ++col) {
            int row_col = row * g_width * 3 + col * 3;
            int col_row = (col - top_left_col) * net_row * 3 + (row - top_left_row) * 3;
            // printf("COL ROW: %d\n", col_row);
            transpose[col_row] = buffer_frame[row_col];
            transpose[col_row + 1] = buffer_frame[row_col + 1];
            transpose[col_row + 2] = buffer_frame[row_col + 2];
            buffer_frame[row_col] = 255;
            buffer_frame[row_col + 1] = 255;
            buffer_frame[row_col + 2] = 255;
        }
    }
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

    for (int row = top_left_row; row <= bottom_left_row; ++row) {
        for (int col = top_left_col; col <= top_right_col; ++col) {
            int cell = row * g_width * 3 + col * 3;
            int r = row - top_left_row;
            int c = col - top_left_col;
            int tcell = r * net_row * 3 + c * 3;
            buffer_frame[cell] = transpose[tcell];
            buffer_frame[cell + 1] = transpose[tcell + 1];
            buffer_frame[cell + 2] = transpose[tcell + 2];
        }
    }
    deallocateFrame(transpose);
    processMirrorX(buffer_frame);
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
void processRotateCW(unsigned char *buffer_frame, int rotate_iteration) {
    if (rotate_iteration == 0 || rotate_iteration == 4) {
        return;
    }
    else if (rotate_iteration == 1) {
        processRotateCW1(buffer_frame);
        return;
    }
    else if (rotate_iteration == 2) {
        processRotateCW2(buffer_frame);
        return;
    }
    processRotateCW3(buffer_frame);
    return;
}
