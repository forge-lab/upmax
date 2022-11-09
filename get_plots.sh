#!/usr/bin/env bash
#Title			: get_plots.sh
#Usage			: bash get_plots.sh
#Author			: pmorvalho
#Date			: November 09, 2022
#Description		: Generates the plots and respective csv files using the mkplot tool
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

csvs_dir="csvs"
# output directory
plots_dir="plots"
mkdir -p $plots_dir

# gen_plot_csv(){
#   mkdir $plots_dir/$family  
#   csv_file=$plots_dir/$family/$1
#   cp $csvs_dir/$family/$algs-$parts".csv" $csv_file   
#   for ((a=0;a<${#algs[@]};a++));
#   do
#       alg=${algs[$a]}
#       for((j=0;j<${#parts[@]};j++));
#       do
# 	  part=${parts[$j]}
# 	  if [[ ($j -ge 1 && $a -ge 5) || ($j -eq 0 && $a -eq 0) ]]; # The MaxHS and UWrMaxSAT algorithms only accept wcnfs (no partitioning scheme) and we want also to ignore the first file since we copy that file to $csv_file a few lines above
# 	  then
# 	      continue
# 	  fi
# 	  output_file=$csvs_dir/$family/$alg-$part".csv"
# 	  { join -1 1 -2 1 -t,  <(head -n 1 $csv_file && tail -n +2 $csv_file | sort)  <(head -n 1 $output_file && tail -n +2 $output_file | sort); } > tmp_file
# 	  mv tmp_file $csv_file
#       done
#   done
#   sed -e 's/,/ /g' $csv_file > tmp_file # to use spaces and not commans to separate the values. mkplot only accepts spaces.
#   mv tmp_file $csv_file
# }    

gen_plot_csv(){
  mkdir $plots_dir/$family  
  csv_file=$plots_dir/$family/$1
  cp $csvs_dir/$family/$algs".csv" $csv_file   
  for ((a=1;a<${#algs[@]};a++));
  do
      alg=${algs[$a]}
      output_file=$csvs_dir/$family/$alg".csv"
      { join -1 1 -2 1 -t,  <(head -n 1 $csv_file && tail -n +2 $csv_file | sort)  <(head -n 1 $output_file && tail -n +2 $output_file | sort); } > tmp_file
      mv tmp_file $csv_file
  done
  sed -e 's/,/ /g' $csv_file > tmp_file # to use spaces and not commans to separate the values. mkplot only accepts spaces.
  mv tmp_file $csv_file
}    

# timeout used
timeout=300
# timeout=1800

# # Seating Assignment Problem
family="seating-assignment"

# Cactus plot SA
# algs=("MSU3" "WBO" "OLL" "RC2" "Hitman" "MaxHS" "UWrMaxSAT")
# parts=("wcnf" "table" "tag") # "vig" "cvig" "res" "random")
algs=("MSU3-table" "MaxHS-wcnf" "OLL-table" "RC2-tag" "MSU3-wcnf" "UWrMaxSAT-wcnf" "RC2-wcnf" "OLL-wcnf" "WBO-tag" "Hitman-tag" "Hitman-wcnf" "WBO-wcnf")
plot_name="seating-cactus"
gen_plot_csv $plot_name".csv"

python3 mkplot/mkplot.py -l -p cactus -b png --save-to $plots_dir/$family/$plot_name".png" --shape squared --ylabel="Time (s)" --xlabel="\#Instances Solved" --timeout=$timeout --lloc="outside left" --xmax=800 $plots_dir/$family/$plot_name".csv"

# Scatter plot MSU3 No Part. vs Table-based Partitions
# algs=("MSU3")
# parts=("wcnf" "table")
algs=("MSU3-wcnf" "MSU3-table")
plot_name="seating-scatter-MSU3"
gen_plot_csv $plot_name".csv"
python3 mkplot/mkplot.py -l -p scatter -b png  --ylog --xlog --save-to  $plots_dir/$family/$plot_name".png" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv"

# Scatter plot WBO No Part. vs Tag-based Partitions
# algs=("WBO")
# parts=("wcnf" "tag")
algs=("WBO-wcnf" "WBO-tag")
plot_name="seating-scatter-WBO"
gen_plot_csv $plot_name".csv"
python3 ~/mkplot/mkplot.py -l -p scatter -b png  --ylog --xlog --save-to $plots_dir/$family/$plot_name".png" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv"


# # Minimum Sum Coloring (MSC) Problem
family="minimum-sum-coloring"

# Cactus plot MSC
# algs=("MSU3" "WBO" "OLL" "RC2" "Hitman" "MaxHS" "UWrMaxSAT")
# parts=("wcnf" "res" "random" "cvig") # "vertex" "color" "vig" "cvig" "res" "random")
algs=("MaxHS-wcnf" "OLL-res" "RC2-res" "OLL-wcnf" "RC2-wcnf" "MSU3-wcnf" "MSU3-random" "WBO-res" "UWrMaxSAT-wcnf" "WBO-wcnf" "Hitman-cvig" "Hitman-wcnf")
plot_name="msc-cactus"
gen_plot_csv $plot_name".csv"
python3 mkplot/mkplot.py -l -p cactus -b pdf --save-to $plots_dir/$family/$plot_name".png" --shape squared --ylabel="Time (s)" --xlabel="\#Instances Solved" --timeout=$timeout --lloc="outside left" --xmax=1000 $plots_dir/$family/$plot_name".csv"

# Scatter plot OLL No Part. vs RES Partitioning
# algs=("OLL")
# parts=("wcng" "res")
algs=("OLL-wcnf" "OLL-res")
plot_name="msc-scatter-OLL"
gen_plot_csv $plot_name".csv"
python3 ~/mkplot/mkplot.py -l -p scatter -b png  --ylog --xlog --save-to $plots_dir/$family/$plot_name".png" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv"

# Scatter plot WBO No Part. vs RES Partitioning
# algs=("WBO")
# parts=("wcng" "res")
algs=("WBO-wcnf" "WBO-res")
plot_name="msc-scatter-WBO"
gen_plot_csv $plot_name".csv"
python3 ~/mkplot/mkplot.py -l -p scatter -b pdf  --ylog --xlog --save-to  $plots_dir/$family/$plot_name".png" --xmin=0.1 --shape squared --timeout=$timeout --lloc=" left" $plots_dir/$family/$plot_name".csv"
