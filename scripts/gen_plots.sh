#!/usr/bin/env bash
#Title			: gen_plots.sh
#Usage			: bash gen_plots.sh TIMEOUT_S
#Author			: pmorvalho
#Date			: November 10, 2022
#Description		: Script to generate the plots we used in our paper.
#Notes			: Uses the timeout passed as parameter ($1). This script uses mkplot, which requires matplotlib installed.
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

plots_dir="results/plots"
# timeout used
timeout=$1
# timeout=1800

# # Seating Assignment Problem
echo "Seating Assignment Problem"
family="seating-assignment"

echo "Cactus plot SA"
plot_name="seating-cactus"
python3 scripts/mkplot/mkplot.py -p cactus -b pdf --save-to $plots_dir/$family/$plot_name".pdf" --shape squared --ylabel="Time (s)" --xlabel="\#Instances Solved" --timeout=$timeout --lloc="outside left" --xmax=800 $plots_dir/$family/$plot_name".csv" 2&> /dev/null

echo "Scatter plot MSU3 No Part. vs Table-based Partitions"
plot_name="seating-scatter-MSU3"
python3 scripts/mkplot/mkplot.py -p scatter -b pdf  --ylog --xlog --save-to  $plots_dir/$family/$plot_name".pdf" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv" 2&> /dev/null

echo "Scatter plot WBO No Part. vs Tag-based Partitions"
plot_name="seating-scatter-WBO"
python3 scripts/mkplot/mkplot.py -p scatter -b pdf  --ylog --xlog --save-to $plots_dir/$family/$plot_name".pdf" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv" 2&> /dev/null

echo
echo

# # Minimum Sum Coloring (MSC) Problem
echo "Minimum Sum Coloring (MSC) Problem"
family="minimum-sum-coloring"

echo "Cactus plot MSC"
plot_name="msc-cactus"
python3 scripts/mkplot/mkplot.py -p cactus -b pdf --save-to $plots_dir/$family/$plot_name".pdf" --shape squared --ylabel="Time (s)" --xlabel="\#Instances Solved" --timeout=$timeout --lloc="outside left" --xmax=1000 $plots_dir/$family/$plot_name".csv" 2&> /dev/null

echo "Scatter plot OLL No Part. vs RES Partitioning"
plot_name="msc-scatter-OLL"
python3 scripts/mkplot/mkplot.py -p scatter -b pdf  --ylog --xlog --save-to $plots_dir/$family/$plot_name".pdf" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv" 2&> /dev/null

echo "Scatter plot WBO No Part. vs RES Partitioning"
plot_name="msc-scatter-WBO"
python3 scripts/mkplot/mkplot.py -p scatter -b pdf  --ylog --xlog --save-to  $plots_dir/$family/$plot_name".pdf" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv" 2&> /dev/null
