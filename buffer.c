#include "buffer.h"

/*
 * Author : Pedro Haro
 * Date : 30/07/2024
 * File Description : Text Buffer, made so I can store text files in memory
 * MODIFICATION : I'm going to use this text buffer implementation on my encryption tool, making possible to encrypt file
*/

// Function to initialize the buffer and the lines with an initial capacity
buf_t *initBuf(const char *filename){
    // Allocating memory for the buffer
    buf_t *buf = (buf_t *)malloc(sizeof(buf_t));
    if(buf == NULL){
        perror("MALLOC FAILED FOR BUFFER IN FUNCTION: \"initBuf()\"\n");
        exit(1);
    }
    // The file buffer has an array of lines, here I'm allocating memory for it
    buf->lines = (line_t *)malloc(sizeof(line_t) * 1);
    if(buf->lines == NULL){
        perror("MEMORY ALLOCATION FAILED FOR LINES IN FUNCTION: \"initBuf()\"\n");
        // Case lines fail's, I free the buffer, since it should fail with the line
        free(buf);
        exit(1);
    }

    // cursor operations
    // initial positions
    buf->cursor.current_x = 0;
    buf->cursor.current_y = 0;

    //buffer has an initial amount of lines of zero, since there are no contents on it
    buf->line_count = 0;
    // initial capacity is set on the main
    buf->capacity = 1;
    // Filename of the buffer, will be the same as the one passed as argument
    buf->filename = filename ? strdup(filename) : NULL;
    if(filename && !buf->filename){
        perror("MALLOC FAILED FOR FILENAME IN FUNCTION: \"initBuf()\"\n");
        free(buf->lines);
        free(buf);
        exit(1);
    }

    buf->file_size = 0;
    return buf;
}

buf_t *buf_string(const char *data){
    size_t len = strlen(data);

    buf_t *buf = initBuf(NULL);
    if(!buf){
        fprintf(stderr, "buf_string: initBuf failed\n");
        return NULL;
    }

    // Ensure lines[0] is safe
    buf->line_count = 1;
    buf->capacity = 1;

    buf->lines[0].line_number = 1;
    buf->lines[0].line_size = len;
    buf->lines[0].content = malloc(len + 1);
    if(!buf->lines[0].content){
        perror("malloc in buf_string");
        freeBuf(buf);
        return NULL;
    }

    memcpy(buf->lines[0].content, data, len);
    buf->lines[0].content[len] = '\0';

    buf->file_size = len;
    return buf;
}

// Function for freeing the buffer
void freeBuf(buf_t* buf){
    if(buf == NULL){
        printf("NO BUFFER TO FREE\n");
        return;
    }
    // Freeing each line of the buffer
    for(size_t i = 0; i < buf->line_count; ++i){
        free(buf->lines[i].content);
    }
    // freeing allcated memory for each struct
    free(buf->filename);
    free(buf->lines);
    free(buf);
}

// Read the file into memory using getline
void readFile(buf_t* buf, int option){
    // Normal error checking
    if(buf == NULL){
        printf("NO BUFFER TO WRITE TO\n");
        return;
    }

    FILE* file = fopen(buf->filename, option == 1? "rb" : "r");
    if(file == NULL){
        perror("ERROR OPENING FILE\n");
        return;
    }

    // The place where the first character will be stored
    char *line = NULL;
    // Hold the size of the line
    size_t length = 0;
    // The characters that were read, if -1 the there are no lines to be read
    ssize_t characters_read;
    if(option){
        // from start to end
        fseek(file, 0, SEEK_END);
        // the hole size 
        size_t size = ftell(file);
        // pointer goes back to the beginning 
        rewind(file);

        // case binary mode, I allocate everything continuously
        buf->lines = realloc(buf->lines, sizeof(line_t));
        // everything on the same block
        buf->lines[0].content = malloc(size);
        // case it failedto allocate everything on the same "spot"
        if (!buf->lines[0].content) {
            perror("FAILED TO ALLOCATE MEMORY FOR BINARY CONTENT");
            fclose(file);
            exit(1);
        }
        // READING the hole spot 
        size_t  temp = fread(buf->lines[0].content, 1, size, file);
        if(temp != size){
            perror("fread failed at readFile func");
            fclose(file);
            exit(1);
        }

        // updating variables
        buf->lines[0].line_size = size;
        buf->lines[0].line_number = 1;

        buf->line_count = 1;
        buf->capacity = 1;
        buf->file_size = size; 
    }else{

        while ((characters_read = getline(&line, &length, file)) != -1) {
            // If the line counter is bigger than the capacity, allocate more memory
            if ((size_t)buf->line_count >= buf->capacity) {
                buf->capacity *= 2; 
                buf->lines = realloc(buf->lines, buf->capacity * sizeof(line_t));
                if (buf->lines == NULL) {
                    perror("REALLOC FAILED IN FUNCTION readFile(buf_t* buf)\n");
                    free(line);
                    fclose(file);
                    exit(1);
                }
            }
            // copying the line from getline to the content of the struct
            if(characters_read > 0){
                buf->lines[buf->line_count].content = strdup(line);
                if (buf->lines[buf->line_count].content == NULL) {
                    printf("MEMORY ALLOCATION FOR LINE CONTENT FAILED IN LINE %zu\n", 
                           buf->line_count); 
                    free(line);
                    fclose(file);
                    exit(1);
                }
            }else{
                printf("FILE IS EMPTY, OR MALFORMED\n");
                exit(1);
            }
            // updating line size
            buf->lines[buf->line_count].line_size = characters_read;
            // updating the amount of lines for each iteration
            buf->lines[buf->line_count].line_number = buf->line_count + 1;
            buf->line_count++;
            // updating the amount of characters
            buf->file_size += characters_read;
        }
    }

    free(line);
    fclose(file);
}

