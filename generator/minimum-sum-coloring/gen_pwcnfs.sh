#!/usr/bin/env bash
#Title			: gen_pwcnfs.sh
#Usage			: bash gen_pwcnfs.sh
#Author			: pmorvalho
#Date			: February 13, 2022
#Description	        : Generates pwcnfs instances giving the intervals minimum and maixmum number of nodes and probability of edges each graph should have
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_graphs=1000
per_timeout=150  # we only want at most 15% of timeouts
n_min=30
n_max=80
p_min=0.4
p_max=0.6
timeouts=0    

data_dir=$(pwd)
# data_dir="/data/tmp/pwcnfs/"
mkdir -p graphs minimum-sum-coloring-pwcnfs/color-based minimum-sum-coloring-pwcnfs/vertex-based

gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit. 
for((g=$1; g<=$2; g++))
do  
   echo $g
   n=$(python -c "import random; print(random.randint("$n_min","$n_max"))")
   p=$(python -c "import random; print(round(random.uniform("$p_min","$p_max"),2))")
   i_name=g$g-n$n-p$(python -c "print(round(100*"$p"))")
   python3 generator.py -n $n -p $p > graphs/$i_name.g
   python3 msc.py -f graphs/$i_name.g > minimum-sum-coloring-pwcnfs/vertex-based/$i_name.pwcnf
   gzip minimum-sum-coloring-pwcnfs/vertex-based/$i_name.pwcnf
   timeout 600s ../../open-wbo -formula=2 minimum-sum-coloring-pwcnfs/vertex-based/$i_name.pwcnf.gz > /tmp/$i_name.out
   # test timrout
   exit_status=$?
   if [[ $exit_status -eq 124 ]]; then
       if [[ $timeout -ge 150 ]]; then # we only want at most 15% of timeouts
	   rm  graphs/$i_name.g
	   rm   minimum-sum-coloring-pwcnfs/vertex-based/$i_name.pwcnf.gz
	   i=$((i-1))
	   continue
       else
	   timeout=$((timeout+1))
       fi
    fi
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$i_name.out) ]]; then
       rm  graphs/$i_name.g
       rm   minimum-sum-coloring-pwcnfs/vertex-based/$i_name.pwcnf.gz
       i=$((i-1))
       continue
   fi
   python3 msc.py -f graphs/$i_name.g -pc > minimum-sum-coloring-pwcnfs/color-based/$i_name.pwcnf
   gzip minimum-sum-coloring-pwcnfs/color-based/$i_name.pwcnf
done
}

for((i=1; i<=1000; i=i+100))
do
    gen_instances $i $((i+99)) &
done     
