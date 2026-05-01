#!/bin/bash
#SBATCH --job-name=omp_perf
#SBATCH --ntasks=1
#SBATCH --nodes=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=1G
#SBATCH --time=00:30:00
#SBATCH --output=omp_perf_%j.out
#SBATCH --error=omp_perf_%j.err

module load GCC/11.3.0

make clean >&2
make >&2

echo "Job ID: $SLURM_JOB_ID" >&2
echo "CPUs per task (threads): $SLURM_CPUS_PER_TASK" >&2
echo "Memory per CPU: $SLURM_MEM_PER_CPU MB" >&2

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

export OMP_PROC_BIND=true
export OMP_PLACES=cores

/usr/bin/time -v -o omp_perf_${SLURM_JOB_ID}.time \
./OpenMP_scorecard ~eyv/cis520/wiki_dump.txt > output_${SLURM_JOB_ID}.txt