#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define NUM_THREADS  4


#define MAX_LINE_LEN 4096


#define BATCH_SIZE   1000 // # of lines read in at a time --> think about changing this to just read in 1MB at a time
#define READ_BUF_SIZE (1 << 20) // 1 MB


const char* filePath = "/homes/eyv/cis520/wiki_dump.txt";


static char lines[BATCH_SIZE][MAX_LINE_LEN]; // part of read in file
static int  results[BATCH_SIZE];
static int  total_lines = 0; // amount of lines loaded into lines


// this sets the bounds for each thread where it starts and stops when reading each chunk
typedef struct {
    int start; // inclusive
    int end;   // exclusive
} ThreadArgs;


// finds the greatest ascii value in each line
static void* compute_max(void* arg) {
    ThreadArgs* a = (ThreadArgs*)arg;
    for (int i = a->start; i < a->end; i++) {
        unsigned char max_val = 0;
        for (int j = 0; lines[i][j] != '\0'; j++) {
            unsigned char c = (unsigned char)lines[i][j];
            if (c > max_val) max_val = c;
        }
        results[i] = (int)max_val;
    }
    return NULL;
}


// spin up threads on the current batch, wait then print
static void process_and_print(int global_offset) {
    pthread_t  threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];


    int base = total_lines / NUM_THREADS;
    int rem  = total_lines % NUM_THREADS;
    int start = 0;


    // creates threads and sets them to use compute_max
    for (int t = 0; t < NUM_THREADS; t++) {
        int extra = (t < rem) ? 1 : 0;
        args[t].start = start;
        args[t].end   = start + base + extra;
        start = args[t].end;


        if (pthread_create(&threads[t], NULL, compute_max, &args[t]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }


    // waits for all threadses to finish
    for (int t = 0; t < NUM_THREADS; t++)
        pthread_join(threads[t], NULL);


    // prints results
    for (int i = 0; i < total_lines; i++)
        printf("%d: %d\n", global_offset + i, results[i]);
}


int main(void) {


    // opens file
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }


    static char read_buf[READ_BUF_SIZE];
    static char leftover[MAX_LINE_LEN]; // this is for when read does not return a complete line


    int leftover_len  = 0; // length of leftover line in chars
    int global_offset = 0; // offset for current line number


    ssize_t n; // number of bytes read
    while ((n = read(fd, read_buf, READ_BUF_SIZE)) > 0) {
        char *ptr = read_buf;
        char *end = read_buf + n;


        while (ptr < end) {


            // find the next newline within this read
            char *nl = memchr(ptr, '\n', (size_t)(end - ptr));


            if (nl == NULL) {
                // rest of buffer is a partial line
                size_t rem = (size_t)(end - ptr);


                size_t space_left = (MAX_LINE_LEN - 1) - (size_t)leftover_len;
                size_t to_copy = (rem < space_left) ? rem : space_left;


                memcpy(leftover + leftover_len, ptr, to_copy);
                leftover_len += (int)to_copy;
                break;
            }


            size_t seg_len = (size_t)(nl - ptr);
            size_t copied = 0;


           
            if (total_lines >= BATCH_SIZE) {
                process_and_print(global_offset);
                global_offset += total_lines;
                total_lines = 0;
            }


            //copies the leftover lines to the current line
            memcpy(lines[total_lines], leftover, (size_t)leftover_len);
            copied = (size_t)leftover_len;


            size_t space_left = (MAX_LINE_LEN - 1) - copied;
            size_t to_copy = (seg_len < space_left) ? seg_len : space_left;


            memcpy(lines[total_lines] + copied, ptr, to_copy);
            copied += to_copy;
            lines[total_lines][copied] = '\0';


            //reset variables
            leftover_len = 0;
            total_lines++;
            ptr = nl + 1;


            // print and process everything
            if (total_lines == BATCH_SIZE) {
                process_and_print(global_offset);
                global_offset += total_lines;
                total_lines = 0;
            }
        }
    }


   
    if (n < 0 && errno != EINTR)
        perror("read");


    // flush any partial last line
    if (leftover_len > 0) {
        leftover[leftover_len] = '\0';
        memcpy(lines[total_lines], leftover, (size_t)leftover_len + 1);
        total_lines++;
    }


    if (total_lines > 0)
        process_and_print(global_offset);


    close(fd);
    return EXIT_SUCCESS;
}

