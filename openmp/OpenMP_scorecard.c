#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

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

    //Read Lines into memory
    size_t capacity = 1024;
    size_t line_count = 0;
    char **lines = malloc(capacity * sizeof(char*));

    if (!lines) {
        perror("malloc failed");
        fclose(fp);
        return 1;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) != -1) {
        if (line_count == capacity) {
            capacity *= 2;
            char **tmp = realloc(lines, capacity * sizeof(char*));
            if (!tmp) {
                perror("realloc failed");
                return 1;
            }
            lines = tmp;
        }

        lines[line_count] = strdup(line);
        line_count++;
    }

    free(line);
    fclose(fp);

    //Allocate Results
    Result *results = malloc(line_count * sizeof(Result));
    if (!results) {
        perror("malloc failed");
        return 1;
    }

    //Parallel Processing
    #pragma omp parallel for
    for (long i = 0; i < line_count; i++) {
        results[i].line_number = i;
        results[i].max_ascii = max_ascii_in_line(lines[i]);
    }

    //Sort (is this needed? Should already be sorted)
    qsort(results, line_count, sizeof(Result), compare_results);

    //Output
    for (size_t i = 0; i < line_count; i++) {
        printf("%ld: %d\n", results[i].line_number, results[i].max_ascii);
    }

    //Freeing up lines
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }

    free(lines);
    free(results);

    return 0;
}