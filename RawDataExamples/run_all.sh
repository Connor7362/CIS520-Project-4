#!/bin/bash

set -u

PROGRAM="./mpi"
INPUT="/homes/eyv/cis520/wiki_dump.txt"
RESULTS_FILE="mpi_results.csv"

# Configs: "nodes cpus_per_task"
CONFIGS=(
  "1 1"
  "1 2"
  "1 4"
  "1 8"
  "1 16"
  "2 2"
  "2 4"
  "2 8"
  "2 16"
  "2 32"
  "4 4"
  "4 8"
  "4 16"
  "4 32"


)

MEMS=("64M" "128M" "512M" "1000M" "1500M" "3000M")

# Number of repeats per configuration
REPEATS=1

if [ ! -x "$PROGRAM" ]; then
    echo "Error: $PROGRAM not found or not executable"
    echo "Compile first with: gcc -pthread C1Pthreads.c -o C1Pthreads"
    exit 1
fi

if [ ! -f "$RESULTS_FILE" ]; then
    echo "jobid,repeat,nodes,cpus_per_task,mem_per_cpu,elapsed,state,maxrss,reqmem,avecpu,totalcpu,alloccpus,nodelist,exitcode" > "$RESULTS_FILE"
fi

echo "Starting experiment runs..."
echo "Results file: $RESULTS_FILE"

for cfg in "${CONFIGS[@]}"; do
    nodes=$(echo "$cfg" | awk '{print $1}')
    cores=$(echo "$cfg" | awk '{print $2}')

    for mem in "${MEMS[@]}"; do
        for rep in $(seq 1 "$REPEATS"); do
            echo
            echo "Submitting: nodes=$nodes cores=$cores mem=$mem repeat=$rep"

            jobid=$(sbatch --parsable --wait \
                --job-name="pth_${nodes}n_${cores}c_${mem}_r${rep}" \
                --time=10:00:00 \
                --nodes="$nodes" \
                --cpus-per-task="$cores" \
                --mem-per-cpu="$mem" \
                --constraint=moles \
                --output="slurm-%j.out" \
                --error="slurm-%j.err" \
                --wrap "cd '$PWD'; '$PROGRAM' '$INPUT'")

            rc=$?
            echo "Finished job $jobid (sbatch exit code $rc)"

            sleep 2

            sacct -j "$jobid" \
                --format=JobIDRaw,Elapsed,State,MaxRSS,ReqMem,AveCPU,TotalCPU,AllocCPUS,NodeList,ExitCode \
                --parsable2 --noheader \
            | awk -F'|' -v jobid="$jobid" -v rep="$rep" -v nodes="$nodes" -v cores="$cores" -v mem="$mem" '
                $1 == jobid {
                    printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
                           jobid, rep, nodes, cores, mem, $2, $3, $4, $5, $6, $7, $8, $9, $10
                }' >> "$RESULTS_FILE"
        done
    done
done

echo
echo "All runs complete."
echo "Saved results to $RESULTS_FILE"