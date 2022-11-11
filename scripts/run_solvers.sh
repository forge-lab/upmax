#!/usr/bin/env bash
#Title			: run_solvers.sh
#Usage			: bash run_solvers.sh DATASET TIMEOUT_S
#Author			: pmorvalho
#Date			: November 08, 2022
#Description		: Runs all solvers with the representative subset of our evaluation dataset (passed as $1), and with a timeout of $2.
#Notes			: Uses the dataset passed as parameter $1 and the timeout ($2). For the artifact evaluation of TACAS 2023.
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

# To use only the representative subset of our evaluation dataset (100 instances) and a timeout of 300s
dataset=$1
wl=$2
mem=8000

## Uncoment below in order to use our entire evaluation dataset and the timeout used in our paper (1800s)
# dataset="evaluation_dataset"
# wl=1800
# mem=32768

run_solvers_with_UP(){
  # Solvers that accept user-based partitions (using pwcnfs)
  for((j=0;j<${#formulae[@]};j++));
    do
	part=${formulae[$j]}
	formulae_dir=$data_dir/$part
	output_dir=$results_dir/$part
	mkdir -p $output_dir"/MSU3/" $output_dir"/WBO/" $output_dir"/OLL/" $output_dir"/RC2/" $output_dir"/Hitman/"
	for f in $(find $formulae_dir/*z -type f);
	do
	    echo "Instance: "$f  
	    i_name="$(basename $f .pwcnf.xz)"
	    xz -cdk $f > maxsat.pwcnf
	    ## MSU3
	    output=$output_dir"/MSU3/"$i_name	    
	    ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=2 -algorithm=1 -upmax maxsat.pwcnf
	    
	    ## WBO
	    output=$output_dir"/WBO/"$i_name
	    ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=2 -algorithm=0 -upmax maxsat.pwcnf
	    
	    ## OLL
	    output=$output_dir"/OLL/"$i_name
	    ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=2 -algorithm=2 -upmax maxsat.pwcnf
	    
	    ## RC2
	    output=$output_dir"/RC2/"$i_name
	    ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem python3 upPySAT/upRC2.py -f maxsat.pwcnf
	    
	    ## Hitman
	    output=$output_dir"/Hitman/"$i_name
	    ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem python3 upPySAT/upHitman.py -f maxsat.pwcnf	    
	    
	    rm -f maxsat.pwcnf
	done
  done
}

run_solvers_without_UP(){
   # Solvers that currently do not accept the pwcnf format: MaxHS and UWrMaxSAT 
   formulae_dir=$data_dir/"wcnf"
   output_dir=$results_dir/"wcnf"
   mkdir -p $output_dir"/MSU3/" $output_dir"/WBO/" $output_dir"/OLL/" $output_dir"/RC2/" $output_dir"/Hitman/" $output_dir"/MaxHS" $output_dir"/UWrMaxSAT"
   for f in $(find $formulae_dir/*xz -type f);
   do
     echo "Instance: "$f  
     i_name="$(basename $f .wcnf.xz)"
     xz -cdk $f > maxsat.wcnf
     ## MSU3
     output=$output_dir"/MSU3/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=0 -algorithm=1 maxsat.wcnf
     
     ## WBO
     output=$output_dir"/WBO/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=0 -algorithm=0 maxsat.wcnf
	    
     ## OLL
     output=$output_dir"/OLL/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UpMax/bin/upmax -formula=0 -algorithm=2 maxsat.wcnf
	    
     ## RC2
     output=$output_dir"/RC2/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem python3 upPySAT/upRC2.py --wcnf maxsat.wcnf
	    
     ## Hitman
     output=$output_dir"/Hitman/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem python3 upPySAT/upHitman.py --wcnf maxsat.wcnf	    

     ## MaxHS
     output=$output_dir"/MaxHS/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/MaxHS/bin/maxhs -no-printOptions -printSoln -verb=0 maxsat.wcnf

     ## UWrMaxSAT
     output=$output_dir"/UWrMaxSAT/"$i_name
     ./run --timestamp -o $output".out" -v $output".var" -w $output".wat" -C $wl -W $wl -M $mem ./solvers/UWrMaxSAT/bin/uwrmaxsat -v0 -no-sat -no-bin -m -bm maxsat.wcnf

     rm -f maxsat.wcnf
   done  
}

run_solvers(){
    data_dir=$dataset/$family
    results_dir="results/logs/"$family
    # pwcnfs    
    run_solvers_with_UP
    # wcnfs
    run_solvers_without_UP
}

echo
echo "Seating Assignment Problem"
echo
formulae=("table" "tag" "vig" "cvig" "res" "random")
family="seating-assignment"
run_solvers

echo
echo
echo "Minimum Sum Coloring (MSC) Problem"
echo
formulae=("vertex" "color" "vig" "cvig" "res" "random")
family="minimum-sum-coloring"
run_solvers

