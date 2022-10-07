#!/usr/bin/env bash
#Title			: gen_pwcnfs.sh
#Usage			: bash gen_pwcnfs.sh
#Author			: pmorvalho
#Date			: February 13, 2022
#Description	        : Generates pwcnfs instances giving the intervals minimum and maixmum number of nodes and probability of edges each graph should have
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_instances=1000
# initial values
# p_min=0.40
# p_max=0.60
p_min=0.02
p_max=0.10

#data_dir=$(pwd)
data_dir="/data/tmp/tacas23/minimum-sum-coloring"
mkdir -p $data_dir $data_dir/outputs $data_dir/graphs $data_dir/pwcnfs-color-based $data_dir/pwcnfs-vertex-based

gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit.
# $3 is the current maximum number of nodes and $4 the minimum.
for((g=$1; g<=$2; g++))
do  
   n=$(python -c "import random; print(random.randint($3,$4))")
   p=$(python -c "import random; print(round(random.uniform("$p_min","$p_max"),2))")
   i_name=g$g-n$n-p$(python -c "print(round(100*"$p"))")
   echo $i_name
   python3 generator.py -n $n -p $p > $data_dir/graphs/$i_name.g
   python3 msc.py -f $data_dir/graphs/$i_name.g > $data_dir/pwcnfs-vertex-based/$i_name.pwcnf
   gzip $data_dir/pwcnfs-vertex-based/$i_name.pwcnf
   timeout 600s ../../open-wbo -formula=2 $data_dir/pwcnfs-vertex-based/$i_name.pwcnf.gz > /tmp/$i_name.out
   
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$i_name.out) ]]; then
       echo $g" UNSAT"
       rm  $data_dir/graphs/$i_name.g
       rm  $data_dir/pwcnfs-vertex-based/$i_name.pwcnf.gz
       i=$((i-1))
       continue
   fi
   python3 msc.py -f $data_dir/graphs/$i_name.g -pc > $data_dir/pwcnfs-color-based/$i_name.pwcnf
   gzip $data_dir/pwcnfs-color-based/$i_name.pwcnf
   cp /tmp/$i_name.out $data_dir/outputs/.
done
}

n_min=5  # initial value : 5 and final 85 
n_max=10 # initial value : 10 and final 90 
for((i=1; i<=$n_instances; i=i+50))
do
    echo "Generating instances from "$i" to "$((i+49))	
    gen_instances $i $((i+49)) $n_min $n_max &
    n_min=$((n_min+4))
    n_max=$((n_max+4))
done
