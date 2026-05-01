CC = gcc
CFLAGS = -O2 -Wall -Wextra -fopenmp

all: OpenMP_scorecard

OpenMP_scorecard: OpenMP_scorecard.c
	$(CC) $(CFLAGS) -o OpenMP_scorecard OpenMP_scorecard.c

clean:
	rm -f OpenMP_scorecard