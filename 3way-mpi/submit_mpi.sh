#!/bin/bash
#SBATCH --job-name=hw4_mpi
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --mem-per-cpu=1G
#SBATCH --time=00:30:00
#SBATCH --output=mpi_%j.out
#SBATCH --error=mpi_%j.err

module load CMake/3.23.1-GCCcore-11.3.0 foss/2022a OpenMPI/4.1.4-GCC-11.3.0

make clean >&2
make >&2

mpirun -np 4 ./mpi_scorecard ~eyv/cis520/wiki_dump.txt
