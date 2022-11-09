#!/usr/bin/env bash
#Title			: run_all.sh
#Usage			: bash run_all.sh
#Author			: pmorvalho
#Date			: November 08, 2022
#Description		: Recreates our experiments step by step from running the solvers to getting the results.
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

echo "Running configuration script to unzip data and install the PySAT package"
chmod +x config.sh
./config.sh
echo

# In order to run the solvers on the entire evaluation dataset the following script run_solvers must be modified.
echo "Running solvers on the representative subset of our dataset"
./run_solvers.sh
echo
echo "All the logs are stored in 'logs/' directory"

echo "Processing the logs..."
./read_logs.sh
echo
echo "Csv files with the resuts save in csvs/ directory"

echo "Printing tables (Table 1 and Table 2)"
./get_tables.sh
echo
echo "Tables saved sucessfully in tables/ directory"

echo "Getting the plots and their csv files"
./get_plots.sh
echo
echo

echo "All done."
