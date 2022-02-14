#!/usr/bin/env bash
#Title			: gen_instances.sh
#Usage			: bash gen_instances.sh
#Author			: pmorvalho
#Date			: February 14, 2022
#Description	        : Generates instances (using the generator.py script) for the Virtual Machine Consolidation (VMC) problem 
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_instances=50
servers_min=10
servers_max=30
vms_min=100
vms_max=320

mkdir instances
for((i=1; i<=$n_instances; i++))
do  
   echo $i
   s=$(python -c "import random; print(random.randint("$servers_min","$servers_max"))")
   v=$(python -c "import random; print(random.randint("$vms_min","$vms_max"))")
   timeout 5s python3 generator.py -s $s -v $v > instances/$i-s$s-vms$v.vmc
   exit_status=$?
   if [[ $exit_status -eq 124 ]]; then
       i=$((i-1))
   fi
done

