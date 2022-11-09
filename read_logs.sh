#!/usr/bin/env bash
#Title			: read_logs.sh
#Usage			: bash read_logs.sh
#Author			: pmorvalho
#Date			: November 08, 2022
#Description		: Processes all the logs and saves a csv file for each pair algorithm-partition_scheme
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

results_dir="logs"
csvs_dir="csvs"
timeout=300
# timeout=1800
algs=("MSU3" "WBO" "OLL" "RC2" "Hitman" "MaxHS" "UWrMaxSAT")

save_results(){
  mkdir -p $csvs_dir/$family/
  for((j=0;j<${#parts[@]};j++));
  do
      part=${parts[$j]}
      for ((a=0;a<${#algs[@]};a++));
      do
	alg=${algs[$a]}
	if [[ $j -ge 0 && $a -ge 5 ]]; # The MaxHS and UWrMaxSAT algorithms only accept wcnfs (no partitioning scheme)
	then
	   continue
        fi  
	output_file=$csvs_dir/$family/$alg-$part".csv"
	rm $output_file
	echo "Benchmark,"$alg-$part > $output_file
	for f in $(find $results_dir/$family/$part/$alg/*.out -type f);
	do
	    i_name="$(basename $f .out)"
	    y=$(grep OPTIMUM $f)
	    t=$timeout
	    if [[ $y != "" ]] ; then
		t=$(grep "CPUTIME=" $results_dir/$family/$part/$alg/$i_name.var | cut -d '=' -f 2)
	    fi
	    echo $i_name","$t >> $output_file
	done
	if [[ `wc -l $output_file | awk '{print $1}'` -le "1" ]];
	then
	    rm $output_file
	fi
      done
  done	
}    

parts=("wcnf" "table" "tag" "vig" "cvig" "res" "random")
family="seating-assignment"
save_results

parts=("wcnf" "vertex" "color" "vig" "cvig" "res" "random")
family="minimum-sum-coloring"
save_results
