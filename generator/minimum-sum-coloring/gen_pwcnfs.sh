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
per_timeout=150  # we only want at most 15% of timeouts
# n_min= 25
# n_max=75
p_min=0.40
p_max=0.65
timeouts=0    

#data_dir=$(pwd)
data_dir="/data/tmp/pwcnfs/minimum-sum-coloring"
mkdir -p $data_dir $data_dir/outputs $data_dir/graphs $data_dir/pwcnfs-color-based $data_dir/pwcnfs-vertex-based

gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit. 
for((g=$1; g<=$2; g++))
do  
   echo $g
   n=$(python -c "import random; print(random.randint("$n_min","$n_max"))")
   p=$(python -c "import random; print(round(random.uniform("$p_min","$p_max"),2))")
   i_name=g$g-n$n-p$(python -c "print(round(100*"$p"))")
   python3 generator.py -n $n -p $p > $data_dir/graphs/$i_name.g
   python3 msc.py -f $data_dir/graphs/$i_name.g > $data_dir/pwcnfs-vertex-based/$i_name.pwcnf
   gzip $data_dir/pwcnfs-vertex-based/$i_name.pwcnf
   timeout 600s ../../open-wbo -formula=2 $data_dir/pwcnfs-vertex-based/$i_name.pwcnf.gz > /tmp/$i_name.out
   # test timrout
   # exit_status=$?
   # if [[ $exit_status -eq 124 ]]; then
   #     if [[ $timeout -ge 150 ]]; then # we only want at most 15% of timeouts
   # 	   rm  $data_dir/graphs/$i_name.g
   # 	   rm  $data_dir/pwcnfs-vertex-based/$i_name.pwcnf.gz
   # 	   i=$((i-1))
   # 	   continue
   #     else
   # 	   timeout=$((timeout+1))
   #     fi
   #  fi
   
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$i_name.out) ]]; then
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


n_min=20
n_max=30
for((i=1; i<=$n_instances; i=i+50))
do
    gen_instances $n_min $n_max &
    n_min=$((n_min+10))
    n_max=$((n_max+10))
done
