#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "buffer.h"

#define NUM_OF_PRIMES 64 

/* HASH VALUES */	
#define h0 0x6a09e667 // 2
#define h1 0xbb67ae85 // 3
#define h2 0x3c6ef372 // 5
#define h3 0xa54ff53a // 7
#define h4 0x510e527f // 11
#define h5 0x9b05688c // 13
#define h6 0x1f83d9ab // 17
#define h7 0x5be0cd19 // 19

// binary = 1000, dec = 8
#define BIT_1 0x80
// binary = 0000, dec = 0
#define BIT_0 0x00

// macro to implement right rotation
//#define right_rotate_asm(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
 
// functions declarations
void free_easy(void *first, ...);
uint32_t *prime_arr_generator(void);
uint32_t *initialize_array_of_constants(uint32_t *primes);
void compress(uint32_t *w, uint32_t *hash, const uint32_t *K);
void process(uint8_t *processed_data, uint32_t *w);
uint8_t *padding(buf_t *buf);

#ifdef _WIN32
// Windows version (no GCC-style inline assembly)
// Use pure C version
#define right_rotate_asm(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
#else
// Linux, WSL, Mac (GCC/Clang)
static inline uint32_t right_rotate_asm(uint32_t x, uint32_t n){
    __asm__("rorl %1, %0"
            : "+r" (x)
            : "cI" (n)
            );
    return x;
}
#define RIGHT_ROTATE(x,n) right_rotate_asm((x),(n))
#endif

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "USAGE: %s <filename or string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    buf_t *buf = NULL;

    // Try to open as a file first
    struct stat st;
    if(stat(argv[1], &st) == 0 && S_ISREG(st.st_mode)){
        // Is a real file
        buf = initBuf(argv[1]);
        readFile(buf, 1); // binary mode
    }else{
        // Not a file → treat as string
        buf = buf_string(argv[1]);
    }

    if(!buf){
        fprintf(stderr, "FAILED TO INITIALIZE BUFFER\n");
        return EXIT_FAILURE;
    }

    uint8_t *data = padding(buf);
    if(!data){
        fprintf(stderr, "ERROR IN PADDING.\n");
        freeBuf(buf);
        return EXIT_FAILURE;
    }

    uint32_t hash[8] = { h0, h1, h2, h3, h4, h5, h6, h7 };
    uint32_t *primes = prime_arr_generator();
    uint32_t *K = initialize_array_of_constants(primes);

    if(!primes || !K){
        fprintf(stderr, "ERROR GENERATING CONSTANTS\n");
        freeBuf(buf);
        return EXIT_FAILURE;
    }

    for(size_t offset = 0; offset < buf->file_size; offset += 64){
        uint32_t w[64];
        process(&data[offset], w);
        compress(w, hash, K);
    }

    for(int i = 0; i < 8; ++i){
        printf("%08x", hash[i]);
    }

    printf("  %s\n", argv[1]);

    free_easy(primes, K, NULL);
    freeBuf(buf);

    return EXIT_SUCCESS;
}


// this function, frees each pointer until it finds a NULL argument
void free_easy(void *first, ...) {
    va_list args;
    va_start(args, first);

    void *ptr = first;
    while(ptr != NULL){
        free(ptr);
        ptr = va_arg(args, void *);
    }
    va_end(args);
}

// returns an array of all the first 64 prime numbers 
uint32_t *prime_arr_generator(){
    int limit = 311;
    int is_prime[limit + 1];

    for(int i = 0; i <= limit; i++){
        is_prime[i] = 1;
    }
    
    is_prime[0] = is_prime[1] = 0;

    for(int i = 2; i * i <= limit; i++){
        if(is_prime[i]){
            for(int j = i * i; j <= limit; j += i){
                is_prime[j] = 0;
            }
        }
    }

    uint32_t *primes = malloc(sizeof(uint32_t) * NUM_OF_PRIMES);
    if(primes == NULL){
        perror("malloc failed, in function prime_arr_generator");
        exit(1);
        return NULL;
    }

    int count = 0;
    for(int i = 2; i <= limit && count < NUM_OF_PRIMES; i++){
        if(is_prime[i]){
            primes[count++] = i;
        }
    }

    return primes;
}

