#!/usr/bin/env bash
#Title			: get_tables.sh
#Usage			: bash get_tables.sh TIMEOUT_S
#Author			: pmorvalho
#Date			: November 09, 2022
#Description		: Prints and save the tables used in our paper. Number of solved instances for each pair Algorithm x Partition_Scheme.
#Notes			: Uses the timeout passed as parameter ($1)
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

results_dir="results/logs"
csvs_dir="results/csvs"
timeout=$1
# timeout=1800

# output directory
tables_dir="results/tables"
mkdir -p $tables_dir

algs=("MSU3" "WBO" "OLL" "RC2" "Hitman" "MaxHS" "UWrMaxSAT")

print_table(){
  for ((a=0;a<${#algs[@]};a++));
  do
      alg=${algs[$a]}
      line=$alg
      for((j=0;j<${#parts[@]};j++));
      do
	  part=${parts[$j]}
	  output_file=$csvs_dir/$family/$alg-$part".csv"
	  num_instances_solved="-"
	  if [ -f "$output_file" ]; then
	      # number of timeouts
	      num_timeouts=$(grep ,$timeout $output_file | wc -l)
	      # number of instances not solved (num_timeouts + 1 - the header of the csv file)
	      not_solved=$((num_timeouts+1))
	      num_lines=$(cat $output_file | wc -l)
	      num_instances_solved=$((num_lines-not_solved))
	  fi
	  line=$line","$num_instances_solved
      done
      echo $line | tee -a $table
  done	
}    

table=$tables_dir/"seating-assignment.csv"
if [ -f "$table" ]; then	 
    rm $table
fi
echo
echo "Seating Assignment Problem"
echo
echo ",,,,Seating Assignment,,," | tee -a $table
echo ",,User Part,,,Graph Part.,," | tee -a $table
echo "Solver,No Part.,Table,Tag,VIG,CVIG,RES,Random" | tee -a $table
parts=("wcnf" "table" "tag" "vig" "cvig" "res" "random")
family="seating-assignment"
print_table

echo
echo
echo

table=$tables_dir/"minimum-sum-coloring.csv"
if [ -f "$table" ]; then	 
    rm $table
fi
echo
echo
echo "Minimum Sum Coloring (MSC) Problem"
echo
echo ",,,,Minimum Sum Coloring,,," | tee -a $table
echo ",,User Part,,,Graph Part.,," | tee -a $table
echo "Solver,No Part.,Vertex,Color,VIG,CVIG,RES,Random" | tee -a $table
parts=("wcnf" "vertex" "color" "vig" "cvig" "res" "random")
family="minimum-sum-coloring"
print_table
