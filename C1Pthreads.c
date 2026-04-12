#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define NUM_THREADS 4

#define MAX_LINE_LEN 65536

const char* filePath = "/homes/eyv/cis520/wiki_dump.txt";


static char (*lines)[MAX_LINE_LEN] = NULL; // stored file
static int  *results               = NULL; // greatest ascii value for each line
static int   total_lines           = 0;    // total lines

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

int main(void) {

    // OPens the file 
    int fd = open(filePath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }
    // stat struct to get file meta data
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    // gets the file size if 0 exits the function
    size_t file_size = (size_t)file_stat.st_size;
    if (file_size == 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    // allocates buffer for file size + 1 for null terminator 
    char* buf = malloc(file_size + 1);
    if (buf == NULL) {
        perror("malloc");
        close(fd);
        return EXIT_FAILURE;
    }

    // Loops until all bytes in the file are read into the buffer
    size_t total_bytes_read = 0;
    while (total_bytes_read < file_size) {
        ssize_t bytes_read = read(fd, buf + total_bytes_read, file_size - total_bytes_read);
        // Bytes are read
        if (bytes_read > 0)
            total_bytes_read += bytes_read;
        else if (bytes_read == 0) // reached EOF early so exit 
        {
            break;
        }
        else if (bytes_read == -1 && errno == EINTR) // signal interrupt did not error so try again
            continue;
        else
            break;
    }

    // Closes the file and makes sure all bytes were read 
    if (close(fd) < 0 || total_bytes_read < file_size) {
        free(buf);
        return EXIT_FAILURE;
    }

    buf[total_bytes_read] = '\0'; // sets null terminator into end of buffer

    // Pass 1: count newlines to know how many lines to allocate
    int line_count = 0;
    for (size_t i = 0; i < total_bytes_read; i++)
        if (buf[i] == '\n') line_count++;
    if (total_bytes_read > 0 && buf[total_bytes_read - 1] != '\n')
        line_count++;

    // Allocate exactly what we need
    lines   = malloc((size_t)line_count * sizeof(*lines));
    results = malloc((size_t)line_count * sizeof(int));
    if (lines == NULL || results == NULL) {
        perror("malloc");
        free(buf);
        return EXIT_FAILURE;
    }

    // Parse buffer into individual lines
    char* line_start = buf;
    char* ptr = buf;
    while (*ptr != '\0') {
        if (*ptr == '\n') {
            *ptr = '\0';
            strncpy(lines[total_lines], line_start, MAX_LINE_LEN - 1);
            lines[total_lines][MAX_LINE_LEN - 1] = '\0';
            total_lines++;
            line_start = ptr + 1;
        }
        ptr++;
    }

    // the check for if the last char is not a new line 
    if (ptr != line_start) {
        strncpy(lines[total_lines], line_start, MAX_LINE_LEN - 1);
        lines[total_lines][MAX_LINE_LEN - 1] = '\0';
        total_lines++;
    }

    free(buf); // frees the buffer 

    // creates threads 
    pthread_t  threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];

    int chunk = total_lines / NUM_THREADS; // splits into how many chunks depending on how many threads 


    for (int t = 0; t < NUM_THREADS; t++) {
        args[t].start = t * chunk;
        args[t].end   = (t == NUM_THREADS - 1) ? total_lines : (t + 1) * chunk;
        if (pthread_create(&threads[t], NULL, compute_max, &args[t]) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    // waits for all threads to finish
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // prints the results of the lines
    for (int i = 0; i < total_lines; i++) {
        printf("%d: %d\n", i, results[i]);
    }

    free(lines);
    free(results);
    return EXIT_SUCCESS;
}
