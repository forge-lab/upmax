#!/usr/bin/env bash
#Title			: gen_pwcnfs.sh
#Usage			: bash gen_pwcnfs.sh
#Author			: pmorvalho
#Date			: February 16, 2022
#Description	        : Generates pwcnfs instances for the VMC problem, giving the  min and max values of servers and the percentage of usage
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_instances=1000
per_timeout=150  # we only want at most 15% of timeouts
# servers_min=10
# servers_max=40
vms_min=20
vms_max=45
timeouts=0    

#data_dir=$(pwd)
data_dir="/data/tmp/pwcnfs/virtual-machine-consolidation"
mkdir -p $data_dir $data_dir/outputs $data_dir/instances $data_dir/unweighted/pwcnfs-server-based $data_dir/unweighted/pwcnfs-vm-based $data_dir/weighted/pwcnfs-server-based $data_dir/weighted/pwcnfs-vm-based

build_pwcnf(){
   # $1 receives the file .out name to replace XX on the PWCNF formula with the values of its variables and clauses 
   file_name=$1
   clauses=$(tail -n 1 $file_name.out)
   vars=$(tail -n 2 $file_name.out | head -n 1)
   head -n 1 $file_name.out | sed -e 's/XX/'$vars' '$clauses'/g' >  $file_name.pwcnf
   sed -n 2,$((clauses+1))p $file_name.out >> $file_name.pwcnf
   rm $file_name.out
   gzip $file_name.pwcnf
}


gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit.
# $3 is the minimum number of servers and $4 the maximum 
for((g=$1; g<=$2; g++))
do  
   s=$(python -c "import random; print(random.randint("$3","$4"))")
   u=$(python -c "import random; print(random.randint("$vms_min","$vms_max"))")
   i_name=vmc$g-s$s-u$u
   ./vmc-generator $s $u $g > $data_dir/instances/$i_name.vmc
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -uw > $data_dir/unweighted/pwcnfs-server-based/$i_name.out
   build_pwcnf $data_dir/unweighted/pwcnfs-server-based/$i_name
   timeout 600s ../../open-wbo -formula=2 $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf.gz > /tmp/$i_name.out
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$i_name.out) ]]; then
       echo $i_name" unsat"
       rm  $data_dir/instances/$i_name.vmc
       rm  $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf.gz
       g=$((g-1))
       continue
   fi
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -pv -uw > $data_dir/unweighted/pwcnfs-vm-based/$i_name.out
   build_pwcnf $data_dir/unweighted/pwcnfs-vm-based/$i_name
   python3 vmc.py -f $data_dir/instances/$i_name.vmc > $data_dir/weighted/pwcnfs-server-based/$i_name.out
   build_pwcnf $data_dir/weighted/pwcnfs-server-based/$i_name
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -pv > $data_dir/weighted/pwcnfs-vm-based/$i_name.out
   build_pwcnf $data_dir/weighted/pwcnfs-vm-based/$i_name
   cp /tmp/$i_name.out $data_dir/outputs/.
done
}

servers_min=3
servers_max=5
for((i=1; i<=$n_instances; i=i+50))
do
    echo "Generating instances from "$i" to "$((i+50))	
    gen_instances $i $((i+50)) $servers_min $servers_max &
    servers_min=$((servers_min+2))
    servers_max=$((servers_max+2))
done     

