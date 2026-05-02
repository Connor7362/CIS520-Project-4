#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#define CHUNK_SIZE (10 * 1024 * 1024) // 10 MB


typedef struct {
    long line_number;
    int max_ascii;
} Result;

int compare_results(const void *a, const void *b) {
    const Result *ra = (const Result *)a;
    const Result *rb = (const Result *)b;

    if (ra->line_number < rb->line_number) return -1;
    if (ra->line_number > rb->line_number) return 1;
    return 0;
}

int max_ascii_in_line(const char *line) {
    int max = 0;

    for (int i = 0; line[i] != '\0'; i++) {
        unsigned char c = (unsigned char)line[i];

        if (c == '\n' || c == '\r') continue;

        if ((int)c > max) {
            max = (int)c;
        }
    }

    return max;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");

    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    //Allows for only 10MB at a time
    char *buffer = malloc(CHUNK_SIZE + 1);
    if (!buffer) {
        perror("malloc failed");
        return 1;
    }
    long global_line_number = 0;


    while(!feof(fp)) {
        size_t bytes_read = fread(buffer, 1, CHUNK_SIZE, fp);
        if (bytes_read == 0) break;
        
        buffer[bytes_read] = '\0';

        //Read Lines into memory
        size_t capacity = 1024;
        size_t line_count = 0;
        char **lines = malloc(capacity * sizeof(char*));

        if (!lines) {
        perror("malloc failed");
        fclose(fp);
        return 1;
        }

        //Splits buffer into lines
        char *start = buffer;
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';

                if (line_count == capacity) {
                    capacity *= 2;
                    char **tmp = realloc(lines, capacity * sizeof(char*));
                    if (!tmp) {
                        perror("realloc failed");
                        return 1;
                    }
                    lines = tmp;
                }

                lines[line_count++] = start;
                start = &buffer[i + 1];
            }
        }
        //Allocate
        Result *results = malloc(line_count * sizeof(Result));
        if (!results) {
            perror("malloc failed");
            return 1;
        }

        //Parallel Processing
        #pragma omp parallel for
        for (long i = 0; i < line_count; i++) {
            results[i].line_number = global_line_number + i;
            results[i].max_ascii = max_ascii_in_line(lines[i]);
        }


        //Output
        for (size_t i = 0; i < line_count; i++) {
            printf("%ld: %d\n", results[i].line_number, results[i].max_ascii);
        }

        global_line_number += line_count;

        free(lines);
        free(results);
    }
    
    free(buffer);
    fclose(fp);

    return 0;
}