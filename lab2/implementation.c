#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "utilities.h"  // DO NOT REMOVE this line
#include "implementation_reference.h"   // DO NOT REMOVE this line

// TODO: update logic in processRotateCW to not reallocate buffer
// TODO: update algorithm in processRotateCW to more efficient algorithm
// TODO: find a way to accumulate rotates, shifts, and mirrors. Currently the problem
// is that rotate then mirror != mirror then rotate && shift then mirror != mirror then shift
// TODO: cache the top left bottom right bounds of image of interest, right now rotating/looping through
// background which is unnecessary
void processMoveUp(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveDown(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveLeft(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);
void processMoveRight(unsigned char *buffer_frame, unsigned width, unsigned height, int offset);

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
    int cell = shift;
    for ( ; cell < width * height * 3; cell += 3) {
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
    int cell = width * height * 3 - 1 - (offset * width * 3);
    for ( ; cell >= 0; cell -= 3) {
        if (buffer_frame[cell] != 255 || buffer_frame[cell - 1] != 255 || buffer_frame[cell - 2] != 255) {
            buffer_frame[cell + shift] = buffer_frame[cell];
            buffer_frame[cell] = 255;
            buffer_frame[cell - 1 + shift] = buffer_frame[cell - 1];
            buffer_frame[cell - 1] = 255;
            buffer_frame[cell - 2 + shift] = buffer_frame[cell - 2];
            buffer_frame[cell - 2] = 255;
        }
    }
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
    int cell = shift;
    for ( ; cell < width * height * 3; cell += 3) {
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
    int cell = width * height * 3 - 1 - shift;
    for ( ; cell >= 0; cell -= 3) {
        if (buffer_frame[cell] != 255 || buffer_frame[cell - 1] != 255 || buffer_frame[cell - 2] != 255) {
            buffer_frame[cell + shift] = buffer_frame[cell];
            buffer_frame[cell] = 255;
            buffer_frame[cell - 1 + shift] = buffer_frame[cell - 1];
            buffer_frame[cell - 1] = 255;
            buffer_frame[cell - 2 + shift] = buffer_frame[cell - 2];
            buffer_frame[cell - 2] = 255;
        }
    }
}

void processRotateCW1(unsigned char *buffer_frame, unsigned width, unsigned height) {
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
    // reflect on y axis
    for (int row = 0; row < height; ++row) {
        int left = 0;
        int right = width - 1;
        int row_cell = row * width * 3;
        for(; left < right; ++left, --right) {
            int row_cell_left = row_cell + left * 3;
            int row_cell_right = row_cell + right * 3;
            unsigned char temp_1 = buffer_frame[row_cell_left];
            unsigned char temp_2 = buffer_frame[row_cell_left + 1];
            unsigned char temp_3 = buffer_frame[row_cell_left + 2];
            buffer_frame[row_cell_left] = buffer_frame[row_cell_right];
            buffer_frame[row_cell_left + 1] = buffer_frame[row_cell_right + 1];
            buffer_frame[row_cell_left + 2] = buffer_frame[row_cell_right + 2];
            buffer_frame[row_cell_right] = temp_1;
            buffer_frame[row_cell_right + 1] = temp_2;
            buffer_frame[row_cell_right + 2] = temp_3;
        }
    }
}
/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @param rotate_iteration - rotate object inside frame buffer clockwise by 90 degrees, <iteration> times
 * @return - pointer pointing a buffer storing a modified 24-bit bitmap image
 * Note: You can assume the frame will always be square and you will be rotating the entire image
 **********************************************************************************************************************/
void processRotateCW(unsigned char *buffer_frame, unsigned width, unsigned height,
                               int rotate_iteration) {
    for (int iteration = 0; iteration < rotate_iteration; iteration++) {
        processRotateCW1(buffer_frame, width, height);
    }
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorX(unsigned char *buffer_frame, unsigned int width, unsigned int height) {
    int top_row = 0;
    int bottom_row = height - 1;
    // store shifted pixels to temporary buffer
    for (; bottom_row > top_row; --bottom_row, ++top_row) {
        for (int col = 0; col < width; ++col) {
            int top_row_cell = top_row * width * 3 + col * 3;
            int bottom_row_cell = bottom_row * width * 3 + col * 3;
            unsigned char temp_1 = buffer_frame[top_row_cell];
            unsigned char temp_2 = buffer_frame[top_row_cell + 1];
            unsigned char temp_3 = buffer_frame[top_row_cell + 2];
            buffer_frame[top_row_cell] = buffer_frame[bottom_row_cell];
            buffer_frame[top_row_cell + 1] = buffer_frame[bottom_row_cell + 1];
            buffer_frame[top_row_cell + 2] = buffer_frame[bottom_row_cell + 2];
            buffer_frame[bottom_row_cell] = temp_1;
            buffer_frame[bottom_row_cell + 1] = temp_2;
            buffer_frame[bottom_row_cell + 2] = temp_3;
        }
    }
}

/***********************************************************************************************************************
 * @param buffer_frame - pointer pointing to a buffer storing the imported 24-bit bitmap image
 * @param width - width of the imported 24-bit bitmap image
 * @param height - height of the imported 24-bit bitmap image
 * @return
 **********************************************************************************************************************/
void processMirrorY(unsigned char *buffer_frame, unsigned width, unsigned height) {
    int left_col = 0;
    int right_col = width - 1;
    for (; left_col < right_col; ++left_col, --right_col) {
        for (int row = 0; row < height; ++row) {
            int left_col_cell = row * width * 3 + left_col * 3;
            int right_col_cell = row * width * 3 + right_col * 3;
            unsigned char temp_1 = buffer_frame[left_col_cell];
            unsigned char temp_2 = buffer_frame[left_col_cell + 1];
            unsigned char temp_3 = buffer_frame[left_col_cell + 2];
            buffer_frame[left_col_cell] = buffer_frame[right_col_cell];
            buffer_frame[left_col_cell + 1] = buffer_frame[right_col_cell + 1];
            buffer_frame[left_col_cell + 2] = buffer_frame[right_col_cell + 2];
            buffer_frame[right_col_cell] = temp_1;
            buffer_frame[right_col_cell + 1] = temp_2;
            buffer_frame[right_col_cell + 2] = temp_3;
        }
    }
}

/***********************************************************************************************************************
 * WARNING: Do not modify the implementation_driver and team info prototype (name, parameter, return value) !!!
 *          Do not forget to modify the team_name and team member information !!!
 **********************************************************************************************************************/
void print_team_info(){
    // Please modify this field with something interesting
    char team_name[] = "What-is-optimization";
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
    for (int sensorValueIdx = 0; sensorValueIdx < sensor_values_count; sensorValueIdx++) {
//        printf("Processing sensor value #%d: %s, %d\n", sensorValueIdx, sensor_values[sensorValueIdx].key,
//               sensor_values[sensorValueIdx].value);
        if (!strcmp(sensor_values[sensorValueIdx].key, "W")) {
            net_shift_ver += sensor_values[sensorValueIdx].value;
            processMoveUp(frame_buffer, width, height, sensor_values[sensorValueIdx].value);
        //    printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "A")) {
            // printBMP(width, height, frame_buffer);
            net_shift_hor -= sensor_values[sensorValueIdx].value;
            processMoveLeft(frame_buffer, width, height, sensor_values[sensorValueIdx].value);
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "S")) {
            // printBMP(width, height, frame_buffer);
            net_shift_ver -= sensor_values[sensorValueIdx].value;
            processMoveDown(frame_buffer, width, height, sensor_values[sensorValueIdx].value);
            // printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "D")) {
            net_shift_hor += sensor_values[sensorValueIdx].value;
            processMoveRight(frame_buffer, width, height, sensor_values[sensorValueIdx].value);
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate += sensor_values[sensorValueIdx].value;
            if (value > 0) {
                processRotateCW(frame_buffer, width, height, sensor_values[sensorValueIdx].value % 4);
            }
            else if (value < 0) {
                processRotateCW(frame_buffer, width, height, 4 - (-sensor_values[sensorValueIdx].value % 4));
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "CCW")) {
            int value = sensor_values[sensorValueIdx].value;
            net_rotate -= sensor_values[sensorValueIdx].value;
            if (value > 0) {
                processRotateCW(frame_buffer, width, height, 4 - (sensor_values[sensorValueIdx].value % 4));
            }
            else if (value < 0) {
                processRotateCW(frame_buffer, width, height, -sensor_values[sensorValueIdx].value % 4);
            }
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MX")) {
            // net_mirror_x += 1;
            // if (net_mirror_x > 1) {
            //     net_mirror_x = 0;
            // }
            processMirrorX(frame_buffer, width, height);
//            printBMP(width, height, frame_buffer);
        } else if (!strcmp(sensor_values[sensorValueIdx].key, "MY")) {
            // net_mirror_y += 1;
            // if (net_mirror_y > 1) {
            //     net_mirror_y = 0;
            // }
            processMirrorY(frame_buffer, width, height);
//            printBMP(width, height, frame_buffer);
        }
        processed_frames += 1;
        // if (processed_frames % 25 == 0) {
            // if (net_shift_hor > 0) {
            //     processMoveRight(frame_buffer, width, height, net_shift_hor);
            // }
            // else if (net_shift_hor < 0) {
            //     processMoveLeft(frame_buffer, width, height, -net_shift_hor);
            // }
            // if (net_shift_ver > 0) {
            //     processMoveUp(frame_buffer, width, height, net_shift_ver);
            // }
            // else if (net_shift_ver < 0) {
            //     processMoveDown(frame_buffer, width, height, -net_shift_ver);
            // }
            // if (net_rotate > 0) {
            //     frame_buffer = processRotateCW(frame_buffer, width, height, net_rotate % 4);
            // }
            // else if (net_rotate < 0) {
            //     frame_buffer = processRotateCW(frame_buffer, width, height, 4 - (-net_rotate % 4));
            // }
        net_shift_hor = 0;
        net_shift_ver = 0;
        net_rotate = 0;
        verifyFrame(frame_buffer, width, height, grading_mode);
        // }
    }
    return;
}
