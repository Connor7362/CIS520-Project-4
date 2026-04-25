#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 65536

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

        if (c == '\n' || c == '\r') {
            continue;
        }

        if ((int)c > max) {
            max = (int)c;
        }
    }

    return max;
}

int main(int argc, char *argv[]) {
    int rank;
    int size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        }

        MPI_Finalize();
        return 1;
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);

    if(fd < 0){
	fprintf(stderr, "Rank %d could not open file: %s\n", rank, filename);
	MPI_Finalize();
	return 1;
    }

    Result *local_results = NULL;
    int local_count = 0;
    int local_capacity = 1024;

    local_results = malloc(local_capacity * sizeof(Result));

    if (local_results == NULL) {
        fprintf(stderr, "Rank %d failed to allocate memory\n", rank);
        close(fd);
        MPI_Finalize();
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    long line_number = 0;
    int current_max = 0;
    int has_chars = 0;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            unsigned char c = (unsigned char)buffer[i];

            if (c == '\n') {
                if (line_number % size == rank) {
                    if (local_count == local_capacity) {
                        local_capacity *= 2;
                        Result *temp = realloc(local_results, local_capacity * sizeof(Result));

                        if (temp == NULL) {
                            fprintf(stderr, "Rank %d failed to grow memory\n", rank);
                            free(local_results);
                            close(fd);
                            MPI_Finalize();
                            return 1;
                        }
 
                        local_results = temp;
                    }

                    local_results[local_count].line_number = line_number;
                    local_results[local_count].max_ascii = current_max;
                    local_count++;
                }

                line_number++;
                current_max = 0;
                has_chars = 0;
            } else if (c != '\r') {
                has_chars = 1;

                if ((int)c > current_max) {
                    current_max = (int)c;
                }
            }
        }
    }

    if (bytes_read < 0) {
        fprintf(stderr, "Rank %d had an error while reading file\n", rank);
        free(local_results);
        close(fd);
        MPI_Finalize();
        return 1;
    }

    if (has_chars) {
        if (line_number % size == rank) {
            if (local_count == local_capacity) {
                local_capacity *= 2;
                Result *temp = realloc(local_results, local_capacity * sizeof(Result));

                if (temp == NULL) {
                    fprintf(stderr, "Rank %d failed to grow memory\n", rank);
                    free(local_results);
                    close(fd);
                    MPI_Finalize();
                    return 1;
                }

                local_results = temp;
           }

            local_results[local_count].line_number = line_number;
            local_results[local_count].max_ascii = current_max;
            local_count++;
        }
    }

    close(fd);
    int *counts = NULL;

    if (rank == 0) {
        counts = malloc(size * sizeof(int));
    }

    MPI_Gather(&local_count, 1, MPI_INT, counts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *displacements = NULL;
    int total_count = 0;

    if (rank == 0) {
        displacements = malloc(size * sizeof(int));

        for (int i = 0; i < size; i++) {
            displacements[i] = total_count;
            total_count += counts[i];
        }
    }

    long *local_line_numbers = malloc(local_count * sizeof(long));
    int *local_max_values = malloc(local_count * sizeof(int));

    for (int i = 0; i < local_count; i++) {
        local_line_numbers[i] = local_results[i].line_number;
        local_max_values[i] = local_results[i].max_ascii;
    }

    long *all_line_numbers = NULL;
    int *all_max_values = NULL;

    if (rank == 0) {
        all_line_numbers = malloc(total_count * sizeof(long));
        all_max_values = malloc(total_count * sizeof(int));
    }

    MPI_Gatherv(
        local_line_numbers,
        local_count,
        MPI_LONG,
        all_line_numbers,
        counts,
        displacements,
        MPI_LONG,
        0,
        MPI_COMM_WORLD
    );

    MPI_Gatherv(
        local_max_values,
        local_count,
        MPI_INT,
        all_max_values,
        counts,
        displacements,
        MPI_INT,
        0,
        MPI_COMM_WORLD
    );

    if (rank == 0) {
        Result *all_results = malloc(total_count * sizeof(Result));

        for (int i = 0; i < total_count; i++) {
            all_results[i].line_number = all_line_numbers[i];
            all_results[i].max_ascii = all_max_values[i];
        }

        qsort(all_results, total_count, sizeof(Result), compare_results);

        for (int i = 0; i < total_count; i++) {
            printf("%ld: %d\n", all_results[i].line_number, all_results[i].max_ascii);
        }

        free(all_results);
    }

    free(local_results);
    free(local_line_numbers);
    free(local_max_values);

    if (rank == 0) {
        free(counts);
        free(displacements);
        free(all_line_numbers);
        free(all_max_values);
    }

    MPI_Finalize();
    return 0;
}
