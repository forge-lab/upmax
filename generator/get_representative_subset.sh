#!/usr/bin/env bash
#Title			: get_representative_subset.sh
#Usage			: bash get_representative_subset.sh
#Author			: pmorvalho
#Date			: November 07, 2022
#Description		: Gathers a representative subset of the initial dataset (10% of the instances).
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

# data_dir="/data/tmp/tacas23"
data_dir=$(pwd)
msc_main_dir=$data_dir/"evaluation_dataset/minimum-sum-coloring"
sa_main_dir=$data_dir/"evaluation_dataset/seating-assignment"

# Minimum Sum Coloring (MSC) Problem
msc_dir=$data_dir/"representative_subset/minimum-sum-coloring"

# Seating Assignment Problem
sa_dir=$data_dir/"representative_subset/seating-assignment"

n_instances=1000
## 10% of the evaluation dataset dataset, at most 26 days of computations
# for((i=1; i<=$n_instances; i=i+50))
## 2 instances (148 runs)
# for((i=1; i<=$n_instances; i=i+500))
## 1% of the evaluation dataset (740 runs), at most 3 days of computations
for((i=1; i<=$n_instances; i=i+100))
do
    ## currently we are collecting 10% of the initial dataset.
    # nums=$(shuf -i $i-$((i+49)) -n5)
    ## to collect only 2 instances, which will correspond to 148 runs
    # nums=$(shuf -i $i-$((i+499)) -n1)        
    ## currently we are collecting 1% of the initial dataset.
    nums=$(shuf -i $i-$((i+99)) -n1)
    instances=($nums)
    for((j=0;j<${#instances[@]};j++));
    do
	f=${instances[$j]}
	# MSC problem
	formulae=("wcnf" "vertex" "color" "vig" "cvig" "res" "random")
	for((k=0;k<${#formulae[@]};k++));
	do
	    formula=${formulae[$k]}
	    mkdir -p $msc_dir/$formula
	    cp $msc_main_dir/$formula/g$f-* $msc_dir/$formula/.
	done
	
	# SA problem
	formulae=("wcnf" "table" "tag" "vig" "cvig" "res" "random")
	for((l=0;l<${#formulae[@]};l++));
	do
	    formula=${formulae[$l]}
	    mkdir -p $sa_dir/$formula
	    cp $sa_main_dir/$formula/s$f-* $sa_dir/$formula/.
	done
    done
done
