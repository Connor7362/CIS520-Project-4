# CIS520-Project-4
Project 4 Repo for CIS520

## MPI Architecture overview 

The MPI implementation uses a distributed-memory model. Instead of threads sharing a single address space, multiple independent processes—called ranks—are launched, each with its own memory. Every rank opens the same input file using low‑level system calls (open(), read(), close()) and scans it in chunks. As each rank reads through the file, it keeps track of the current line number and the largest ASCII value on that line. Ownership of a line is determined by the expression line_number % num_ranks; for example, with four ranks, rank 0 handles lines 0, 4, 8… and rank 1 handles lines 1, 5, 9…, and so on. Each rank stores results only for its assigned lines. When all ranks have finished processing, the results are gathered back to rank 0. Rank 0 then sorts the line‑number/max‑ASCII pairs and prints the final output in the correct order. 

Because each rank has its own memory space, no locks are required to protect shared data. Synchronization occurs only during the final gather and sort phases, which combine the distributed results into a single, ordered list for printing. This design avoids shared-memory race conditions but introduces overhead when multiple ranks must each scan the entire file and then communicate their results. 

## Running MPI
To run the MPI version, you usually choose between the provided Slurm script and a manual execution for small tests. On Beocat, you would go into the 3way-mpi folder and submit submit_mpi.sh with sbatch. The script takes care of loading the compiler and MPI modules, building the program, and launching it via mpirun; Slurm writes the output and error messages into log files named after the job ID. For local testing or debugging with a small input file, you can run the program yourself: first load the required modules (including OpenMPI) using the module load command, then compile the code with make clean && make, and finally start it with a command like mpirun -np 4 ./mpi_scorecard /path/to/small_input.txt, adjusting the number of processes and the input file name as needed. It’s important not to run the full 1.7 GB wiki file directly on a login node; use the Slurm script for those larger runs. 

## OPENMP Architecture overview
The OpenMP (hereafter referred to as OMP) implementation follows a shared-memory parallelism model, where a single process spawns multiple threads that operate within the same address space. OMP relies on lightweight threads that can directly access shared data structures. The OMP program processes data by first opening its respective file (in this case, the text file provided for this project) and reads the file in 10MB chunks to avoid an out of memory error. This is stored into a dynamically rezied array, which is accessible to every thread while avoiding duplication.

The key function of OMP is parallelism, which is brought into the project using the
#pragma omp parallel for 
line of code. This distributes the workload across multiple threads, where one thread receives a single line from the text file. After the work for this is finished, the memory is freed so that the next portion of the text file can be read.

## RUNNING OPENMP
To run the OpenMP tests, the user must open the directory where the OpenMP (interchangeable with OMP) portion of this project exists. Within this directory exists the Makefile for compiling the OpenMP_scorecard.c file, a submit_omp.sh shell script file, and a run_all_openmp.sh file. Running the run_all_openmp.sh file allows the user to simply run all of the settings used for gathering the data for the included report, which is done so by using the following command: ./run_all_openmp.sh
Running the other shell script file, submit_omp.sh, allows the user to specify what parameters are to be used for a specific test. This shell script file is to be ran using the following command: sbatch --constraint=moles --cpus-per-task=4 --mem-per-cpu=1G with the last two parameters allowing for customization.