static inline uint32_t sigma0(uint32_t x){
    return right_rotate_asm(x, 7) ^ right_rotate_asm(x, 18) ^ (x >> 3);
}

static inline uint32_t sigma1(uint32_t x){
    return right_rotate_asm(x, 17) ^ right_rotate_asm(x, 19) ^ (x >> 10);
}

// Expand message schedule
void process(uint8_t *processed_data, uint32_t *w){
    // Load the first 16 words
    for(int i = 0; i < 16; ++i){
        int j = i * 4;
        w[i] = (processed_data[j] << 24) | (processed_data[j+1] << 16) | (processed_data[j+2] << 8) | processed_data[j+3];
    }

    // Expand words 16..63
    for(int i = 16; i < 64; ++i){
        w[i] = sigma1(w[i-2]) + w[i-7] + sigma0(w[i-15]) + w[i-16];
    }
}

// compression loop
void compress(uint32_t *w, uint32_t *hash, const uint32_t *K){
    // getting my initial values
    uint32_t a = hash[0];
    uint32_t b = hash[1];
    uint32_t c = hash[2];
    uint32_t d = hash[3];
    uint32_t e = hash[4];
    uint32_t f = hash[5];
    uint32_t g = hash[6];
    uint32_t h = hash[7];

    // doing the compression
    for(int i = 0; i < 64; ++i){
        uint32_t S1 = right_rotate_asm(e,6) ^ right_rotate_asm(e,11) ^ right_rotate_asm(e,25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + K[i] + w[i];

        uint32_t S0 = right_rotate_asm(a,2) ^ right_rotate_asm(a,13) ^ right_rotate_asm(a,22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
    hash[5] += f;
    hash[6] += g;
    hash[7] += h;
}

// function to pad the file
uint8_t* padding(buf_t* buf){
    // K is the amount of 0x00 that I will have to put on the padded data
    ssize_t K = buf->file_size;
    // this is used to store the CURRENT size of the file, important because it need's to be added at the
    // last 8 bytes
    uint64_t length = buf->file_size;
    // this ensures that K is leaves "space", for the size of the file
    while(K % 64 != 56){
        K++;
    }
    // I realloc lines[0].content, so I can hold the bytes that have 0x00 and the size
    buf->lines[0].content = realloc(buf->lines[0].content, K + 8);
    // simple checking
    if(buf->lines[0].content == NULL){
        perror("REALOC Failed, in function padding");
        exit(1);
        return NULL;
    }
    // appending 0x80 at the right next available space
    buf->lines->content[buf->file_size++] = BIT_1;
    // appending K 0x00
    for(int i = buf->file_size; i < K; ++i){
        buf->lines->content[i] = BIT_0;
    }
    // this holds the length, in bytes
    uint64_t bit_len = length * 8;
    for (int i = 7; i >= 0; --i){
        // getting in the inverse, because it needs to be in small endian, the shifiting then data to it's
        // direct opposite position, then ensuring that I get only 8 bits per time
        buf->lines[0].content[K + (7 - i)] = (bit_len >> (i * 8)) & 0xFF;
    }
    // updating
    buf->file_size = K + 8;
    buf->lines[0].line_size = K + 8;
    // returning
    return (uint8_t*)buf->lines[0].content;
}

// Initialize array of round constants
uint32_t *initialize_array_of_constants(uint32_t *primes){
    if(primes == NULL){
        perror("the argument was NULL, in function initialize_array_of_conastants");
        exit(1);
        return NULL;
    }

    // each constant takes 32 bits
    uint32_t *array_of_constants = malloc(sizeof(uint32_t) * NUM_OF_PRIMES);
    if(array_of_constants == NULL){
        exit(1);
        return NULL;
    }

    // we only take the fractional part of the cubic root of the constant
    for(int i = 0; i < NUM_OF_PRIMES; ++i) {
        // base = cubic root of the prime
        double base = cbrt((double) primes[i]);
        // base minus it's own fractional part
        double fractional_part = base - floor(base);
        // meking it equal
        array_of_constants[i] = (uint32_t)(floor(fractional_part * (1ULL << 32)));
    }

    return array_of_constants;
}
