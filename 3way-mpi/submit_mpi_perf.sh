#!/bin/bash
#SBATCH --job-name=mpi_perf
#SBATCH --nodes=1
#SBATCH --time=00:30:00
#SBATCH --output=mpi_perf_%j.out
#SBATCH --error=mpi_perf_%j.err

module load CMake/3.23.1-GCCcore-11.3.0 foss/2022a OpenMPI/4.1.4-GCC-11.3.0

make clean >&2
make >&2

echo "Job ID: $SLURM_JOB_ID" >&2
echo "Tasks: $SLURM_NTASKS" >&2
echo "Memory per CPU: $SLURM_MEM_PER_CPU MB" >&2

/usr/bin/time -v -o mpi_perf_${SLURM_JOB_ID}.time \
mpirun -np $SLURM_NTASKS ./mpi_scorecard ~eyv/cis520/wiki_dump.txt > /dev/null
