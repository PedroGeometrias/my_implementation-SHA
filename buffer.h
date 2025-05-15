#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

// The struct that represents lines from the bufferi
typedef struct Line {
    // Each line has their own size
    size_t line_size;
    // Each line have their own content, which is the characters
    char *content;
    // The number that the current line represent on the buffer 
    int line_number;
    // 
} line_t;

// this will track the current position on the buffer
typedef struct Buffer_cursor{
    // current line
    size_t current_x;
    // current column
    size_t current_y;
} cursor;

// The Buffer holds the file in the memory, so it can be 
// manipulated
typedef struct Buffer {
    // A file is basically an array of lines
    line_t *lines;
    // the current x and why position on the buffer
    cursor cursor;
    // The number of lines on the buffer
    size_t line_count;
    // The capacity of the buffer, will be expanded as the buffer grows in size
    size_t capacity;
    // A string that holds the file name
    char *filename;
    // The amount of characters on the file
    size_t file_size;
} buf_t;


// Allocating memory for the buffer
buf_t *initBuf(const char *filename);
// Reading file into memory
void readFile(buf_t *buf, int option);
// Free the memory of the buffer
void freeBuf(buf_t *buf);
// Prints the buffer for debug purposes
void printFile(buf_t* buf, int option); 
// Change the name of the buffer
//void changeName(buf_t *buf, char* new_name);
// Get the current size of the buffer
size_t returnSize(buf_t *buf);
// Get the name of the buffer
char *returnName(buf_t *buf);
// get the capacity of the buffer
int returnCapacity(buf_t *buf);
// get the current amount of lines on the buffer 
int returnLines(buf_t *buf);
// Writing the new buffer
void writeFile(buf_t *buf, const char *new_name);
// updates the cursor, and returns the next character based on the current position of the cursor
char buffer_next(buf_t *buf);
// returns the next character on the file, based on the current cursor position, but it doesn't updated it's position 
char buffer_peek(buf_t *buf);
// returns the previous character on the file, based on the current cursor position, but it doesn't updated it's position 
char buffer_peek_behind(buf_t *buf);
// updates the cursor, and returns the previous character based on the current position of the cursor
char buffer_behind(buf_t *buf);
// resets the buffer to initial position
void soft_reset(buf_t *buf);
// rewinds the cursor position by an certain amount of characters
void buffer_rewind(buf_t *buf, int amount);
buf_t *buf_string(const char *data);
buf_t *buf_from_FILE(FILE *fp);
#endif
