CC = mpicc
CFLAGS = -O2 -Wall -Wextra

all: mpi_scorecard

mpi_scorecard: mpi_scorecard.c
	$(CC) $(CFLAGS) -o mpi_scorecard mpi_scorecard.c

clean:
	rm -f mpi_scorecard
