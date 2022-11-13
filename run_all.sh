#!/usr/bin/env bash
#Title			: run_all.sh
#Usage			: bash run_all.sh
#Author			: pmorvalho
#Date			: November 08, 2022
#Description		: Recreates our experiments step by step from running the solvers to getting the results.
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

# echo "Running configuration script to unzip data and install the PySAT package"
# chmod +x scripts/config.sh
# ./scripts/config.sh
# echo

dataset=$1
timeout=$2
if [[ $dataset == "" ]];
then
    dataset="representative_subset_10_instances"
fi
if [[ $timeout == "" ]];
then
    timeout=180
fi

echo "Starting..."
echo "Using dataset "$dataset" with timeout of "$timeout" seconds!"
echo

# In order to run the solvers on the entire evaluation dataset the following script run_solvers must be modified.
echo "Running solvers on the representative subset of our dataset"
./scripts/run_solvers.sh $dataset $timeout
echo
echo "All the logs are stored in 'results/logs/' directory"

echo "Processing the logs..."
./scripts/read_logs.sh $timeout
echo
echo "Csv files with the resuts save in results/csvs/ directory"

echo "Printing tables (Table 1 and Table 2)"
./scripts/get_tables.sh $timeout
echo "Tables saved sucessfully in results/tables/ directory"
echo

echo "Getting the csv files for the plots (results/plots directory)"
./scripts/get_plots.sh
echo
echo

## The following script requires an existing installation of matplotlib.
echo "Getting the plots..."
./scripts/gen_plots.sh $timeout
echo
echo

echo "All done."