// Debug purpose - printing the contents of the buffer
void printFile(buf_t* buf, int option){
    if(buf == NULL){
        printf("NO BUFFER TO PRINT\n");
        return;
    }
    // if option is 0, going to print without numbers
    if(option==0){
        for(size_t i = 0; i < buf->line_count; ++i) {
            printf("%s", buf->lines[i].content);
        }
    }else if(option==1){
        for(size_t i = 0; i < buf->line_count; ++i) {
            printf("%d  %s", buf->lines[i].line_number, buf->lines[i].content);
        }
    }
}

// function to write from the buffer in memory to a file
void writeFile(buf_t *buf, const char *new_name) {
    // new file to be written
    FILE *new_file = fopen(new_name, "wb");
    if(!new_file){
        perror("New File wasn't created");
        return;
    }

    // writing
    for(size_t i = 0; i < buf->line_count; ++i){
        line_t *line = &buf->lines[i];
        fwrite(line->content, 1, line->line_size, new_file);
    }
    fclose(new_file);
}

// returning the amount of lines on the buffer
int returnLines(buf_t *buf){
    return buf->line_count;
}

// returning the current capacity of the file
int returnCapacity(buf_t *buf){
    return buf->capacity;
}

// returning the name of the current buffer
char *returnName(buf_t *buf){
    return buf->filename;
}

// return the current amount of characters on the buffer
size_t returnSize(buf_t *buf){
    return buf->file_size;
}

// change the name of the buffer
/* void changeName(buf_t *buf, char* new_name){
    strcpy(buf->filename, new_name);
} */

char buffer_next(buf_t *buf){
    // in case buffer doesn't exist, or the cursor doesn't exist, or the amount of columns are larger the the line counter
    if(!buf || buf->cursor.current_y >= buf->line_count){
        return '\0';
    }

    // pointer to the current line buffer
    line_t *line = &buf->lines[buf->cursor.current_y];

    // in case the cursor reaches the end of the line
    if(buf->cursor.current_x >= line->line_size){
        // y position gets updated
        buf->cursor.current_y++;
        // we go to the start of the line
        buf->cursor.current_x = 0;

        // check if the line has hit the end of the buffer 
        if(buf->cursor.current_y >= buf->line_count){
            return '\0';
        }
        // updating the helper variable
        line = &buf->lines[buf->cursor.current_y];
    }

    // get the current character
    char ch = line->content[buf->cursor.current_x];
    // move the cursor 
    buf->cursor.current_x++;
    return ch;
}

// this returns the next char, without updating the current position on the buffer
char buffer_peek(buf_t *buf) {
    // checking if the buffer exists
    if(!buf){
        return '\0';
    }

    // not effecting the position, just using temporary variables for the reason
    size_t temp_x = buf->cursor.current_x;
    size_t temp_y = buf->cursor.current_y;

    // checking if buffer still has lines
    if(temp_y >= buf->line_count){
        return '\0';
    }

    // simplifying the use of the structure
    line_t *line = &buf->lines[temp_y];

    // in case the line ended
    if(temp_x >= line->line_size){
        // updates the current line posi
        temp_y++;
        // going to the start of the line
        temp_x = 0;
        // if I raech the end of the buffer
        if(temp_y >= buf->line_count){
            return '\0';
        }
        // updating the line to new positions
        line = &buf->lines[temp_y];
    }

    // returning desired char
    return line->content[temp_x++];
}


// this returns the next char, without updating the current position on the buffer
char buffer_peek_behind(buf_t *buf) {
    if(!buf){
        return '\0';
    }

    size_t temp_x = buf->cursor.current_x;
    size_t temp_y = buf->cursor.current_y;

    if(temp_y == 0 && temp_x == 0){
        return '\0';
    }

    if(temp_y >= buf->line_count){
        return '\0';
    }

    line_t *line = &buf->lines[temp_y];

    if(temp_x > 0 && temp_y > 0){
        return line->content[temp_x - 1];
    }

    return '\0';
}

