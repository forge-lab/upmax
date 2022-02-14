#!/usr/bin/env bash
#Title			: gen_graphs.sh
#Usage			: bash gen_graphs.sh
#Author			: pmorvalho
#Date			: February 13, 2022
#Description	        : Generates n_graphs graphs giving the intervals minimum and maixmum number of nodes and probability of edges
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_graphs=50
n_min=10
n_max=30
p_min=0.7
p_max=0.8

mkdir graphs
for((g=1; g<=$n_graphs; g++))
do  
   echo $g
   n=$(python -c "import random; print(random.randint("$n_min","$n_max"))")
   p=$(python -c "import random; print(round(random.uniform("$p_min","$p_max"),2))")
   python generator.py -n $n -p $p > graphs/g$g-n$n-p$(python -c "print(round(100*"$p"))").g 
done
